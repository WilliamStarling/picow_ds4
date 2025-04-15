// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Brian Starkey <stark3y@gmail.com>

#include <stdio.h>
#include <string.h>

#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "bt_hid.h"

// These magic values are just taken from M0o+, not calibrated for
// the Tiny chassis.
#define PWM_MIN 80
#define PWM_MAX (PWM_MIN + 127)

static inline int8_t clamp8(int16_t value) {
        if (value > 127) {
                return 127;
        } else if (value < -128) {
                return -128;
        }

        return value;
}

struct slice {
	unsigned int slice_num;
	unsigned int pwm_min;
};

struct chassis {
	struct slice slice_l;
	struct slice slice_r;

	int8_t l;
	int8_t r;
};

//Some of the triggers will rapidly switch between 1 and 0 when pressed,
//So making these to keep track of how long they've been released for.
//To make sure they're really off.
struct buttonStatus {
	int square;
	int ex;
	int circle;
	int triangle;
	int left;
	int down;
	int right;
	int up;
	int l1;
	int l2;
	int r1;
	int r2;
	int lJoy;
	int rJoy;
	int share;
	int options;
};

void init_slice(struct slice *slice, unsigned int slice_num, unsigned int pwm_min, uint8_t pin_a)
{
	slice->slice_num = slice_num;
	slice->pwm_min = pwm_min;
	gpio_set_function(pin_a, GPIO_FUNC_PWM);
	gpio_set_function(pin_a + 1, GPIO_FUNC_PWM);
	pwm_set_wrap(slice->slice_num, slice->pwm_min + 127 + 1);
	pwm_set_chan_level(slice->slice_num, PWM_CHAN_A, 0);
	pwm_set_chan_level(slice->slice_num, PWM_CHAN_B, 0);
	pwm_set_enabled(slice->slice_num, true);
}

void chassis_init(struct chassis *chassis, uint8_t pin_la, uint8_t pin_ra)
{

	init_slice(&chassis->slice_l, pwm_gpio_to_slice_num(pin_la), PWM_MIN, pin_la);
	init_slice(&chassis->slice_r, pwm_gpio_to_slice_num(pin_ra), PWM_MIN, pin_ra);
}

static inline uint8_t abs8(int8_t v) {
	return v < 0 ? -v : v;
}

void slice_set_with_brake(struct slice *slice, int8_t value, bool brake)
{
	uint8_t mag = abs8(value);

	if (value == 0) {
		pwm_set_both_levels(slice->slice_num, brake ? slice->pwm_min + 127 : 0, brake ? slice->pwm_min + 127 : 0);
	} else if (value < 0) {
		pwm_set_both_levels(slice->slice_num, slice->pwm_min + mag, 0);
	} else {
		pwm_set_both_levels(slice->slice_num, 0, slice->pwm_min + mag);
	}
}

void slice_set(struct slice *slice, int8_t value)
{
	slice_set_with_brake(slice, value, false);
}

void chassis_set_raw(struct chassis *chassis, int8_t left, int8_t right)
{
	slice_set(&chassis->slice_l, left);
	slice_set(&chassis->slice_r, right);

	chassis->l = left;
	chassis->r = right;
}

void chassis_set(struct chassis *chassis, int8_t linear, int8_t rot)
{
	// Positive rotation == CCW == right goes faster

	if (linear < -127) {
		linear = -127;
	}

	if (rot < -127) {
		rot = -127;
	}

	int l = linear - rot;
	int r = linear + rot;
	int adj = 0;

	if (l > 127) {
		adj = l - 127;
	} else if (l < -127) {
		adj = l + 127;
	}else if (r > 127) {
		adj = r - 127;
	} else if (r < -127) {
		adj = r + 127;
	}

	l -= adj;
	r -= adj;

	// FIXME: Motor directions should be a parameter
	r = -r;

	chassis_set_raw(chassis, l, r);
}

//ChatGPT gave me this, printing out the hex as bits to visualize it easier :)
void print_bits(uint8_t val) {
    for (int i = 7; i >= 0; i--) {
        printf("%d", (val >> i) & 1);
        if (i % 4 == 0) printf(" "); // optional: space every 4 bits
    }
    //printf("\n");
}

#define BUTTON_PRESSED 1
#define BUTTON_RELEASED -1
#define BUTTON_HELD 0
#define DEBOUNCE_TIME -11 //number of decrementations before the button is considered released. only works for odd numbers.

int buttonDebouncer(bool isPressed, int *buttonStatus) {
	//printf("Button status: %d\n", *buttonStatus);
	if(isPressed)
	{ //SQUARE BUTTON CODE
		if(*buttonStatus < DEBOUNCE_TIME) //only run this if the button had been released.
		{
			//Code for if Square / X button is pressed
			*buttonStatus = 1; //Change this so code doens't get spam ran.
			return BUTTON_PRESSED;
		}
		*buttonStatus = 1; //Change this so code doens't get spam ran.
	}
	else if (!isPressed)
	{
		*buttonStatus = *buttonStatus - 1;
		if (*buttonStatus == DEBOUNCE_TIME)
		{  //once the button has been released for long enough, run released code.
			//Code for if Square / X button is released	
			//Stop doing whatever it square made it do.
			return BUTTON_RELEASED;
		}
	}
	return BUTTON_HELD;
}

//Handles checking for each button press.
void ButtonHandler(struct bt_hid_state state, struct buttonStatus *buttonsStatus) 
{
	//First, assign buttons their values.
	uint8_t buttons = state.buttons;
	uint8_t triggers = state.triggers;
	uint8_t lx = state.lx;
	uint8_t ly = state.ly;
	uint8_t rx = state.rx;
	uint8_t ry = state.ry;

	/*
	//First, print out the buttons:
	printf("Face buttons: %02x, l: %d,%d, r: %d,%d, Trigger buttons: %02x",
		buttons, lx, ly, rx, ry, triggers);
	printf("\t Face Bits: ");
	print_bits(buttons);
	printf("\t Trigger Bits: ");
	print_bits(triggers);
	printf("\n");		
	/*

	//Check each button for if it's pressed.
	/*starting with 1 in binary, and then shifting it over a certain ammount gets a byte
	with just that bit on it's own. so using and & statement will return true if that bit
	is on and false if not.*/
	//Start with face buttons.
	if(buttons & (1 << 4))
	{ //SQUARE BUTTON CODE
		if(buttonsStatus->square < -5) //only run this if the button had been released.
		{
			//Code for if Square / X button is pressed
			printf("Square pressed\n");
		}
		buttonsStatus->square = 1; //Change this so code doens't get spam ran.
	}
	else if (!(buttons & (1 << 4)))
	{
		buttonsStatus->square--;
		if (buttonsStatus->square == -5)
		{  //once the button has been released for long enough, run released code.
			//Code for if Square / X button is released	
			//Stop doing whatever it square made it do.
			printf("Square released\n");
		}
	}
	/*
	That probably seemed very messy- and will continue to seem so. The logic behind it is thus.
	Soem of The buttons flicker between 1 and 0 when pressed, so I had to make sure it had been 0 for
	a sufficient ammount of time. also, I had to make sure the code for when the button is pressed is
	only ran once. so, whenever the input is 1 it updates the status varaible to 1, and when it was previously less
	than -5 it runs the button pressed code. Then, when the button is off, it decreements  the status variable. 
	Once it has been off for enough iterations, indicated by becoming -5, it runs the button released code.
	*/

	if(buttons & (1 << 5)) {
		//Code for if X / A button is pressed
	}
	if(buttons & (1 << 6)) {
		//Code for if Circle / B button is pressed
	}
	if(buttons & (1 << 7)) {
		//Code for if Triangle / Y button is pressed
	}

	//TRIGGER BUTTONS
	/*
	if(triggers & (1 << 3)) //RIGHT TRIGGER
	{ 
		if(buttonsStatus->r2 < -5) //only runs the button pressed code if it has been released.
		{
			//Code for if right trigger is pulled
			printf("right trigger pressed\n");
		}
		buttonsStatus->r2 = 1; //Change this so code doens't get spam ran.
	}
	else if (!(triggers & (1 << 3)))
	{
		buttonsStatus->r2 --;
		if (buttonsStatus->r2 == -5) 
		{  //once the button has been released for long enough, run released code.
			//Code for if right trigger is released	
			//Stop doing whatever it square made it do.
			printf("right trigger released\n");
		}
	}
	*/
	if(buttonDebouncer(triggers & (1 << 3), &buttonsStatus->r2) == BUTTON_PRESSED)
	{
		//Code for if right trigger is pulled
		printf("\t\t\tright trigger pressed\n");
	}
	else if(buttonDebouncer((triggers & (1 << 3)), &buttonsStatus->r2) == BUTTON_RELEASED)
	{
		//Code for if right trigger is released	
		//Stop doing whatever it square made it do.
		printf("\t\t\tright trigger released\n");
	}

	if(triggers & (1 << 2)) //LEFT TRIGGER
	{ 
		if(buttonsStatus->l2 < -5) //only runs the button pressed code if it has been released.
		{
			//Code for if left trigger is pulled
			printf("left trigger pressed\n");
		}
		buttonsStatus->l2 = 1; //Change this so code doens't get spam ran.
	}
	else if (!(triggers & (1 << 2)))
	{
		buttonsStatus->l2 --;
		if (buttonsStatus->l2 == -5) 
		{  //once the button has been released for long enough, run released code.
			//Code for if left trigger is released	
			//Stop doing whatever it square made it do.
			printf("left trigger released\n");
		}
	}
}

void main(void) {
	stdio_init_all();

	sleep_ms(1000);
	printf("Hello\n");

	multicore_launch_core1(bt_main);
	// Wait for init (should do a handshake with the fifo here?)
	sleep_ms(1000);

	struct chassis chassis = { 0 };
	chassis_init(&chassis, 6, 8);

	struct bt_hid_state state;
	struct buttonStatus buttonsStatus = { 0 };
	for ( ;; ) {
		sleep_ms(20);
		bt_hid_get_latest(&state);

		//handle button inputs
		ButtonHandler(state, &buttonsStatus);

		float speed_scale = 1.0;
		int8_t linear = clamp8(-(state.ly - 128) * speed_scale);
		int8_t rot = clamp8(-(state.rx - 128));
		chassis_set(&chassis, linear, rot);
	}
}

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

//const uint LED_PIN = 13; //for blinking an led.

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
#define DEBOUNCE_TIME -5 //number of decrementations before the button is considered released. only works for odd numbers.

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
	/*
	That probably seemed very messy- and will continue to seem so. The logic behind it is thus.
	Soem of The buttons flicker between 1 and 0 when pressed, so I had to make sure it had been 0 for
	a sufficient ammount of time. also, I had to make sure the code for when the button is pressed is
	only ran once. so, whenever the input is 1 it updates the status varaible to 1, and when it was previously less
	than -5 it runs the button pressed code. Then, when the button is off, it decreements  the status variable. 
	Once it has been off for enough iterations, indicated by becoming -5, it runs the button released code.
	*/
}

//Handles checking for each button press.
void ButtonHandler(struct bt_hid_state state, struct buttonStatus *buttonsStatus) 
{
	//First, assign buttons their values.
	uint8_t buttons = state.buttons;
	uint8_t triggers = state.triggers;
	/*The joysticks by default range from 0 - 255 (and in the case of the horizontal)
	axis, 0 is all the way up). This code helps convert that from -127 to 128. 
	Feel free to mess with as needed for your function calls. */
	int ly = -state.ly + 128; //Center the joystick values to be between -127 and 128.
	int lx = state.lx - 128; //Center the joystick values to be between -128 and 127.
	int ry = -state.ry + 128; //Center the joystick values to be between -127 and 128.
	int rx = state.rx - 128; //Center the joystick values to be between -128 and 127.

	int stick_threshold = 10; //Threshold for joystick to be considered pushed. 10 is a good value, but can be changed.

	//State variables for keeping track if the buttons were pressed, released, or being held.
	int square_state = 0;
	int ex_state = 0;
	int circle_state = 0;
	int triangle_state = 0;
	int left_state = 0;
	int down_state = 0;
	int right_state = 0;
	int up_state = 0;
	int l1_state = 0;
	int l2_state = 0;
	int r1_state = 0;
	int r2_state = 0;
	int lJoy_state = 0;
	int rJoy_state = 0;
	int share_state = 0;
	int options_state = 0;

	/*
	//First, print out the controller input:
	printf("Face buttons: %02x, l: %d,%d, r: %d,%d, Trigger buttons: %02x",
		buttons, lx, ly, rx, ry, triggers);
	printf("\t Face Bits: ");
	print_bits(buttons);
	printf("\t Trigger Bits: ");
	print_bits(triggers);
	printf("\n");		
	*/

	//JOYSTICKS

	if(ly > stick_threshold || ly < -stick_threshold) //LEFT JOYSTICK VERTICAL
	{
		//Code for if left joystick is moved vertically.
		printf("left joystick moved vertically: %d\n", ly);
	}
	if(lx > stick_threshold || lx < -stick_threshold) //LEFT JOYSTICK HORIZONTAL
	{
		//Code for if left joystick is moved horizontally.
		printf("left joystick moved horizontally: %d\n", lx);
	}

	if(ry > stick_threshold || ry < -stick_threshold) //RIGHT JOYSTICK VERTICAL
	{
		//Code for if right joystick is moved vertically.
		printf("right joystick moved vertically: %d\n", ry);
	}
	if(rx > stick_threshold || rx < -stick_threshold) //RIGHT JOYSTICK HORIZONTAL
	{
		//Code for if right joystick is moved horizontally.
		printf("right joystick moved horiontally: %d\n", rx);
	}

	//Check each button for if it's pressed.

	//Dpad input uses a "Hat switch" encoding, so this code takes care of that. 2 alternatives are presented. but Hat switch looks like this:
/*
   0
 7   1
6  8  2
 5   3
   4
*/

//option 1: each direction is assigned an input, including diagonal, for a total of 8 individual actions.
switch (buttons) {
    case 0:
		//Up code 
		printf("Up pressed\n");
		break;
    case 1:   
		//Up + Right code
		printf("Up + Right pressed\n");
		break;
    case 2:
		//Right code
		printf("Right pressed\n");
		break;
    case 3:
		//Down + Right code
		printf("Down + Right pressed\n");
		break;
    case 4:
		//Down code
		printf("Down pressed\n");
		break;
    case 5:
		//Down + Left code
		printf("Down + Left pressed\n");
		break;
    case 6:
		//Left code
		printf("Left pressed\n");
		break;
    case 7:
		//Up + Left code
		printf("Up + Left pressed\n");
		break;
    case 8:
		//Centered, nothing pressed
		break;
}

/*
//Option 2: each button is assigned an individual function, that so 2 can be performed at once if both are pressed.
	bool dpad_up    = (buttons == 0 || buttons == 1 || buttons == 7);
	bool dpad_right = (buttons == 1 || buttons == 2 || buttons == 3);
	bool dpad_down  = (buttons == 3 || buttons == 4 || buttons == 5);
	bool dpad_left  = (buttons == 5 || buttons == 6 || buttons == 7);
	if(dpad_up) //UP BUTTON
	{
		//Code for if up button is pressed
		printf("up button pressed\n");
	}
	if(dpad_right) //RIGHT BUTTON
	{
		//Code for if right button is pressed
		printf("right button pressed\n");
	}
	if(dpad_down) //DOWN BUTTON
	{
		//Code for if down button is pressed
		printf("down button pressed\n");
	}
	if(dpad_left) //LEFT BUTTON
	{
		//Code for if left button is pressed
		printf("left button pressed\n");
	}
*/

	/*starting with 1 in binary, and then shifting it over a certain ammount gets a byte
	with just that bit on it's own. so using and & statement will return true if that bit
	is on and false if not.*/
	//Start with shape buttons.
	square_state = buttonDebouncer(buttons & (1 << 4), &buttonsStatus->square); //SQUARE BUTTON
	if(square_state == BUTTON_PRESSED)
	{
		//Code for if Square / X button is pressed
		printf("Square pressed\n");
		//gpio_put(LED_PIN, 1); //turns the led on when pressed
	}
	else if(square_state == BUTTON_RELEASED)
	{
		//Code for if Square / X button is released	
		//Stop doing whatever it square made it do.
		printf("Square released\n");
		//gpio_put(LED_PIN, 0); turns the led off when released
	}

	ex_state = buttonDebouncer(buttons & (1 << 5), &buttonsStatus->ex); //EX BUTTON
	if(ex_state == BUTTON_PRESSED) 
	{
		//Code for if X / A button is pressed
		printf("Ex pressed\n");
	} else if(ex_state == BUTTON_RELEASED) 
	{
		//Code for if X / A button is released	
		//Stop doing whatever it square made it do.
		printf("Ex released\n");
	}

	circle_state = buttonDebouncer(buttons & (1 << 6), &buttonsStatus->circle); //CIRCLE BUTTON
	if(circle_state == BUTTON_PRESSED) 
	{
		//Code for if Circle / B button is pressed
		printf("Circle pressed\n");
	}
	else if(circle_state == BUTTON_RELEASED) 
	{
		//Code for if Circle / B button is released	
		//Stop doing whatever it square made it do.
		printf("Circle released\n");
	}

	triangle_state = buttonDebouncer(buttons & (1 << 7), &buttonsStatus->triangle); //TRIANGLE BUTTON
	if(triangle_state == BUTTON_PRESSED){
		//Code for if Triangle / Y button is pressed
		printf("Triangle pressed\n");
	}
	else if(triangle_state == BUTTON_RELEASED) 
	{
		//Code for if Triangle / Y button is released	
		//Stop doing whatever it square made it do.
		printf("Triangle released\n");
	}

	//TRIGGER BUTTONS
	l1_state = buttonDebouncer(triggers & (1 << 0), &buttonsStatus->l1); //LEFT BUMPER BUTTON
	if(l1_state == BUTTON_PRESSED) 
	{
		//Code for if left bumper is pressed
		printf("left bumper pressed\n");
	}
	else if(l1_state == BUTTON_RELEASED) 
	{
		//Code for if left bumper is released	
		//Stop doing whatever left bumper made it do.
		printf("left bumper released\n");
	}

	r1_state = buttonDebouncer(triggers & (1 << 1), &buttonsStatus->r1); //RIGHT BUMPER BUTTON
	if(r1_state == BUTTON_PRESSED) 
	{
		//Code for if right bumper is pressed
		printf("right bumper pressed\n");
	}
	else if(r1_state == BUTTON_RELEASED) 
	{
		//Code for if right bumper is released	
		//Stop doing whatever right bumper made it do.
		printf("right bumper released\n");
	}

	l2_state = buttonDebouncer(triggers & (1 << 2), &buttonsStatus->l2); //LEFT TRIGGER
	if(l2_state == BUTTON_PRESSED)
	{ 
		//Code for if left trigger is pulled
		printf("left trigger pressed\n");
	}
	else if (l2_state == BUTTON_RELEASED)
	{ 
		//Code for if left trigger is released	
		//Stop doing whatever left trigger made it do.
		printf("left trigger released\n");
	}

	r2_state = buttonDebouncer(triggers & (1 << 3), &buttonsStatus->r2); //RIGHT TRIGGER
	if(r2_state == BUTTON_PRESSED)
	{
		//Code for if right trigger is pulled
		printf("right trigger pressed\n");
	}
	else if(r2_state == BUTTON_RELEASED)
	{
		//Code for if right trigger is released	
		//Stop doing whatever right trigger made it do.
		printf("right trigger released\n");
	}

	//EXTRA BUTTONS
	share_state = buttonDebouncer(triggers & (1 << 4), &buttonsStatus->share); //SHARE BUTTON
	if(share_state == BUTTON_PRESSED) 
	{
		//Code for if share button is pressed
		printf("share button pressed\n");
	}
	else if(share_state == BUTTON_RELEASED) 
	{
		//Code for if share button is released	
		//Stop doing whatever share button made it do.
		printf("share button released\n");
	}

	options_state = buttonDebouncer(triggers & (1 << 5), &buttonsStatus->options); //OPTIONS BUTTON
	if(options_state == BUTTON_PRESSED) 
	{
		//Code for if options button is pressed
		printf("options button pressed\n");
	}
	else if(options_state == BUTTON_RELEASED) 
	{
		//Code for if options button is released	
		//Stop doing whatever options button made it do.
		printf("options button released\n");
	}
	
	lJoy_state = buttonDebouncer(triggers & (1 << 6), &buttonsStatus->lJoy); //LEFT JOYSTICK BUTTON
	if(lJoy_state == BUTTON_PRESSED) 
	{
		//Code for if right joystick is pressed
		printf("left joystick pressed\n");
	}
	else if(lJoy_state == BUTTON_RELEASED) 
	{
		//Code for if right joystick is released	
		//Stop doing whatever right joystick made it do.
		printf("left joystick released\n");
	}

	rJoy_state = buttonDebouncer(triggers & (1 << 7), &buttonsStatus->rJoy); //LEFT JOYSTICK BUTTON
	if(rJoy_state == BUTTON_PRESSED) 
	{
		//Code for if left joystick is pressed
		printf("right joystick pressed\n");
	}
	else if(rJoy_state == BUTTON_RELEASED) 
	{
		//Code for if left joystick is released	
		//Stop doing whatever left joystick made it do.
		printf("right joystick released\n");
	}
}

void main(void) {
	stdio_init_all();

	sleep_ms(1000);
	printf("Hello\n");

	//test code for blinking an LED. happens in the square button, with gpio pin 13.
	//gpio_init(LED_PIN);
	//gpio_set_dir(LED_PIN, GPIO_OUT);
	
	multicore_launch_core1(bt_main);
	// Wait for init (should do a handshake with the fifo here?)
	sleep_ms(1000);
	
	struct bt_hid_state state;
	struct buttonStatus buttonsStatus = { 0 };
	for ( ;; ) {https://docs.google.com/document/d/1Wt3UV09HwD1t7vMnimtrmzCTw2O6JCgw0TMRz4ddzdU/edit?usp=sharing
		sleep_ms(20);
		bt_hid_get_latest(&state);

		//handle button inputs
		ButtonHandler(state, &buttonsStatus);
	}
}

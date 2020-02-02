#ifndef COMMONINPUT_H
#define COMMONINPUT_H

#include <bits/stdint-uintn.h>
//enum virtualController
//{
//    CROSS,
//    TRIANGLE,
//    CIRCLE,
//    SQUARE,
//    START,
//    SELECT,
//    R1,
//    R2,
//    R3,
//    L1,
//    L2,
//    L3,
//    virtualController_MAX
//};

enum CONTROLLER_BUTTON
{
    SELECT,
    L3,
    R3,
    START,
    UP,
    RIGHT,
    DOWN,
    LEFT,
    L2,
    R2,
    L1,
    R1,
    TRIANGLE,
    CIRCLE,
    CROSS,
    SQUARE,
    CONTROLLER_BUTTON_MAX
};


enum deviceType
{
	CONTROLLER,
	MOUSE,
	KEYBOARD,
	USB
}; // USB is a subset of controllers like Guitar Hero stuff and for Pandubz future Eye Toy PR. Right pandubz ... you didnt forget, right?

struct inputEvent
{
	bool input[CONTROLLER_BUTTON_MAX];
	int playerNumber;

	float lStickXAxis;
	float lStickYAxis;

	float rStickXAxis;
	float rStickYAxis;

	//void construct(virtualController buttonStates[virtualController_MAX], int playerNum, int lXAxis, int lYAxis, int rXAxis, int rYAxis)
	//{
	//	//input = eventI;
	//	input = buttonStates;
	//	lStickXAxis = lXAxis;
	//	lStickYAxis = lYAxis;
	//	rStickXAxis = rXAxis;
	//	rStickYAxis = rYAxis;
	//	playerNumber = playerNum;
	//}
};

enum DeviceAPI
{
	NO_API = 0,
	WM = 2,	 // Windows Keyboard
	RAW = 3, // It's FUCKING RAW!!!
	XINPUT = 4,
	DS4_LIB = 5,	   // Potential custom option for later
	LNX_KEYBOARD = 16, // Xorg / Wayland input specific or we do some real shitz!
	EVDEV = 17,
};

class CommonInput
{

protected:
	DeviceAPI currentAPI;
	int playerNumber;

public:
	virtual bool reset() = 0;
	virtual inputEvent poll() = 0;
};

#endif

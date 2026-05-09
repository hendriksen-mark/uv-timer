#pragma once


#include <Button2.h>

void initButtons();

// Service all buttons (must be called regularly in loop)
void serviceButtons();

// Event-checking functions (used within control flows)
clickType buttonEvent(Button2 &btn);
bool buttonPressed(Button2 &btn);
bool buttonDoublePressed(Button2 &btn);
bool buttonLongPressed(Button2 &btn);
bool buttonWasPressedAndConsume(Button2 &btn);
bool anyButtonPressedAndConsume();
bool waitForAnyButtonPress();

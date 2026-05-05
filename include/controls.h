#pragma once


// Turn off UV MOSFET outputs and turn ON white strip MOSFET
void outputsIdle();

// Turn on UV MOSFET outputs based on current mode and turn OFF white strip MOSFET
void outputsUvOnForMode();

// Buzzer driver function to turn buzzer on or off based on active high setting
void writeBuzzer(bool on);

// Beep sequence to indicate timer complete
void beepDone();

// Hidden menu for calibration and settings
void hiddenMenu();

// Time editing menu
void editTimeMenu();

// Show a quick help screen with button mappings
void showHelpScreen();

// Stepwise exposure mode for dialing in exposure time with a mask strip
void runTestStripMode();

// Run the timer exposure cycle
void runTimerCycle();

// Confirmation dialog for critical actions, returns true if user confirms
bool confirmAction(const char *message);


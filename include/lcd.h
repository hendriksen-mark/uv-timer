#pragma once


#include <stdint.h>

// Print time formatted as MM:SS
void lcdPrintTime(uint8_t mm, uint8_t ss);

// Display startup information screen
void showStartupMessage();

// Draw the main start screen
void drawStartScreen();

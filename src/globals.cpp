#include "globals.h"

// ============================================================================
// LCD INSTANCE
// ============================================================================
LiquidCrystal_I2C lcd(LCD_COLS, LCD_ROWS);

// ============================================================================
// TIMER STATE (Interrupt-driven exposure countdown)
// ============================================================================
volatile bool timerGo = false;
volatile uint8_t countMin = 0;
volatile uint8_t countSec = 0;
volatile uint16_t tickMs = 1000;
volatile uint16_t msAccumulator = 0;

// ============================================================================
// BUTTON INSTANCES
// ============================================================================
Button2 button1;
Button2 button2;
Button2 button3;

// ============================================================================
// USER SETTINGS (Persisted in EEPROM or runtime-configured)
// ============================================================================
uint8_t singleDouble = 0; // 0=single sided, 1=double sided
uint8_t timeMin = 0;      // exposure time minutes
uint8_t timeSec = 10;     // exposure time seconds
uint8_t calByte = 128;    // timing calibration byte (0..255)
uint8_t whitePwm = 255;   // white strip PWM brightness (0..255)
bool uvActiveHigh = DEFAULT_UV_ACTIVE_HIGH;
bool whiteActiveHigh = DEFAULT_WHITE_ACTIVE_HIGH;
bool buzzerActiveHigh = DEFAULT_BUZZER_ACTIVE_HIGH;
bool skipStartup = false;      // skip startup message on boot
uint8_t topBottom = 0;         // which side for single-sided exposure
bool doubleSideEnabled = true; // whether double-sided mode is available
uint8_t stripStepCount = DEFAULT_TEST_STRIP_STEPS;
uint8_t stripStepMin = DEFAULT_TEST_STRIP_MIN;
uint8_t stripStepSec = DEFAULT_TEST_STRIP_SEC;
uint8_t buzzerMode = DEFAULT_BUZZER_MODE;
uint8_t debugLogLevel = DEFAULT_DEBUG_LEVEL;

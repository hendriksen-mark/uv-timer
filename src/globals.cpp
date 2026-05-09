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
uint8_t singleDouble = DEFAULT_SINGLE_DOUBLE;         // 0=single sided, 1=double sided
uint8_t timeMin = DEFAULT_TIME_MIN;                   // exposure time minutes
uint8_t timeSec = DEFAULT_TIME_SEC;                   // exposure time seconds
uint8_t calByte = DEFAULT_CAL_BYTE;                   // timing calibration byte (0..255)
uint8_t whitePwm = DEFAULT_WHITE_PWM;                 // white strip PWM brightness (0..255)
bool uvActiveHigh = DEFAULT_UV_ACTIVE_HIGH;           // UV MOSFET polarity: true=active high
bool whiteActiveHigh = DEFAULT_WHITE_ACTIVE_HIGH;     // white MOSFET polarity: true=active high
bool buzzerActiveHigh = DEFAULT_BUZZER_ACTIVE_HIGH;   // buzzer polarity: true=active high
bool skipStartup = DEFAULT_SKIP_STARTUP;              // skip startup message on boot
uint8_t topBottom = DEFAULT_TOP_BOTTOM;               // which side for single-sided exposure
bool doubleSideEnabled = DEFAULT_DOUBLE_SIDE_ENABLED; // whether double-sided mode is available
uint8_t stripStepCount = DEFAULT_TEST_STRIP_STEPS;    // last-used test strip step count
uint8_t stripStepMin = DEFAULT_TEST_STRIP_MIN;        // last-used test strip step minutes
uint8_t stripStepSec = DEFAULT_TEST_STRIP_SEC;        // last-used test strip step seconds
uint8_t buzzerMode = DEFAULT_BUZZER_MODE;             // buzzer mode 0=off, 1=short, 2=double, 3=alarm
uint8_t debugLogLevel = DEFAULT_DEBUG_LEVEL;          // debug log level 0=LVL_INFO, 1=LVL_DEBUG
bool serialWaitEnabled = DEFAULT_SERIAL_WAIT_ENABLED; // wait for serial connection at boot

#pragma once


// ============================================================================
// PROJECT CONFIGURATION
// ============================================================================
#define APP_DEBUG_BAUD 115200
#define APP_SERIAL_WAIT_MS 10000
#define APP_SERIAL_HEARTBEAT_INTERVAL_MS 500
#define APP_RGB_LED_COUNT 1
#define APP_RGB_FADE_IN_STEPS 50
#define APP_RGB_FADE_OUT_STEPS 50
#define APP_RGB_FADE_STEP_DELAY_MS 20

// ============================================================================
// PIN MAPPING (Raspberry Pi Pico GPIO numbers)
// ============================================================================
#define PIN_LCD_SDA 0
#define PIN_LCD_SCL 1

#define PIN_BUZZER -1 // is now in the lcd driver backlight pin, so no separate buzzer pin needed
#define PIN_WHITE_MOSFET 9
#define PIN_UV_MOSFET_1 13
#define PIN_UV_MOSFET_2 11

// ============================================================================
// LCD CONFIGURATION
// ============================================================================
#define LCD_I2C_ADDR 0x20
#define LCD_COLS 16
#define LCD_ROWS 2

// ============================================================================
// SETTINGS STORAGE (Filesystem + JSON on RP2040)
// ============================================================================
#define SETTINGS_FILE "/settings.json"

// ============================================================================
// BUTTON PIN CONFIGURATION (direct GPIO, active low, internal pull-up)
// ============================================================================
#define PIN_BTN1 5
#define PIN_BTN2 6
#define PIN_BTN3 7
#define APP_BTN_DEBOUNCE_MS   30
#define APP_BTN_DOUBLE_MS     350
#define APP_BTN_LONG_MS       700
#define APP_BTN_HOLD_REPEAT_MS 150
#define APP_BTN_HOLD_ACCEL_AFTER_MS 1000
#define APP_BTN_HOLD_REPEAT_FAST_MS 90

// ============================================================================
// TIMING & CALIBRATION DEFAULTS
// ============================================================================
#define DEFAULT_TIME_MIN 0
#define DEFAULT_TIME_SEC 30
#define DEFAULT_CAL_BYTE 128  // 128 = exactly 1000 ms
#define DEFAULT_WHITE_PWM 160 // comfortable inspection brightness
#define DEFAULT_UV_ACTIVE_HIGH 1
#define DEFAULT_WHITE_ACTIVE_HIGH 1
#define DEFAULT_BUZZER_ACTIVE_HIGH 1
#define DEFAULT_SKIP_STARTUP 0
#define DEFAULT_TOP_BOTTOM 0
#define DEFAULT_DOUBLE_SIDE_ENABLED 1
#define DEFAULT_SINGLE_DOUBLE 1
#define DEFAULT_TEST_STRIP_STEPS 5
#define DEFAULT_TEST_STRIP_MIN DEFAULT_TIME_MIN
#define DEFAULT_TEST_STRIP_SEC DEFAULT_TIME_SEC
#define DEFAULT_BUZZER_MODE 2
#define DEFAULT_DEBUG_LEVEL 1 // 0=LVL_INFO, 1=LVL_DEBUG
#define DEFAULT_SERIAL_WAIT_ENABLED 1 // 1=wait for serial at boot, 0=skip

// ============================================================================
// TIMING CONSTRAINTS
// ============================================================================
#define MIN_TICK_MS 850
#define MAX_TICK_MS 1150
#define TIMER_CALIBRATION_CENTER 128

#ifndef GLOBALS_H
#define GLOBALS_H

#pragma once

#include <Arduino.h>
#include <IskakINO_LiquidCrystal_I2C.h>
#include <Button2.h>
#include "config.h"

// ============================================================================
// LCD INSTANCE
// ============================================================================
extern LiquidCrystal_I2C lcd;

// ============================================================================
// TIMER STATE (Interrupt-driven exposure countdown)
// ============================================================================
extern volatile bool timerGo;
extern volatile uint8_t countMin;
extern volatile uint8_t countSec;
extern volatile uint16_t tickMs;
extern volatile uint16_t msAccumulator;

// ============================================================================
// BUTTON INSTANCES
// ============================================================================
extern Button2 button1;
extern Button2 button2;
extern Button2 button3;

// ============================================================================
// USER SETTINGS (Persisted in EEPROM or runtime-configured)
// ============================================================================
extern uint8_t singleDouble;   // 0=single sided, 1=double sided
extern uint8_t timeMin;        // exposure time minutes
extern uint8_t timeSec;        // exposure time seconds
extern uint8_t calByte;        // timing calibration byte (0..255)
extern uint8_t whitePwm;       // white strip PWM brightness (0..255)
extern bool uvActiveHigh;      // UV MOSFET polarity: true=active high
extern bool whiteActiveHigh;   // white MOSFET polarity: true=active high
extern bool buzzerActiveHigh;  // buzzer polarity: true=active high
extern bool skipStartup;       // skip startup message on boot
extern uint8_t topBottom;      // which side for single-sided exposure
extern bool doubleSideEnabled; // whether double-sided mode is available
extern uint8_t stripStepCount; // last-used test strip step count
extern uint8_t stripStepMin;   // last-used test strip step minutes
extern uint8_t stripStepSec;   // last-used test strip step seconds
extern uint8_t buzzerMode;     // 0=off, 1=short, 2=double
extern uint8_t debugLogLevel;  // 0=LVL_INFO, 1=LVL_DEBUG

#endif // GLOBALS_H

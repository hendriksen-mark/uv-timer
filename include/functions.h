#pragma once


#include <NeoPixelBus.h>

#include "config.h"

void serialWait();
void ChangeNeoPixels_info();
void setInfoLedBrightness(float brightness);
void infoLight(RgbColor color);
void infoLedOff();
void infoLedFadeIn(RgbColor color, uint16_t duration = 500);
void infoLedFadeOut(uint16_t duration = 500);
void infoLedPulse(RgbColor color, uint8_t pulses = 1, uint16_t pulseDuration = 1000);
void infoLedIdle();
void infoLedBusy();
void infoLedSuccess();
void infoLedError();
void blinkLed(uint8_t count, uint16_t interval = 200);


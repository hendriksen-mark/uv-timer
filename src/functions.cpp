#include "functions.h"
#include <DebugLog.h>
#include <NeoPixelBus.h>
#include "globals.h"

#define PRIMITIVE_CAT3(a, b, c) a##b##c
#define CAT3(a, b, c) PRIMITIVE_CAT3(a, b, c)
typedef CAT3(Neo, INFO_LED_ORDER, Feature) InfoLedColorFeature;

NeoPixelBus<InfoLedColorFeature, Rp2040x4Pio1Ws2812xMethod> *strip_info = NULL;
float info_led_brightness = 0.3; // Default brightness (30% to avoid being too bright)

RgbColor red = RgbColor(255, 0, 0);
RgbColor green = RgbColor(0, 255, 0);
RgbColor blue = RgbColor(0, 0, 255);
RgbColor yellow = RgbColor(255, 255, 0);
RgbColor cyan = RgbColor(0, 255, 255);
RgbColor magenta = RgbColor(255, 0, 255);
RgbColor orange = RgbColor(255, 165, 0);
RgbColor purple = RgbColor(128, 0, 128);
RgbColor white = RgbColor(255);
RgbColor black = RgbColor(0);

void serialWait()
{
	// Give user time to open serial monitor while showing progress on the LCD.
	const int waitMs = APP_SERIAL_WAIT_MS;
	const int intervalMs = APP_SERIAL_HEARTBEAT_INTERVAL_MS;
	const int loops = max(1, waitMs / intervalMs);
	lcd.print("Serial wait");

	for (int i = 0; i < loops; ++i)
	{
		const int remainingSec = (waitMs - i * intervalMs + 999) / 1000;
		const int filled = ((i + 1) * LCD_COLS) / loops;

		char line0[17];
		snprintf(line0, sizeof(line0), "%2ds", remainingSec);
		lcd.setCursor(13, 0);
		lcd.print(line0);

		lcd.setCursor(0, 1);
		for (int col = 0; col < LCD_COLS; ++col)
		{
			lcd.print(col < filled ? '#' : '-');
		}

		LOG_INFO("Waiting for serial monitor...", remainingSec, "seconds remaining");
		infoLedPulse(white, 1, intervalMs); // Pulse white while waiting for serial monitor
	}

	lcd.clear();
}

void ChangeNeoPixels_info() // this set the number of leds of the strip based on web configuration
{
	if (strip_info != NULL)
	{
		delete strip_info; // delete the previous dynamically created strip
	}
	// Sanity check pin again before initializing RMT
	LOG_DEBUG("INFO_DATA_PIN=", INFO_DATA_PIN);
	if (INFO_DATA_PIN < 0 || INFO_DATA_PIN > 47)
	{
		LOG_ERROR("ChangeNeoPixels_info: invalid INFO_DATA_PIN, aborting strip init");
		strip_info = NULL;
		return;
	}
	strip_info = new NeoPixelBus<InfoLedColorFeature, Rp2040x4Pio1Ws2812xMethod>(APP_RGB_LED_COUNT, INFO_DATA_PIN); // and recreate with new count
	strip_info->Begin();
}

// Helper function to apply brightness to a color
RgbColor applyBrightness(RgbColor color, float brightness)
{
	brightness = constrain(brightness, 0.0, 1.0);
	return RgbColor(
		(uint8_t)(color.R * brightness),
		(uint8_t)(color.G * brightness),
		(uint8_t)(color.B * brightness));
}

void setInfoLedBrightness(float brightness)
{
	info_led_brightness = constrain(brightness, 0.0, 1.0);
}

void blinkLed(uint8_t count, uint16_t interval)
{
	if (strip_info == NULL)
	{
		ChangeNeoPixels_info();
	}

	if (strip_info == NULL)
		return;

	RgbColor color = strip_info->GetPixelColor(0);
	for (uint8_t i = 0; i < count; i++)
	{
		strip_info->SetPixelColor(0, black);
		strip_info->Show();
		delay(interval);
		strip_info->SetPixelColor(0, color);
		strip_info->Show();
		delay(interval);
	}
}

void infoLight(RgbColor color)
{ // boot animation for leds count and wifi test
	if (strip_info == NULL)
	{
		ChangeNeoPixels_info();
	}

	if (strip_info == NULL)
		return;

	// Flash the strip in the selected color. White = booted, green = WLAN connected, red = WLAN could not connect
	RgbColor adjusted_color = applyBrightness(color, info_led_brightness);
	strip_info->SetPixelColor(0, adjusted_color);
	strip_info->Show();
}

void infoLedOff()
{
	if (strip_info == NULL)
	{
		ChangeNeoPixels_info();
	}

	if (strip_info == NULL)
		return;

	strip_info->SetPixelColor(0, black);
	strip_info->Show();
}

void infoLedFadeIn(RgbColor color, uint16_t duration)
{
	if (strip_info == NULL)
	{
		ChangeNeoPixels_info();
	}

	if (strip_info == NULL)
		return;

	uint8_t steps = APP_RGB_FADE_IN_STEPS;
	uint16_t stepDelay = duration / steps;

	for (uint8_t i = 0; i <= steps; i++)
	{
		float progress = (float)i / steps;
		RgbColor fade_color = applyBrightness(color, progress * info_led_brightness);
		strip_info->SetPixelColor(0, fade_color);
		strip_info->Show();
		delay(stepDelay);
	}
}

void infoLedFadeOut(uint16_t duration)
{
	if (strip_info == NULL)
	{
		ChangeNeoPixels_info();
	}

	if (strip_info == NULL)
		return;

	RgbColor current_color = strip_info->GetPixelColor(0);
	uint8_t steps = APP_RGB_FADE_OUT_STEPS;
	uint16_t stepDelay = duration / steps;

	for (uint8_t i = steps; i > 0; i--)
	{
		float progress = (float)i / steps;
		RgbColor fade_color = RgbColor(
			(uint8_t)(current_color.R * progress),
			(uint8_t)(current_color.G * progress),
			(uint8_t)(current_color.B * progress));
		strip_info->SetPixelColor(0, fade_color);
		strip_info->Show();
		delay(stepDelay);
	}
	strip_info->SetPixelColor(0, black);
	strip_info->Show();
}

void infoLedPulse(RgbColor color, uint8_t pulses, uint16_t pulseDuration)
{
	if (strip_info == NULL)
	{
		ChangeNeoPixels_info();
	}

	if (strip_info == NULL)
		return;

	for (uint8_t p = 0; p < pulses; p++)
	{
		infoLedFadeIn(color, pulseDuration / 2);
		infoLedFadeOut(pulseDuration / 2);
		if (p < pulses - 1)
		{
			delay(pulseDuration / 4); // Short pause between pulses
		}
	}
}

// Status indication helpers
void infoLedIdle()
{
	if (strip_info == NULL)
	{
		ChangeNeoPixels_info();
	}
	if (strip_info == NULL)
		return;
	RgbColor dim_blue = applyBrightness(blue, info_led_brightness * 0.3);
	strip_info->SetPixelColor(0, dim_blue);
	strip_info->Show();
}

void infoLedBusy()
{
	if (strip_info == NULL)
	{
		ChangeNeoPixels_info();
	}
	if (strip_info == NULL)
		return;
	infoLedPulse(orange, 1, 1000); // Orange pulse
}

void infoLedSuccess()
{
	if (strip_info == NULL)
	{
		ChangeNeoPixels_info();
	}
	if (strip_info == NULL)
		return;
	infoLedPulse(green, 2, 400); // Two quick green pulses
}

void infoLedError()
{
	if (strip_info == NULL)
	{
		ChangeNeoPixels_info();
	}
	if (strip_info == NULL)
		return;
	infoLight(red);	  // Set to red first
	blinkLed(3, 100); // Three fast blinks
	delay(200);		  // Brief pause before returning to idle
	infoLedIdle();	  // Return to idle state (dim blue)
}

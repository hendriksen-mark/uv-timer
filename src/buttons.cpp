#include "buttons.h"
#include "globals.h"
#include <DebugLog.h>

void initButtons()
{
  button1.begin(PIN_BTN1, INPUT_PULLUP, true);
  button2.begin(PIN_BTN2, INPUT_PULLUP, true);
  button3.begin(PIN_BTN3, INPUT_PULLUP, true);

  button1.setDebounceTime(APP_BTN_DEBOUNCE_MS);
  button2.setDebounceTime(APP_BTN_DEBOUNCE_MS);
  button3.setDebounceTime(APP_BTN_DEBOUNCE_MS);

  button1.setDoubleClickTime(APP_BTN_DOUBLE_MS);
  button2.setDoubleClickTime(APP_BTN_DOUBLE_MS);
  button3.setDoubleClickTime(APP_BTN_DOUBLE_MS);

  button1.setLongClickTime(APP_BTN_LONG_MS);
  button2.setLongClickTime(APP_BTN_LONG_MS);
  button3.setLongClickTime(APP_BTN_LONG_MS);

  LOG_DEBUG(F("buttons init"), F("b1"), PIN_BTN1, F("b2"), PIN_BTN2, F("b3"), PIN_BTN3);
}

static unsigned long holdStart1 = 0;
static unsigned long holdStart2 = 0;
static unsigned long holdStart3 = 0;
static bool holdReported1 = false;
static bool holdReported2 = false;
static bool holdReported3 = false;
static unsigned long holdRepeat1 = 0;
static unsigned long holdRepeat2 = 0;
static unsigned long holdRepeat3 = 0;

static unsigned long &holdStartFor(Button2 &btn)
{
  if (&btn == &button1)
  {
    return holdStart1;
  }
  if (&btn == &button2)
  {
    return holdStart2;
  }
  return holdStart3;
}

static bool &holdReportedFor(Button2 &btn)
{
  if (&btn == &button1)
  {
    return holdReported1;
  }
  if (&btn == &button2)
  {
    return holdReported2;
  }
  return holdReported3;
}

static unsigned long &holdRepeatFor(Button2 &btn)
{
  if (&btn == &button1)
  {
    return holdRepeat1;
  }
  if (&btn == &button2)
  {
    return holdRepeat2;
  }
  return holdRepeat3;
}

void serviceButtons()
{
  button1.loop();
  button2.loop();
  button3.loop();
}

static bool buttonEventMatches(Button2 &btn, clickType expected)
{
  return buttonEvent(btn) == expected;
}

clickType buttonEvent(Button2 &btn)
{
  serviceButtons();
  if (!btn.wasPressed())
  {
    return empty;
  }
  return btn.read();
}

bool buttonPressed(Button2 &btn)
{
  return buttonEventMatches(btn, single_click);
}

bool buttonDoublePressed(Button2 &btn)
{
  return buttonEventMatches(btn, double_click);
}

bool buttonLongPressed(Button2 &btn)
{
  serviceButtons();

  unsigned long &holdStart = holdStartFor(btn);
  bool &holdReported = holdReportedFor(btn);
  unsigned long &holdRepeat = holdRepeatFor(btn);

  if (!btn.isPressed())
  {
    holdStart = 0;
    holdReported = false;
    holdRepeat = 0;
    return false;
  }

  unsigned long nowMs = millis();
  if (holdStart == 0)
  {
    holdStart = nowMs;
    return false;
  }

  if (!holdReported && nowMs - holdStart >= btn.getLongClickTime())
  {
    holdReported = true;
    holdRepeat = nowMs;
    return true;
  }

  unsigned long repeatIntervalMs = APP_BTN_HOLD_REPEAT_MS;
  unsigned long fastModeThresholdMs = static_cast<unsigned long>(btn.getLongClickTime()) + APP_BTN_HOLD_ACCEL_AFTER_MS;
  if (holdReported && nowMs - holdStart >= fastModeThresholdMs)
  {
    repeatIntervalMs = APP_BTN_HOLD_REPEAT_FAST_MS;
  }

  if (holdReported && nowMs - holdRepeat >= repeatIntervalMs)
  {
    holdRepeat = nowMs;
    return true;
  }

  return false;
}

bool buttonWasPressedAndConsume(Button2 &btn)
{
  serviceButtons();
  if (!btn.wasPressed())
  {
    return false;
  }
  btn.read();
  return true;
}

bool anyButtonPressedAndConsume()
{
  return buttonWasPressedAndConsume(button1) ||
         buttonWasPressedAndConsume(button2) ||
         buttonWasPressedAndConsume(button3);
}

bool waitForAnyButtonPress()
{
  // Wait for all buttons to be released so the previous gesture does not
  // immediately dismiss the next "Press any key" screen.
  while (digitalRead(PIN_BTN1) == LOW || digitalRead(PIN_BTN2) == LOW || digitalRead(PIN_BTN3) == LOW)
  {
    serviceButtons();
  }

  // Flush any queued events from the action that led into the waiting screen.
  button1.read();
  button2.read();
  button3.read();

  while (true)
  {
    if (anyButtonPressedAndConsume())
    {
      return true;
    }
  }
}

#include "globals.h"
#include <Wire.h>
#include <DebugLog.h>
#include <hardware/watchdog.h>
#include "buttons.h"
#include "lcd.h"
#include "timers.h"
#include "controls.h"
#include "settings_store.h"
#include "functions.h"

void setup()
{
  loadSettings();
  Serial.begin(APP_DEBUG_BAUD);

  // Initialize LCD before serial wait so boot status can be shown.
  Wire.setSDA(PIN_LCD_SDA);
  Wire.setSCL(PIN_LCD_SCL);
  Wire.begin();
  lcd.init();
  lcd.clear();
  writeBuzzer(false);

  serialWait();
  LOG_ATTACH_SERIAL(Serial);

  lcd.clear();

  // Now set log level based on loaded setting
  LOG_SET_LEVEL(debugLogLevel == 0 ? DebugLogLevel::LVL_INFO : DebugLogLevel::LVL_DEBUG);
  LOG_DEBUG(F("setup start"));

  initButtons();

  pinMode(PIN_UV_MOSFET_1, OUTPUT);
  pinMode(PIN_UV_MOSFET_2, OUTPUT);
  pinMode(PIN_WHITE_MOSFET, OUTPUT);

  outputsIdle();

  LOG_DEBUG(F("settings loaded"), F("time"), timeMin, ':', timeSec, F("singleDouble"), singleDouble,
            F("doubleSideEnabled"), doubleSideEnabled, F("whitePwm"), whitePwm);
  outputsIdle();
  setupTimer1();
  serviceButtons();

  if (!skipStartup)
  {
    showStartupMessage();
  }

  // Hidden menu entry is a startup hold gesture on B1.
  // With I2C buttons, watch for one long-press event during a short startup window.
  bool enterHiddenMenu = false;
  unsigned long startupCheckBeginMs = millis();
  while (millis() - startupCheckBeginMs < 1600)
  {
    if (buttonLongPressed(button1))
    {
      enterHiddenMenu = true;
      break;
    }
    if (buttonPressed(button3))
    {
      factoryResetSettings();
      break; // any other button press cancels hidden menu entry
    }
    delay(10);
  }

  if (enterHiddenMenu)
  {
    LOG_WARN(F("startup hidden menu requested"));
    hiddenMenu();
    outputsIdle();
    LOG_DEBUG(F("hidden menu finished during setup"));
    return;
  }

  LOG_DEBUG(F("setup complete"));
}

void loop()
{
  serviceButtons();
  drawStartScreen();

  clickType event1 = buttonEvent(button1);
  if (event1 == single_click)
  {
    LOG_DEBUG(F("start exposure cycle"));
    runTimerCycle();
    return;
  }

  if (event1 == double_click)
  {
    LOG_DEBUG(F("reboot requested via double click"));
    watchdog_reboot(0, 0, 10);
    delay(25);

    // On real hardware, execution should not continue past this point.
    // In simulators where watchdog reset is not applied, recover with soft restart.
    LOG_WARN(F("reboot not applied by platform, using soft restart"));
    setup();
    return;
  }

  if (event1 == long_click)
  {
    LOG_WARN(F("bootloader requested via long click"));
    rp2040.rebootToBootloader();
    return;
  }

  clickType event2 = buttonEvent(button2);
  if (event2 == single_click)
  {
    if (doubleSideEnabled)
    {
      singleDouble = singleDouble ? 0 : 1;
      saveSettingsToJson();
      LOG_DEBUG(F("mode toggled"), singleDouble ? F("double-sided") : F("single-sided"));
    }
    return;
  }

  if (event2 == double_click)
  {
    LOG_DEBUG(F("enter test strip mode"));
    runTestStripMode();
    return;
  }

  clickType event3 = buttonEvent(button3);
  if (event3 == single_click)
  {
    LOG_DEBUG(F("enter time edit menu"));
    editTimeMenu();
    return;
  }

  if (event3 == double_click)
  {
    LOG_DEBUG(F("show help screen"));
    showHelpScreen();
    return;
  }

  if (event3 == long_click)
  {
    LOG_DEBUG(F("enter hidden menu"));
    hiddenMenu();
    outputsIdle();
    LOG_DEBUG(F("hidden menu finished during main loop"));
    return;
  }
}

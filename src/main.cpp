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
  Serial.begin(APP_DEBUG_BAUD);
  LOG_ATTACH_SERIAL(Serial);
  loadSettings();
  // Now set log level based on loaded setting
  LOG_SET_LEVEL(debugLogLevel == 0 ? DebugLogLevel::LVL_INFO : DebugLogLevel::LVL_DEBUG);
  LOG_DEBUG(F("setup start"));
  LOG_DEBUG(F("settings loaded"), F("time"), timeMin, ':', timeSec, F("singleDouble"), singleDouble,
            F("doubleSideEnabled"), doubleSideEnabled, F("whitePwm"), whitePwm);

  // Initialize LCD before serial wait so boot status can be shown.
  Wire.setSDA(PIN_LCD_SDA);
  Wire.setSCL(PIN_LCD_SCL);
  lcd.begin();
  lcd.clear();
  writeBuzzer(false);

  serialWait();

  initButtons();

  pinMode(PIN_UV_MOSFET_1, OUTPUT);
  pinMode(PIN_UV_MOSFET_2, OUTPUT);
  pinMode(PIN_WHITE_MOSFET, OUTPUT);

  outputsIdle();
  setupTimer1();
  serviceButtons();

  if (!skipStartup)
  {
    showStartupMessage();
  }

  // Hidden menu entry is a startup hold gesture on B1.
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


  // ============================================================================
  // Button 1 gestures for starting timer, reboot, and bootloader
  // ============================================================================
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
    lcd.setCursor(0, 0);
    lcd.print("Enter bootloader");
    lcd.setCursor(0, 1);
    lcd.print("mode and wait...     ");
    rp2040.rebootToBootloader();
    return;
  }

  // ============================================================================
  // Button 2 gestures for mode toggle and test strip mode
  // ============================================================================
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

  // ============================================================================
  // Button 3 gestures for time edit menu, help screen, and hidden menu
  // ============================================================================
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

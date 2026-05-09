#include "controls.h"
#include <DebugLog.h>
#include "globals.h"
#include "buttons.h"
#include "lcd.h"
#include "timers.h"
#include "settings_store.h"

static inline void writeUv1(bool on)
{
  uint8_t level = on ? (uvActiveHigh ? HIGH : LOW) : (uvActiveHigh ? LOW : HIGH);
  digitalWrite(PIN_UV_MOSFET_1, level);
}

static inline void writeUv2(bool on)
{
  uint8_t level = on ? (uvActiveHigh ? HIGH : LOW) : (uvActiveHigh ? LOW : HIGH);
  digitalWrite(PIN_UV_MOSFET_2, level);
}

static inline void writeWhite(bool on)
{
  if (!on)
  {
    digitalWrite(PIN_WHITE_MOSFET, whiteActiveHigh ? LOW : HIGH);
    return;
  }

  analogWrite(PIN_WHITE_MOSFET, whiteActiveHigh ? whitePwm : static_cast<uint8_t>(255 - whitePwm));
}

void writeBuzzer(bool on)
{
  bool driveActive = buzzerActiveHigh ? on : !on;

  LOG_DEBUG(on ? F("buzzer on") : F("buzzer off"));

  if (driveActive)
  {
    lcd.backlight();
  }
  else
  {
    lcd.noBacklight();
  }
}

static void beepPulse(uint16_t onMs, uint16_t offMs = 0)
{
  writeBuzzer(true);
  delay(onMs);
  writeBuzzer(false);
  if (offMs > 0)
  {
    delay(offMs);
  }
}

static void beepStep()
{
  if (buzzerMode == 1)
  {
    beepPulse(70);
  }
  if (buzzerMode == 2)
  {
    beepPulse(55, 65);
    beepPulse(55);
  }
  // buzzerMode == 0
  writeBuzzer(false);
}

bool confirmAction(const char *title)
{
  lcd.clear();
  lcd.printCenter(title, 0);
  lcd.printCenter("Yes           No", 1);

  while (true)
  {
    if (buttonPressed(button1))
    {
      return true;
    }
    if (buttonPressed(button3))
    {
      return false;
    }
  }
}

static void runManualOutputsMenu()
{
  bool uv1On = false;
  bool uv2On = false;
  bool whiteOn = false;
  bool buzzerOn = false;
  uint8_t selected = 0;
  bool redraw = true;

  writeUv1(false);
  writeUv2(false);
  writeWhite(false);
  writeBuzzer(false);

  while (true)
  {
    if (redraw)
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("Manual outputs  "));

      const char *name = "UV1";
      bool state = uv1On;
      if (selected == 1)
      {
        name = "UV2";
        state = uv2On;
      }
      else if (selected == 2)
      {
        name = "WHT";
        state = whiteOn;
      }
      else if (selected == 3)
      {
        name = "BZR";
        state = buzzerOn;
      }

      char line[17];
      snprintf(line, sizeof(line), "%s %s Tog Next", name, state ? "On " : "Off");
      lcd.setCursor(0, 1);
      lcd.print(line);
      redraw = false;
    }

    if (buttonPressed(button1))
    {
      selected = static_cast<uint8_t>((selected + 3) % 4);
      redraw = true;
      continue;
    }

    if (buttonPressed(button2))
    {
      if (selected == 0)
      {
        uv1On = !uv1On;
        writeUv1(uv1On);
      }
      else if (selected == 1)
      {
        uv2On = !uv2On;
        writeUv2(uv2On);
      }
      else if (selected == 2)
      {
        whiteOn = !whiteOn;
        writeWhite(whiteOn);
      }
      else
      {
        buzzerOn = !buzzerOn;
        writeBuzzer(buzzerOn);
      }
      redraw = true;
      continue;
    }

    clickType b3Event = buttonEvent(button3);
    if (b3Event == single_click)
    {
      selected = static_cast<uint8_t>((selected + 1) % 4);
      redraw = true;
      continue;
    }
    if (b3Event == double_click)
    {
      writeUv1(false);
      writeUv2(false);
      writeWhite(false);
      writeBuzzer(false);
      outputsIdle();
      return;
    }
  }
}

static bool startCountdown(uint8_t startMin, uint8_t startSec)
{
  if (startMin == 0 && startSec == 0)
  {
    LOG_WARN(F("countdown refused because time is 00:00"));
    lcd.clear();
    lcd.printCenter("Set time first", 0);
    lcd.printCenter("Press Any Key.", 1);
    waitForAnyButtonPress();
    lcd.clear();
    return false;
  }

  noInterrupts();
  countMin = startMin;
  countSec = startSec;
  msAccumulator = 0;
  timerGo = true;
  interrupts();
  LOG_INFO(F("countdown armed"), startMin, ':', startSec);
  return true;
}

static bool runStripExposureStep(uint8_t stepIndex, uint8_t stepCount, uint8_t stepMin, uint8_t stepSec)
{
  if (!startCountdown(stepMin, stepSec))
  {
    return false;
  }

  LOG_DEBUG(F("test strip step start"), stepIndex, '/', stepCount, F("stepTime"), stepMin, ':', stepSec);
  outputsUvOnForMode();

  while (true)
  {
    uint8_t cm;
    uint8_t cs;
    bool active;
    noInterrupts();
    cm = countMin;
    cs = countSec;
    active = timerGo;
    interrupts();

    lcd.setCursor(0, 0);
    lcd.print("Strip ");
    lcd.print(stepIndex);
    lcd.print('/');
    lcd.print(stepCount);
    lcd.print("        ");
    lcd.setCursor(0, 1);
    lcdPrintTime(cm, cs);
    lcd.print(" Hold=Stop ");

    if (!active)
    {
      break;
    }

    if (buttonLongPressed(button3))
    {
      LOG_WARN(F("test strip aborted by user during step"), stepIndex);
      noInterrupts();
      timerGo = false;
      interrupts();
      outputsIdle();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.printCenter("Strip aborted", 0);
      lcd.printCenter("Press Any Key.", 1);
      waitForAnyButtonPress();
      lcd.clear();
      return false;
    }

    delay(120);
  }

  outputsIdle();
  LOG_DEBUG(F("test strip step complete"), stepIndex, '/', stepCount);
  return true;
}

static bool editBlinkingTime(const char *title, uint8_t valueCol, uint8_t &mm, uint8_t &ss)
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(title);
  lcd.setCursor(0, 1);
  lcd.print("-      +    Next");

  auto selectedCol = [&](uint8_t pos) -> uint8_t
  {
    if (pos == 0)
    {
      return valueCol;
    }
    if (pos == 1)
    {
      return static_cast<uint8_t>(valueCol + 1);
    }
    if (pos == 2)
    {
      return static_cast<uint8_t>(valueCol + 3);
    }
    return static_cast<uint8_t>(valueCol + 4);
  };

  auto redraw = [&](uint8_t pos, bool showDigit)
  {
    lcd.setCursor(valueCol, 0);
    lcdPrintTime(mm, ss);

    if (!showDigit)
    {
      lcd.setCursor(selectedCol(pos), 0);
      lcd.print(' ');
    }
  };

  for (uint8_t pos = 0; pos < 4; ++pos)
  {
    bool showDigit = true;
    bool redrawNeeded = true;
    unsigned long lastBlinkMs = millis();

    while (true)
    {
      unsigned long nowMs = millis();
      if (nowMs - lastBlinkMs >= 350)
      {
        showDigit = !showDigit;
        lastBlinkMs = nowMs;
        redrawNeeded = true;
      }

      if (redrawNeeded)
      {
        redraw(pos, showDigit);
        redrawNeeded = false;
      }

      clickType b3Event = buttonEvent(button3);
      if (b3Event == double_click)
      {
        return false;
      }

      bool valueChanged = false;
      if (buttonPressed(button1))
      {
        uint8_t prevMm = mm;
        uint8_t prevSs = ss;
        if (pos == 0 && mm >= 10)
        {
          mm -= 10;
        }
        else if (pos == 1 && mm >= 1)
        {
          --mm;
        }
        else if (pos == 2 && ss >= 10)
        {
          ss -= 10;
        }
        else if (pos == 3 && ss >= 1)
        {
          --ss;
        }

        valueChanged = (mm != prevMm) || (ss != prevSs);
      }

      if (buttonPressed(button2))
      {
        uint8_t prevMm = mm;
        uint8_t prevSs = ss;
        if (pos == 0 && mm <= 89)
        {
          mm += 10;
        }
        else if (pos == 1 && mm <= 98)
        {
          ++mm;
        }
        else if (pos == 2 && ss <= 49)
        {
          ss += 10;
        }
        else if (pos == 3 && ss <= 58)
        {
          ++ss;
        }

        valueChanged = valueChanged || (mm != prevMm) || (ss != prevSs);
      }

      if (valueChanged)
      {
        showDigit = true;
        lastBlinkMs = nowMs;
        redrawNeeded = true;
      }

      if (b3Event == single_click)
      {
        break;
      }
    }
  }

  return true;
}

void outputsIdle()
{
  writeUv1(false);
  writeUv2(false);
  writeWhite(true);
}

void inspectionLight()
{
  LOG_DEBUG(F("inspection light on"));

  // Temporarily drive white at full brightness regardless of whitePwm setting.
  uint8_t savedPwm = whitePwm;
  whitePwm = 255;
  writeWhite(true);
  whitePwm = savedPwm;

  lcd.clear();
  lcd.printCenter("Inspect mode", 0);
  lcd.printCenter("Press Any Key.", 1);

  waitForAnyButtonPress();

  writeWhite(true); // restore normal whitePwm brightness
  lcd.clear();
  LOG_DEBUG(F("inspection light off"));
}

void outputsUvOnForMode()
{
  writeUv1(false);
  writeUv2(false);
  writeWhite(false);

  if (singleDouble == 0)
  {
    if (topBottom == 0)
    {
      writeUv1(true);
    }
    else
    {
      writeUv2(true);
    }
  }
  else
  {
    writeUv1(true);
    writeUv2(true);
  }
}

void showHelpScreen()
{
  LOG_DEBUG(F("help screen open"));
  uint8_t page = 0;
  bool redraw = true;
  while (true)
  {
    if (redraw)
    {
      lcd.clear();
      if (page == 0)
      {
        lcd.setCursor(0, 0);
        lcd.print("Boot:Hold B1:Cfg");
        lcd.setCursor(0, 1);
        lcd.print("Hold B2:Factory ");
      }
      else if (page == 1)
      {
        lcd.setCursor(0, 0);
        lcd.print("Home: B1:Start ");
        lcd.setCursor(0, 1);
        lcd.print("B2:Mode B3:SetT");
      }
      else if (page == 2)
      {

        lcd.setCursor(0, 0);
        lcd.print("B1x2:Reboot     ");
        lcd.setCursor(0, 1);
        lcd.print("Hold B1:Bootldr  ");
      }
      else if (page == 3)
      {
        lcd.setCursor(0, 0);
        lcd.print("B2x2:Test Strip");
        lcd.setCursor(0, 1);
        lcd.print("Hold B2:Inspect ");
      }
      else if (page == 4)
      {
        lcd.setCursor(0, 0);
        lcd.print("B3x2:Help/Exit ");
        lcd.setCursor(0, 1);
        lcd.print("Hold B3:HidnMenu ");
      }
      else
      {
        lcd.setCursor(0, 0);
        lcd.print("Run:Hold B3:Stop");
        lcd.setCursor(0, 1);
        lcd.print("Stopped:B3x2:Ext");
      }
      redraw = false;
    }

    if (buttonDoublePressed(button3))
    {
      LOG_DEBUG(F("help screen close"));
      lcd.clear();
      return;
    }
    if (buttonPressed(button2))
    {
      page = static_cast<uint8_t>((page + 1) % 6);
      LOG_DEBUG(F("help page"), page);
      redraw = true;
    }
  }
}

bool beepDone()
{
  if (buzzerMode == 1)
  {
    beepPulse(180);
  }
  else if (buzzerMode == 2)
  {
    beepPulse(200, 250);
    beepPulse(200);
  }
  else if (buzzerMode == 3)
  {
    // Alarm mode: repeat beep cycle until any button is pressed.
    while (true)
    {
      beepPulse(200, 150);
      beepPulse(200, 300);
      serviceButtons();
      if (button1.wasPressed() || button2.wasPressed() || button3.wasPressed())
      {
        break;
      }
    }
    writeBuzzer(false);
    return true; // button press already consumed
  }
  writeBuzzer(false);
  return false;
}

void hiddenMenu()
{
  LOG_INFO(F("hidden menu open"));
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("- Hidden Menu - ");
  lcd.setCursor(0, 1);
  lcd.print("-/+ then Next   ");
  delay(2000);

  uint8_t page = 0;
  bool redraw = true;
  bool dirty = false;

  while (true)
  {
    if (redraw)
    {
      lcd.clear();
      switch (page)
      {
      case 0:
        lcd.setCursor(0, 0);
        lcd.print("Skip startup?   ");
        lcd.setCursor(0, 1);
        lcd.print(skipStartup ? ">Yes<  No   Next" : " Yes  >No<  Next");
        break;
      case 1:
        lcd.setCursor(0, 0);
        lcd.print("Timing adjust   ");
        lcd.setCursor(0, 1);
        lcd.print("Value:      Next");
        break;
      case 2:
        lcd.setCursor(0, 0);
        lcd.print("Double side?    ");
        lcd.setCursor(0, 1);
        lcd.print(doubleSideEnabled ? ">Yes<  No   Next" : " Yes  >No<  Next");
        break;
      case 3:
        lcd.setCursor(0, 0);
        lcd.print("Single side:    ");
        lcd.setCursor(0, 1);
        lcd.print(topBottom == 1 ? ">Top<  Bot  Next" : " Top  >Bot< Next");
        break;
      case 4:
        lcd.setCursor(0, 0);
        lcd.print("UV active high? ");
        lcd.setCursor(0, 1);
        lcd.print(uvActiveHigh ? ">Yes<  No   Next" : " Yes  >No<  Next");
        break;
      case 5:
        lcd.setCursor(0, 0);
        lcd.print("White act high? ");
        lcd.setCursor(0, 1);
        lcd.print(whiteActiveHigh ? ">Yes<  No   Next" : " Yes  >No<  Next");
        break;
      case 6:
        lcd.setCursor(0, 0);
        lcd.print("White PWM:      ");
        lcd.setCursor(0, 1);
        lcd.print("-      +    Next");
        break;
      case 7:
        lcd.setCursor(0, 0);
        lcd.print(F("Buzz mode       "));
        lcd.setCursor(0, 1);
        lcd.print("-      +    Next");
        break;
      case 8:
        lcd.setCursor(0, 0);
        lcd.print(F("Buzz act high?  "));
        lcd.setCursor(0, 1);
        lcd.print(buzzerActiveHigh ? ">Yes<  No   Next" : " Yes  >No<  Next");
        break;
      case 9:
        lcd.setCursor(0, 0);
        lcd.print(F("Debug lvl       "));
        lcd.setCursor(0, 1);
        lcd.print("-      +    Next");
        break;
      case 10:
        lcd.setCursor(0, 0);
        lcd.print(F("Serial wait?    "));
        lcd.setCursor(0, 1);
        lcd.print(serialWaitEnabled ? ">Yes<  No   Next" : " Yes  >No<  Next");
        break;
      default:
        lcd.setCursor(0, 0);
        lcd.print(F("Manual toggles  "));
        lcd.setCursor(0, 1);
        lcd.print(F("     Enter  Exit"));
        break;
      }
      redraw = false;
    }

    if (page == 1)
    {
      char v[4];
      snprintf(v, sizeof(v), "%03u", calByte);
      lcd.setCursor(7, 1);
      lcd.print(v);
    }
    else if (page == 6)
    {
      char w[4];
      snprintf(w, sizeof(w), "%03u", whitePwm);
      lcd.setCursor(10, 0);
      lcd.print(w);
    }
    else if (page == 7)
    {
      const char *mode = "Off   ";
      if (buzzerMode == 1)
      {
        mode = "Short ";
      }
      else if (buzzerMode == 2)
      {
        mode = "Double";
      }
      else if (buzzerMode == 3)
      {
        mode = "Alarm ";
      }
      lcd.setCursor(10, 0);
      lcd.print(mode);
    }
    else if (page == 8)
    {
      // buzzerActiveHigh page — indicator is in the static label, no dynamic value
    }
    else if (page == 9)
    {
      const char *dbgMode = debugLogLevel == 0 ? "Info  " : "Debug ";
      lcd.setCursor(11, 0);
      lcd.print(dbgMode);
    }

    clickType b3Event = buttonEvent(button3);
    if (b3Event == double_click)
    {
      if (page > 0)
      {
        --page;
        redraw = true;
      }
      continue;
    }

    switch (page)
    {
    case 0:
      if (buttonPressed(button1))
      {
        skipStartup = true;
        dirty = true;
        LOG_DEBUG(F("startup disabled"));
        redraw = true;
      }
      if (buttonPressed(button2))
      {
        skipStartup = false;
        dirty = true;
        LOG_DEBUG(F("startup enabled"));
        redraw = true;
      }
      if (b3Event == single_click)
      {
        ++page;
        redraw = true;
      }
      break;

    case 1:
      if (buttonPressed(button1))
      {
        if (calByte > 0)
        {
          --calByte;
        }
        dirty = true;
        LOG_DEBUG(F("timing calibration decreased"), calByte);
      }
      if (buttonPressed(button2))
      {
        if (calByte < 255)
        {
          ++calByte;
        }
        dirty = true;
        LOG_DEBUG(F("timing calibration increased"), calByte);
      }
      if (b3Event == single_click)
      {
        ++page;
        redraw = true;
      }
      break;

    case 2:
      if (buttonPressed(button1))
      {
        doubleSideEnabled = true;
        dirty = true;
        LOG_DEBUG(F("double side enabled"));
        redraw = true;
      }
      if (buttonPressed(button2))
      {
        doubleSideEnabled = false;
        singleDouble = 0;
        dirty = true;
        LOG_DEBUG(F("double side disabled"));
        redraw = true;
      }
      if (b3Event == single_click)
      {
        ++page;
        redraw = true;
      }
      break;

    case 3:
      if (buttonPressed(button1))
      {
        topBottom = 1;
        dirty = true;
        LOG_DEBUG(F("single side target set"), F("top"));
        redraw = true;
      }
      if (buttonPressed(button2))
      {
        topBottom = 0;
        dirty = true;
        LOG_DEBUG(F("single side target set"), F("bottom"));
        redraw = true;
      }
      if (b3Event == single_click)
      {
        ++page;
        redraw = true;
      }
      break;

    case 4:
      if (buttonPressed(button1))
      {
        uvActiveHigh = true;
        dirty = true;
        outputsIdle();
        LOG_DEBUG(F("uv polarity set"), F("active-high"));
        redraw = true;
      }
      if (buttonPressed(button2))
      {
        uvActiveHigh = false;
        dirty = true;
        outputsIdle();
        LOG_DEBUG(F("uv polarity set"), F("active-low"));
        redraw = true;
      }
      if (b3Event == single_click)
      {
        ++page;
        redraw = true;
      }
      break;

    case 5:
      if (buttonPressed(button1))
      {
        whiteActiveHigh = true;
        dirty = true;
        outputsIdle();
        LOG_DEBUG(F("white polarity set"), F("active-high"));
        redraw = true;
      }
      if (buttonPressed(button2))
      {
        whiteActiveHigh = false;
        dirty = true;
        outputsIdle();
        LOG_DEBUG(F("white polarity set"), F("active-low"));
        redraw = true;
      }
      if (b3Event == single_click)
      {
        ++page;
        redraw = true;
      }
      break;

    case 6:
      if (buttonPressed(button1))
      {
        if (whitePwm > 0)
        {
          --whitePwm;
        }
        dirty = true;
        writeWhite(true);
        LOG_DEBUG(F("white pwm decreased"), whitePwm);
      }
      if (buttonPressed(button2))
      {
        if (whitePwm < 255)
        {
          ++whitePwm;
        }
        dirty = true;
        writeWhite(true);
        LOG_DEBUG(F("white pwm increased"), whitePwm);
      }
      if (b3Event == single_click)
      {
        ++page;
        redraw = true;
      }
      break;

    case 7:
      if (buttonPressed(button1))
      {
        buzzerMode = static_cast<uint8_t>((buzzerMode + 3) % 4);
        dirty = true;
        LOG_DEBUG(F("buzzer mode changed"), buzzerMode);
      }
      if (buttonPressed(button2))
      {
        buzzerMode = static_cast<uint8_t>((buzzerMode + 1) % 4);
        dirty = true;
        LOG_DEBUG(F("buzzer mode changed"), buzzerMode);
      }
      if (b3Event == single_click)
      {
        ++page;
        redraw = true;
      }
      break;

    case 8:
      if (buttonPressed(button1))
      {
        buzzerActiveHigh = true;
        dirty = true;
        writeBuzzer(false);
        LOG_DEBUG(F("buzzer polarity set"), F("active-high"));
        redraw = true;
      }
      if (buttonPressed(button2))
      {
        buzzerActiveHigh = false;
        dirty = true;
        writeBuzzer(false);
        LOG_DEBUG(F("buzzer polarity set"), F("active-low"));
        redraw = true;
      }
      if (b3Event == single_click)
      {
        ++page;
        redraw = true;
      }
      break;

    case 9:
      if (buttonPressed(button1))
      {
        debugLogLevel = debugLogLevel == 0 ? 1 : 0;
        dirty = true;
        LOG_DEBUG(F("debug level changed"), debugLogLevel ? F("DEBUG") : F("INFO"));
      }
      if (buttonPressed(button2))
      {
        debugLogLevel = debugLogLevel == 0 ? 1 : 0;
        dirty = true;
        LOG_DEBUG(F("debug level changed"), debugLogLevel ? F("DEBUG") : F("INFO"));
      }
      if (b3Event == single_click)
      {
        ++page;
        redraw = true;
      }
      break;

    case 10:
      if (buttonPressed(button1))
      {
        serialWaitEnabled = true;
        dirty = true;
        LOG_DEBUG(F("serial wait enabled"));
        redraw = true;
      }
      if (buttonPressed(button2))
      {
        serialWaitEnabled = false;
        dirty = true;
        LOG_DEBUG(F("serial wait disabled"));
        redraw = true;
      }
      if (b3Event == single_click)
      {
        ++page;
        redraw = true;
      }
      break;

    default:
      if (buttonPressed(button2))
      {
        runManualOutputsMenu();
        redraw = true;
      }
      if (b3Event == single_click)
      {
        if (dirty)
        {
          saveSettingsToJson();
        }
        updateTickFromCalibration();
        LOG_SET_LEVEL(debugLogLevel == 0 ? DebugLogLevel::LVL_INFO : DebugLogLevel::LVL_DEBUG);
        lcd.clear();
        LOG_INFO(F("hidden menu close"));
        return;
      }
      break;
    }
  }
}

void editTimeMenu()
{
  uint8_t mm = timeMin;
  uint8_t ss = timeSec;

  if (!editBlinkingTime("Set Time:       ", 10, mm, ss))
  {
    lcd.clear();
    return;
  }

  timeMin = mm;
  timeSec = ss;
  saveSettingsToJson();
  LOG_DEBUG(F("time updated"), timeMin, ':', timeSec);
  lcd.clear();
}

void runTestStripMode()
{
  LOG_INFO(F("test strip mode open"));
  uint8_t stepCount = stripStepCount;
  uint8_t stripMin = stripStepMin;
  uint8_t stripSec = stripStepSec;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Strip steps:    ");
  lcd.setCursor(0, 1);
  lcd.print("-      +    Next");

  while (true)
  {
    lcd.setCursor(13, 0);
    if (stepCount < 10)
    {
      lcd.print('0');
    }
    lcd.print(stepCount);

    clickType b3Event = buttonEvent(button3);
    if (b3Event == double_click)
    {
      LOG_INFO(F("test strip mode cancelled before start"));
      lcd.clear();
      return;
    }
    if (buttonPressed(button1))
    {
      if (stepCount > 2)
      {
        --stepCount;
        LOG_DEBUG(F("test strip step count decreased"), stepCount);
      }
      continue;
    }
    if (buttonPressed(button2))
    {
      if (stepCount < 9)
      {
        ++stepCount;
        LOG_DEBUG(F("test strip step count increased"), stepCount);
      }
      continue;
    }
    if (b3Event == single_click)
    {
      break;
    }
  }

  stripStepCount = stepCount;
  stripStepMin = stripMin;
  stripStepSec = stripSec;
  saveSettingsToJson();

  if (!editBlinkingTime("Strip time:     ", 11, stripMin, stripSec))
  {
    LOG_INFO(F("test strip mode cancelled at time edit"));
    lcd.clear();
    return;
  }

  stripStepCount = stepCount;
  stripStepMin = stripMin;
  stripStepSec = stripSec;
  saveSettingsToJson();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Strip time:");
  lcdPrintTime(stripMin, stripSec);
  lcd.setCursor(0, 1);
  lcd.print("Go    Edit   End");
  while (true)
  {
    if (buttonPressed(button1))
    {
      LOG_INFO(F("test strip mode started"), F("steps"), stepCount, F("stepTime"), stripMin, ':', stripSec);
      break;
    }
    if (buttonPressed(button2))
    {
      if (!editBlinkingTime("Strip time:     ", 11, stripMin, stripSec))
      {
        LOG_INFO(F("test strip mode cancelled at time edit"));
        lcd.clear();
        return;
      }
      stripStepCount = stepCount;
      stripStepMin = stripMin;
      stripStepSec = stripSec;
      saveSettingsToJson();

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Strip time:");
      lcdPrintTime(stripMin, stripSec);
      lcd.setCursor(0, 1);
      lcd.print("Go    Edit   End");
    }
    if (buttonDoublePressed(button3))
    {
      LOG_INFO(F("test strip mode cancelled at confirmation"));
      lcd.clear();
      return;
    }
  }

  for (uint8_t stepIndex = 1; stepIndex <= stepCount; ++stepIndex)
  {
    if (!runStripExposureStep(stepIndex, stepCount, stripMin, stripSec))
    {
      return;
    }

    if (stepIndex == stepCount)
    {
      break;
    }

    beepStep();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Move mask       ");
    lcd.setCursor(0, 1);
    lcd.print("      Next   End");

    while (true)
    {
      if (buttonPressed(button2))
      {
        LOG_DEBUG(F("test strip continue to next step"));
        break;
      }
      if (buttonDoublePressed(button3))
      {
        LOG_DEBUG(F("test strip ended early between steps"));
        lcd.clear();
        return;
      }
    }
  }

  lcd.clear();
  lcd.printCenter("Strip done", 0);
  lcd.printCenter("Press Any Key.", 1);
  if (!beepDone())
  {
    waitForAnyButtonPress();
  }
  LOG_DEBUG(F("test strip mode complete"));
  lcd.clear();
}

void runTimerCycle()
{
  LOG_INFO(F("exposure cycle begin"), timeMin, ':', timeSec, singleDouble ? F("D.S.") : F("S.S."));
  if (!startCountdown(timeMin, timeSec))
  {
    return;
  }

  while (true)
  {
    outputsIdle();
    outputsUvOnForMode();

    int8_t lastHeaderMode = -1;

    while (true)
    {
      uint8_t cm;
      uint8_t cs;
      bool active;
      noInterrupts();
      cm = countMin;
      cs = countSec;
      active = timerGo;
      interrupts();

      int8_t headerMode = doubleSideEnabled ? (singleDouble ? 2 : 1) : 0;
      if (headerMode != lastHeaderMode)
      {
        lcd.setCursor(0, 0);
        if (headerMode == 2)
        {
          lcd.print("Status:On (D.S.)");
        }
        else if (headerMode == 1)
        {
          lcd.print("Status:On (S.S.)");
        }
        else
        {
          lcd.print("Status:On       ");
        }
        lastHeaderMode = headerMode;
      }

      lcd.setCursor(0, 1);
      lcdPrintTime(cm, cs);
      lcd.print(" Hold=Stop ");

      if (!active)
      {
        break;
      }

      if (buttonLongPressed(button3))
      {
        LOG_WARN(F("exposure cycle stopped by user"));
        noInterrupts();
        timerGo = false;
        interrupts();
        break;
      }

      delay(120);
    }

    outputsIdle();

    uint8_t cm;
    uint8_t cs;
    noInterrupts();
    cm = countMin;
    cs = countSec;
    interrupts();

    if (cm == 0 && cs == 0)
    {
      break;
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Stopped at "));
    lcdPrintTime(cm, cs);
    lcd.setCursor(0, 1);
    lcd.print(F("Resume  Rst  End"));

    while (true)
    {
      if (buttonPressed(button1))
      {
        LOG_DEBUG(F("exposure cycle resumed"));
        noInterrupts();
        msAccumulator = 0;
        timerGo = true;
        interrupts();
        break;
      }
      if (buttonPressed(button2))
      {
        if (confirmAction("Restart full run?"))
        {
          LOG_DEBUG(F("exposure cycle restarted"));
          noInterrupts();
          countMin = timeMin;
          countSec = timeSec;
          msAccumulator = 0;
          timerGo = true;
          interrupts();
        }
        else
        {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print(F("Stopped at "));
          lcdPrintTime(cm, cs);
          lcd.setCursor(0, 1);
          lcd.print(F("Resume  Rst  End"));
          continue;
        }
        break;
      }
      if (buttonDoublePressed(button3))
      {
        if (confirmAction("End exposure?"))
        {
          LOG_INFO(F("exposure cycle cancelled after stop"));
          lcd.clear();
          return;
        }
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("Stopped at "));
        lcdPrintTime(cm, cs);
        lcd.setCursor(0, 1);
        lcd.print(F("Resume  Rst  End"));
      }
    }
  }

  lcd.clear();
  lcd.printCenter("DONE", 0);
  lcd.printCenter("Press Any Key.", 1);
  bool buttonConsumed = beepDone();
  LOG_INFO(F("exposure cycle complete"));
  if (!buttonConsumed)
  {
    waitForAnyButtonPress();
  }
  lcd.clear();
}

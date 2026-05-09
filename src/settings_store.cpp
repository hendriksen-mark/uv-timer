#include "settings_store.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "globals.h"
#include "timers.h"
#include "controls.h"
#include "DebugLog.h"

static bool g_fs_ready = false;

static void emitSettingsJson(const __FlashStringHelper *reason)
{
  JsonDocument doc;
  doc["singleDouble"] = singleDouble;
  doc["timeMin"] = timeMin;
  doc["timeSec"] = timeSec;
  doc["calByte"] = calByte;
  doc["skipStartup"] = skipStartup;
  doc["topBottom"] = topBottom;
  doc["doubleSideEnabled"] = doubleSideEnabled;
  doc["whitePwm"] = whitePwm;
  doc["uvActiveHigh"] = uvActiveHigh;
  doc["whiteActiveHigh"] = whiteActiveHigh;
  doc["buzzerActiveHigh"] = buzzerActiveHigh;
  doc["stripStepCount"] = stripStepCount;
  doc["stripStepMin"] = stripStepMin;
  doc["stripStepSec"] = stripStepSec;
  doc["buzzerMode"] = buzzerMode;
  doc["debugLogLevel"] = debugLogLevel;
  doc["serialWaitEnabled"] = serialWaitEnabled;

  String out;
  serializeJson(doc, out);
  LOG_INFO(F("settings JSON"), reason, out.c_str());
}

static void ensureFilesystem()
{
  if (g_fs_ready)
  {
    LOG_DEBUG(F("filesystem already ready"));
    return;
  }

  if (!LittleFS.begin())
  {
    LOG_WARN(F("LittleFS init failed"));
    return;
  }

  LOG_DEBUG(F("LittleFS initialized successfully"));
  g_fs_ready = true;
}

void factoryResetSettings()
{
  if (!confirmAction("Factory reset?"))
  {
    return;
  }

  if (LittleFS.exists(SETTINGS_FILE))
  {
    LittleFS.remove(SETTINGS_FILE);
  }
  loadSettings();
  LOG_INFO(F("factory reset complete"));
}

static void loadSettingsFromJson(const char *json_str)
{
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json_str);

  if (error)
  {
    LOG_WARN(F("JSON parse error"), error.code());
    return;
  }

  auto getU8 = [&](const char *key, uint8_t fallback) -> uint8_t
  {
    JsonVariantConst value = doc[key];
    return value.isNull() ? fallback : value.as<uint8_t>();
  };

  auto getBool = [&](const char *key, bool fallback) -> bool
  {
    JsonVariantConst value = doc[key];
    return value.isNull() ? fallback : value.as<bool>();
  };

  // Load with defaults if keys missing
  singleDouble = getU8("singleDouble", DEFAULT_SINGLE_DOUBLE);
  timeMin = getU8("timeMin", DEFAULT_TIME_MIN);
  timeSec = getU8("timeSec", DEFAULT_TIME_SEC);
  calByte = getU8("calByte", DEFAULT_CAL_BYTE);
  skipStartup = getBool("skipStartup", DEFAULT_SKIP_STARTUP);
  topBottom = getU8("topBottom", DEFAULT_TOP_BOTTOM);
  doubleSideEnabled = getBool("doubleSideEnabled", DEFAULT_DOUBLE_SIDE_ENABLED);
  whitePwm = getU8("whitePwm", DEFAULT_WHITE_PWM);
  uvActiveHigh = getBool("uvActiveHigh", DEFAULT_UV_ACTIVE_HIGH);
  whiteActiveHigh = getBool("whiteActiveHigh", DEFAULT_WHITE_ACTIVE_HIGH);
  buzzerActiveHigh = getBool("buzzerActiveHigh", DEFAULT_BUZZER_ACTIVE_HIGH);
  stripStepCount = getU8("stripStepCount", DEFAULT_TEST_STRIP_STEPS);
  stripStepMin = getU8("stripStepMin", DEFAULT_TEST_STRIP_MIN);
  stripStepSec = getU8("stripStepSec", DEFAULT_TEST_STRIP_SEC);
  buzzerMode = getU8("buzzerMode", DEFAULT_BUZZER_MODE);
  debugLogLevel = getU8("debugLogLevel", DEFAULT_DEBUG_LEVEL);
  serialWaitEnabled = getBool("serialWaitEnabled", DEFAULT_SERIAL_WAIT_ENABLED);

  // Validate ranges
  if (timeSec > 59)
  {
    timeSec = 59;
  }
  if (stripStepCount < 2 || stripStepCount > 9)
  {
    stripStepCount = DEFAULT_TEST_STRIP_STEPS;
  }
  if (stripStepMin > 99)
  {
    stripStepMin = 99;
  }
  if (stripStepSec > 59)
  {
    stripStepSec = 59;
  }
  if (buzzerMode > 3)
  {
    buzzerMode = DEFAULT_BUZZER_MODE;
  }
  if (debugLogLevel > 1)
  {
    debugLogLevel = DEFAULT_DEBUG_LEVEL;
  }
}

void saveSettingsToJson()
{
  ensureFilesystem();

  JsonDocument doc;
  doc["singleDouble"] = singleDouble;
  doc["timeMin"] = timeMin;
  doc["timeSec"] = timeSec;
  doc["calByte"] = calByte;
  doc["skipStartup"] = skipStartup;
  doc["topBottom"] = topBottom;
  doc["doubleSideEnabled"] = doubleSideEnabled;
  doc["whitePwm"] = whitePwm;
  doc["uvActiveHigh"] = uvActiveHigh;
  doc["whiteActiveHigh"] = whiteActiveHigh;
  doc["buzzerActiveHigh"] = buzzerActiveHigh;
  doc["stripStepCount"] = stripStepCount;
  doc["stripStepMin"] = stripStepMin;
  doc["stripStepSec"] = stripStepSec;
  doc["buzzerMode"] = buzzerMode;
  doc["debugLogLevel"] = debugLogLevel;
  doc["serialWaitEnabled"] = serialWaitEnabled;

  File f = LittleFS.open(SETTINGS_FILE, "w");
  if (!f)
  {
    LOG_WARN(F("settings file open failed"));
    return;
  }

  if (serializeJson(doc, f) == 0)
  {
    LOG_WARN(F("JSON serialize failed"));
  }

  f.close();
  emitSettingsJson(F("saved"));
}

void loadSettings()
{
  ensureFilesystem();

  // Try to read settings from JSON file
  if (LittleFS.exists(SETTINGS_FILE))
  {
    File f = LittleFS.open(SETTINGS_FILE, "r");
    if (f)
    {
      String json = f.readString();
      f.close();
      loadSettingsFromJson(json.c_str());
      LOG_DEBUG(F("settings loaded from JSON"));
      updateTickFromCalibration();
      emitSettingsJson(F("loaded"));
      return;
    }
  }

  // No file or read failed: use all defaults
  LOG_DEBUG(F("no settings file, using defaults"));
  singleDouble = DEFAULT_SINGLE_DOUBLE;
  timeMin = DEFAULT_TIME_MIN;
  timeSec = DEFAULT_TIME_SEC;
  calByte = DEFAULT_CAL_BYTE;
  skipStartup = DEFAULT_SKIP_STARTUP;
  topBottom = DEFAULT_TOP_BOTTOM;
  doubleSideEnabled = DEFAULT_DOUBLE_SIDE_ENABLED;
  whitePwm = DEFAULT_WHITE_PWM;
  uvActiveHigh = DEFAULT_UV_ACTIVE_HIGH;
  whiteActiveHigh = DEFAULT_WHITE_ACTIVE_HIGH;
  buzzerActiveHigh = DEFAULT_BUZZER_ACTIVE_HIGH;
  stripStepCount = DEFAULT_TEST_STRIP_STEPS;
  stripStepMin = DEFAULT_TEST_STRIP_MIN;
  stripStepSec = DEFAULT_TEST_STRIP_SEC;
  buzzerMode = DEFAULT_BUZZER_MODE;
  debugLogLevel = DEFAULT_DEBUG_LEVEL;
  serialWaitEnabled = DEFAULT_SERIAL_WAIT_ENABLED;
  updateTickFromCalibration();
  saveSettingsToJson();
  emitSettingsJson(F("defaults"));
}

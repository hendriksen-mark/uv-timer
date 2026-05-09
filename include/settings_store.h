#pragma once


// Load all settings from EEPROM (initializes if needed)
void loadSettings();

// Write all settings to JSON
void saveSettingsToJson();

// factory reset: clear settings file and reload defaults
void factoryResetSettings();

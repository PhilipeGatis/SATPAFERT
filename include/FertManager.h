#pragma once

#include "Config.h"
#include <Arduino.h>
#include <Preferences.h> // ESP32 NVS
#include <RTClib.h>

/// @brief Manages fertilizer dosing with NVS deduplication and stock tracking.
class FertManager {
public:
  FertManager();

  /// Initialize NVS and load saved state
  void begin();

  /// Check schedule and dose if needed. Returns true if dosing happened.
  /// @param now Current DateTime from TimeManager
  /// @param schedHour Scheduled hour (from WebManager)
  /// @param schedMinute Scheduled minute (from WebManager)
  bool checkAndDose(DateTime now, uint8_t schedHour, uint8_t schedMinute);

  /// Manually dose a specific channel
  /// @param ch Channel index 0-3 (fertilizers) or 4 (prime)
  /// @param ml Volume in mL
  /// @return true if dosing completed without timeout
  bool doseChannel(uint8_t ch, float ml);

  // ---- Dose volumes (set via WebManager) ----
  void setDoseML(uint8_t ch, float ml);
  float getDoseML(uint8_t ch) const;

  // ---- Stock tracking ----
  float getStockML(uint8_t ch) const;
  void setStockML(uint8_t ch, float ml);
  void resetStock(uint8_t ch, float ml);

  // ---- Custom Names (NVS) ----
  String getName(uint8_t ch) const;
  void setName(uint8_t ch, const String &name);

  /// Save stock levels and names to NVS
  void saveState();

  /// Was today's dose already applied?
  bool wasDosedToday(DateTime now) const;

private:
  Preferences _prefs;

  // Dose volumes per channel (CH1-CH4 ferts + CH5 prime)
  float _doseML[NUM_FERTS + 1];

  // Remaining stock per channel
  float _stockML[NUM_FERTS + 1];

  // Custom names per channel
  String _names[NUM_FERTS + 1];

  // Last dose date (day of year * 1000 + year) for dedup
  uint32_t _lastDoseKey;

  /// Compute unique key for a date (for NVS dedup)
  uint32_t _dateKey(DateTime dt) const;

  /// Load state from NVS
  void _loadState();

  /// Mark today as dosed in NVS
  void _markDosed(DateTime now);

  /// Get the GPIO pin for a channel (0-3 = fert, 4 = prime)
  uint8_t _pinForChannel(uint8_t ch) const;
};

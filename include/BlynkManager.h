#pragma once

#include "Config.h"
#include <Arduino.h>

// =============================================================================
// BLYNK CONFIGURATION — credentials come from platformio.ini build_flags
// =============================================================================
// BLYNK_TEMPLATE_ID, BLYNK_TEMPLATE_NAME, BLYNK_AUTH_TOKEN
// WIFI_SSID, WIFI_PASSWORD — all defined via -D flags in platformio.ini

// Uncomment for debug prints from Blynk library
// #define BLYNK_PRINT Serial

// =============================================================================
// VIRTUAL PIN MAP
// =============================================================================

// --- Telemetry (read-only) ---
#define VPIN_WATER_LEVEL V0  // Gauge: water level cm
#define VPIN_TPA_STATE V1    // Display: TPA state name
#define VPIN_SYSTEM_TIME V24 // Display: current time

// --- Schedule config (read/write) ---
#define VPIN_FERT_HOUR V2   // Slider 0–23
#define VPIN_FERT_MINUTE V3 // Slider 0–59
#define VPIN_TPA_DAY V4     // Slider 0–6
#define VPIN_TPA_HOUR V5    // Slider 0–23
#define VPIN_TPA_MINUTE V6  // Slider 0–59

// --- TPA controls (write) ---
#define VPIN_START_TPA V7   // Button (momentary)
#define VPIN_ABORT_TPA V8   // Button (momentary)
#define VPIN_MAINTENANCE V9 // Switch (toggle)

// --- Sensor status (read-only) ---
#define VPIN_EMERGENCY V10 // LED widget
#define VPIN_OPTICAL V11   // LED widget
#define VPIN_FLOAT V12     // LED widget
#define VPIN_CANISTER V13  // LED widget

// --- Fertilizer config (read/write) ---
#define VPIN_DOSE_CH1 V14   // Slider 0–50 mL
#define VPIN_DOSE_CH2 V15   // Slider 0–50 mL
#define VPIN_DOSE_CH3 V16   // Slider 0–50 mL
#define VPIN_DOSE_CH4 V17   // Slider 0–50 mL
#define VPIN_DOSE_PRIME V18 // Slider 0–50 mL

// --- Stock levels (read-only) ---
#define VPIN_STOCK_CH1 V19   // Display mL
#define VPIN_STOCK_CH2 V20   // Display mL
#define VPIN_STOCK_CH3 V21   // Display mL
#define VPIN_STOCK_CH4 V22   // Display mL
#define VPIN_STOCK_PRIME V23 // Display mL

// Forward declarations
class TimeManager;
class WaterManager;
class FertManager;
class SafetyWatchdog;

/// @brief Manages Blynk IoT connection, virtual pins, and serial commands.
class BlynkManager {
public:
  BlynkManager();

  /// Initialize Blynk and serial UI
  void begin(TimeManager *time, WaterManager *water, FertManager *fert,
             SafetyWatchdog *safety);

  /// Run Blynk + update telemetry (call from loop)
  void update();

  // ---- Schedule parameters (read by main loop) ----
  uint8_t getFertHour() const { return _fertHour; }
  uint8_t getFertMinute() const { return _fertMinute; }
  uint8_t getTPADay() const { return _tpaDay; }
  uint8_t getTPAHour() const { return _tpaHour; }
  uint8_t getTPAMinute() const { return _tpaMinute; }

  /// Process serial commands (always active)
  void processSerialCommands();

  /// Sync current firmware state to Blynk app
  void syncToApp();

  /// Send push notification via Blynk event
  void notifyEmergency(const char *message);
  void notifyTPAComplete();
  void notifyStockLow(uint8_t channel, float remaining);

  // ---- Manager pointers + schedule params (public for Blynk callbacks) ----
  TimeManager *_time;
  WaterManager *_water;
  FertManager *_fert;
  SafetyWatchdog *_safety;

  uint8_t _fertHour;
  uint8_t _fertMinute;
  uint8_t _tpaDay;
  uint8_t _tpaHour;
  uint8_t _tpaMinute;

  void _saveParams();

private:
  // Telemetry timing
  unsigned long _lastTelemetryMs;

  // Track notification state
  bool _emergencyNotified;
  bool _tpaCompleteNotified;

  void _loadParams();
  void _updateTelemetry();
  void _printStatus();
  void _printHelp();
};

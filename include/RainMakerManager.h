#pragma once

#include "Config.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiProv.h>

// Forward declarations
class TimeManager;
class WaterManager;
class FertManager;
class SafetyWatchdog;

/// @brief Manages ESP RainMaker node, devices, and parameters.
///
/// NOTE: ESP RainMaker requires the esp_rmaker Arduino library and specific
/// partition tables. Since the full RainMaker library has complex dependencies
/// on ESP-IDF components that may not be available in all PlatformIO configs,
/// this implementation provides the structure and can be compiled with or
/// without the actual RainMaker library by toggling USE_RAINMAKER.
///
/// When USE_RAINMAKER is not defined, parameters are stored locally and
/// accessible via Serial commands instead.
class RainMakerManager {
public:
  RainMakerManager();

  /// Initialize RainMaker node and devices
  void begin(TimeManager *time, WaterManager *water, FertManager *fert,
             SafetyWatchdog *safety);

  /// Update telemetry parameters (call periodically from loop)
  void updateTelemetry();

  // ---- Schedule parameters (read by main loop) ----
  uint8_t getFertHour() const { return _fertHour; }
  uint8_t getFertMinute() const { return _fertMinute; }
  uint8_t getTPADay() const { return _tpaDay; }
  uint8_t getTPAHour() const { return _tpaHour; }
  uint8_t getTPAMinute() const { return _tpaMinute; }

  /// Process serial commands (fallback when RainMaker not available)
  void processSerialCommands();

private:
  TimeManager *_time;
  WaterManager *_water;
  FertManager *_fert;
  SafetyWatchdog *_safety;

  // Schedule params (persisted via NVS through Preferences)
  uint8_t _fertHour;
  uint8_t _fertMinute;
  uint8_t _tpaDay;
  uint8_t _tpaHour;
  uint8_t _tpaMinute;

  // Telemetry timing
  unsigned long _lastTelemetryMs;

  /// Load schedule params from NVS
  void _loadParams();
  /// Save schedule params to NVS
  void _saveParams();

  /// Print current status to Serial
  void _printStatus();

  /// Print help for serial commands
  void _printHelp();

#ifdef USE_RAINMAKER
#include <RMakerDevice.h>
#include <RMakerParam.h>
#include <RMakerQR.h>
  /// Setup RainMaker devices and parameters
  void _setupRainMaker();
  /// RainMaker write callback (static, dispatches to instance)
  static void _writeCallback(Device *device, Param *param,
                             const param_val_t val, void *priv_data,
                             write_ctx_t *ctx);
#endif
};

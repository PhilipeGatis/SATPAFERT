#pragma once

#include "Config.h"
#include <Arduino.h>

// Forward declarations
class TimeManager;
class WaterManager;
class FertManager;
class SafetyWatchdog;

#ifdef USE_WEBSERVER
#include <ESPAsyncWebServer.h>
#endif

/// @brief Manages embedded web dashboard, REST API, and serial commands.
class WebManager {
public:
  WebManager();

  /// Initialize web server and serial UI
  void begin(TimeManager *time, WaterManager *water, FertManager *fert,
             SafetyWatchdog *safety);

  /// Run web server + update telemetry (call from loop)
  void update();

  // ---- Schedule parameters (read by main loop) ----
  uint8_t getFertHour() const { return _fertHour; }
  uint8_t getFertMinute() const { return _fertMinute; }
  uint8_t getTPADay() const { return _tpaDay; }
  uint8_t getTPAHour() const { return _tpaHour; }
  uint8_t getTPAMinute() const { return _tpaMinute; }

  /// Process serial commands (always active)
  void processSerialCommands();

private:
  // Manager pointers
  TimeManager *_time;
  WaterManager *_water;
  FertManager *_fert;
  SafetyWatchdog *_safety;

  // Schedule parameters
  uint8_t _fertHour;
  uint8_t _fertMinute;
  uint8_t _tpaDay;
  uint8_t _tpaHour;
  uint8_t _tpaMinute;

  // Telemetry timing
  unsigned long _lastTelemetryMs;
  unsigned long _lastSSEMs;

  // NVS persistence
  void _loadParams();
  void _saveParams();

  // Telemetry
  void _updateTelemetry();
  String _buildStatusJSON();

  // Serial UI
  void _printStatus();
  void _printHelp();

  // JSON helpers
  static int _extractInt(const String &json, const char *key);
  static float _extractFloat(const String &json, const char *key);
  static String _extractString(const String &json, const char *key);

#ifdef USE_WEBSERVER
  AsyncWebServer _server;
  AsyncEventSource _events;
  void _setupRoutes();
#endif
};

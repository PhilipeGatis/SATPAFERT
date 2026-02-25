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
  uint16_t getTpaInterval() const { return _tpaInterval; }
  uint8_t getTpaHour() const { return _tpaHour; }
  uint8_t getTpaMinute() const { return _tpaMinute; }
  uint32_t getTpaLastRun() const { return _tpaLastRun; }
  void setTpaLastRun(uint32_t epoch);
  uint8_t getTpaPercent() const { return _tpaPercent; }

  // ---- Aquarium config ----
  uint32_t getAquariumVolume() const {
    return (uint32_t)_aqHeight * _aqLength * _aqWidth / 1000;
  }
  float getLitersPerCm() const { return (float)_aqLength * _aqWidth / 1000.0f; }
  float getReservoirSafetyML() const { return _reservoirSafetyML; }
  uint16_t getReservoirVolume() const { return _reservoirVolume; }
  bool isTpaConfigReady() const {
    return _aqHeight > 0 && _aqLength > 0 && _aqWidth > 0 &&
           _reservoirVolume > 0 && _tpaPercent > 0;
  }

  /// Process serial commands (always active)
  void processSerialCommands();

private:
  // Manager pointers
  TimeManager *_time;
  WaterManager *_water;
  FertManager *_fert;
  SafetyWatchdog *_safety;

  // Schedule parameters
  uint16_t _tpaInterval;
  uint8_t _tpaHour;
  uint8_t _tpaMinute;
  uint32_t _tpaLastRun;
  uint8_t _tpaPercent; // % of aquarium volume to change

  float _primeML;

  // Aquarium dimensions (cm)
  uint16_t _aqHeight;        // Altura (cm)
  uint16_t _aqLength;        // Comprimento (cm)
  uint16_t _aqWidth;         // Largura (cm)
  float _drainFlowRate;      // mL/s
  float _refillFlowRate;     // mL/s
  float _primeRatio;         // mL per liter (manufacturer ratio)
  uint16_t _reservoirVolume; // reservoir liters
  float _reservoirSafetyML;  // min mL to keep in reservoir for pump

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
  static bool _extractFloatArray(const String &json, const char *key,
                                 float *outArray, uint8_t expectedSize);

#ifdef USE_WEBSERVER
  AsyncWebServer _server;
  AsyncEventSource _events;
  void _setupRoutes();
#endif
};

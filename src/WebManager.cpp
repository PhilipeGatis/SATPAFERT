#include "WebManager.h"
#include "FertManager.h"
#include "SafetyWatchdog.h"
#include "TimeManager.h"
#include "WaterManager.h"
#include "web_dashboard.h"
#include <Preferences.h>

#ifdef USE_WEBSERVER
#include <WiFi.h>
#endif

static Preferences _prefs;

// ============================================================================
// CONSTRUCTOR
// ============================================================================

WebManager::WebManager()
#ifdef USE_WEBSERVER
    : _server(80), _events("/events"),
#else
    :
#endif
      _time(nullptr), _water(nullptr), _fert(nullptr), _safety(nullptr),
      _fertHour(DEFAULT_FERT_HOUR), _fertMinute(DEFAULT_FERT_MINUTE),
      _tpaDay(DEFAULT_TPA_DAY), _tpaHour(DEFAULT_TPA_HOUR),
      _tpaMinute(DEFAULT_TPA_MINUTE), _lastTelemetryMs(0), _lastSSEMs(0) {
}

// ============================================================================
// BEGIN
// ============================================================================

void WebManager::begin(TimeManager *time, WaterManager *water,
                       FertManager *fert, SafetyWatchdog *safety) {
  _time = time;
  _water = water;
  _fert = fert;
  _safety = safety;

  _loadParams();

#ifdef USE_WEBSERVER
  _setupRoutes();
  _server.begin();
  Serial.println("[Web] Dashboard at http://" + WiFi.localIP().toString());
#else
  Serial.println("[Web] Web server disabled.");
#endif

  _printHelp();
  Serial.printf("[Web] Schedule: Fert=%02d:%02d | TPA=day%d %02d:%02d\n",
                _fertHour, _fertMinute, _tpaDay, _tpaHour, _tpaMinute);
}

// ============================================================================
// UPDATE (call from loop)
// ============================================================================

void WebManager::update() {
#ifdef USE_WEBSERVER
  // Send SSE telemetry every 2 seconds
  unsigned long now = millis();
  if ((now - _lastSSEMs) >= 2000 && _events.count() > 0) {
    _lastSSEMs = now;
    _events.send(_buildStatusJSON().c_str(), "status", millis());
  }
#endif
  _updateTelemetry();
}

// ============================================================================
// STATUS JSON
// ============================================================================

String WebManager::_buildStatusJSON() {
  String json = "{";

  if (_time) {
    json += "\"time\":\"" + _time->getFormattedTime() + "\",";
  }
  if (_safety) {
    json += "\"waterLevel\":" + String(_safety->getLastDistance(), 1) + ",";
    json +=
        "\"optical\":" + String(_safety->isOpticalHigh() ? "true" : "false") +
        ",";
    json +=
        "\"float\":" + String(_safety->isReservoirFull() ? "true" : "false") +
        ",";
    json +=
        "\"emergency\":" + String(_safety->isEmergency() ? "true" : "false") +
        ",";
    json += "\"maintenance\":" +
            String(_safety->isMaintenanceMode() ? "true" : "false") + ",";
  }
  if (_water) {
    json += "\"tpaState\":\"" + String(_water->getStateName()) + "\",";
    json +=
        "\"canister\":" + String(_water->isCanisterOn() ? "true" : "false") +
        ",";
  }

  // Schedule
  json += "\"fertHour\":" + String(_fertHour) + ",";
  json += "\"fertMinute\":" + String(_fertMinute) + ",";
  json += "\"tpaDay\":" + String(_tpaDay) + ",";
  json += "\"tpaHour\":" + String(_tpaHour) + ",";
  json += "\"tpaMinute\":" + String(_tpaMinute) + ",";

  // Stocks
  json += "\"stocks\":[";
  if (_fert) {
    for (uint8_t i = 0; i < NUM_FERTS + 1; i++) {
      if (i > 0)
        json += ",";
      json += "{\"stock\":" + String(_fert->getStockML(i), 0) +
              ",\"dose\":" + String(_fert->getDoseML(i), 1) + "}";
    }
  }
  json += "]";

  json += "}";
  return json;
}

// ============================================================================
// WEB ROUTES
// ============================================================================

#ifdef USE_WEBSERVER
void WebManager::_setupRoutes() {
  // ---- Dashboard HTML ----
  _server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", DASHBOARD_HTML);
  });

  // ---- SSE Events ----
  _events.onConnect([this](AsyncEventSourceClient *client) {
    Serial.println("[Web] SSE client connected");
    client->send(_buildStatusJSON().c_str(), "status", millis());
  });
  _server.addHandler(&_events);

  // ---- GET /api/status ----
  _server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
    request->send(200, "application/json", _buildStatusJSON());
  });

  // ---- POST /api/tpa/start ----
  _server.on("/api/tpa/start", HTTP_POST,
             [this](AsyncWebServerRequest *request) {
               if (_water)
                 _water->startTPA();
               Serial.println("[Web] TPA started via dashboard");
               request->send(200, "application/json", "{\"ok\":true}");
             });

  // ---- POST /api/tpa/abort ----
  _server.on("/api/tpa/abort", HTTP_POST,
             [this](AsyncWebServerRequest *request) {
               if (_water)
                 _water->abortTPA();
               Serial.println("[Web] TPA aborted via dashboard");
               request->send(200, "application/json", "{\"ok\":true}");
             });

  // ---- POST /api/maintenance/toggle ----
  _server.on("/api/maintenance/toggle", HTTP_POST,
             [this](AsyncWebServerRequest *request) {
               if (_safety) {
                 if (_safety->isMaintenanceMode()) {
                   _safety->exitMaintenance();
                   Serial.println("[Web] Maintenance OFF");
                 } else {
                   _safety->enterMaintenance();
                   Serial.println("[Web] Maintenance ON");
                 }
               }
               request->send(200, "application/json", "{\"ok\":true}");
             });

  // ---- POST /api/emergency/stop ----
  _server.on("/api/emergency/stop", HTTP_POST,
             [this](AsyncWebServerRequest *request) {
               if (_safety)
                 _safety->emergencyShutdown();
               Serial.println("[Web] EMERGENCY STOP via dashboard!");
               request->send(200, "application/json", "{\"ok\":true}");
             });

  // ---- POST /api/schedule (JSON body) ----
  _server.on(
      "/api/schedule", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
      [this](AsyncWebServerRequest *request, uint8_t *data, size_t len,
             size_t index, size_t total) {
        String body = String((char *)data).substring(0, len);
        // Simple JSON parsing without ArduinoJson (save memory)
        bool changed = false;
        int val;

        val = _extractInt(body, "fertHour");
        if (val >= 0 && val <= 23) {
          _fertHour = val;
          changed = true;
        }
        val = _extractInt(body, "fertMinute");
        if (val >= 0 && val <= 59) {
          _fertMinute = val;
          changed = true;
        }
        val = _extractInt(body, "tpaDay");
        if (val >= 0 && val <= 6) {
          _tpaDay = val;
          changed = true;
        }
        val = _extractInt(body, "tpaHour");
        if (val >= 0 && val <= 23) {
          _tpaHour = val;
          changed = true;
        }
        val = _extractInt(body, "tpaMinute");
        if (val >= 0 && val <= 59) {
          _tpaMinute = val;
          changed = true;
        }

        if (changed) {
          _saveParams();
          Serial.printf("[Web] Schedule updated: Fert=%02d:%02d TPA=day%d "
                        "%02d:%02d\n",
                        _fertHour, _fertMinute, _tpaDay, _tpaHour, _tpaMinute);
        }
        request->send(200, "application/json", "{\"ok\":true}");
      });

  // ---- POST /api/dose (JSON body) ----
  _server.on(
      "/api/dose", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
      [this](AsyncWebServerRequest *request, uint8_t *data, size_t len,
             size_t index, size_t total) {
        String body = String((char *)data).substring(0, len);
        int ch = _extractInt(body, "channel");
        float ml = _extractFloat(body, "ml");
        if (ch >= 0 && ch <= 4 && ml > 0 && _fert) {
          _fert->setDoseML(ch, ml);
          _fert->saveState();
          Serial.printf("[Web] Dose CH%d set to %.1f ml\n", ch + 1, ml);
        }
        request->send(200, "application/json", "{\"ok\":true}");
      });

  // ---- POST /api/stock/reset (JSON body) ----
  _server.on(
      "/api/stock/reset", HTTP_POST, [](AsyncWebServerRequest *request) {},
      NULL,
      [this](AsyncWebServerRequest *request, uint8_t *data, size_t len,
             size_t index, size_t total) {
        String body = String((char *)data).substring(0, len);
        int ch = _extractInt(body, "channel");
        float ml = _extractFloat(body, "ml");
        if (ch >= 0 && ch <= 4 && ml > 0 && _fert) {
          _fert->resetStock(ch, ml);
          Serial.printf("[Web] Stock CH%d reset to %.0f ml\n", ch + 1, ml);
        }
        request->send(200, "application/json", "{\"ok\":true}");
      });
}
#endif

// ============================================================================
// SIMPLE JSON EXTRACTORS (avoid ArduinoJson dependency)
// ============================================================================

int WebManager::_extractInt(const String &json, const char *key) {
  String search = String("\"") + key + "\":";
  int idx = json.indexOf(search);
  if (idx < 0)
    return -1;
  idx += search.length();
  return json.substring(idx).toInt();
}

float WebManager::_extractFloat(const String &json, const char *key) {
  String search = String("\"") + key + "\":";
  int idx = json.indexOf(search);
  if (idx < 0)
    return -1;
  idx += search.length();
  return json.substring(idx).toFloat();
}

// ============================================================================
// TELEMETRY (Serial)
// ============================================================================

void WebManager::_updateTelemetry() {
  unsigned long now = millis();
  if ((now - _lastTelemetryMs) < TELEMETRY_INTERVAL_MS)
    return;
  _lastTelemetryMs = now;

  Serial.println("--- Telemetry ---");
  if (_time) {
    Serial.printf("  Time: %s\n", _time->getFormattedTime().c_str());
  }
  if (_safety) {
    Serial.printf("  Water Level: %.1f cm\n", _safety->getLastDistance());
    Serial.printf("  Optical: %s | Float: %s\n",
                  _safety->isOpticalHigh() ? "HIGH" : "low",
                  _safety->isReservoirFull() ? "FULL" : "empty");
    Serial.printf("  Emergency: %s | Maintenance: %s\n",
                  _safety->isEmergency() ? "YES" : "no",
                  _safety->isMaintenanceMode() ? "YES" : "no");
  }
  if (_water) {
    Serial.printf("  TPA State: %s | Canister: %s\n", _water->getStateName(),
                  _water->isCanisterOn() ? "ON" : "OFF");
  }
  if (_fert) {
    for (uint8_t i = 0; i < NUM_FERTS; i++) {
      Serial.printf("  Fert CH%d: stock=%.0f ml\n", i + 1,
                    _fert->getStockML(i));
    }
    Serial.printf("  Prime: stock=%.0f ml\n", _fert->getStockML(NUM_FERTS));
  }
  Serial.println("-----------------");
}

// ============================================================================
// NVS PERSISTENCE
// ============================================================================

void WebManager::_loadParams() {
  _prefs.begin("aqua", true);
  _fertHour = _prefs.getUChar("fertH", DEFAULT_FERT_HOUR);
  _fertMinute = _prefs.getUChar("fertM", DEFAULT_FERT_MINUTE);
  _tpaDay = _prefs.getUChar("tpaD", DEFAULT_TPA_DAY);
  _tpaHour = _prefs.getUChar("tpaH", DEFAULT_TPA_HOUR);
  _tpaMinute = _prefs.getUChar("tpaM", DEFAULT_TPA_MINUTE);
  _prefs.end();
}

void WebManager::_saveParams() {
  _prefs.begin("aqua", false);
  _prefs.putUChar("fertH", _fertHour);
  _prefs.putUChar("fertM", _fertMinute);
  _prefs.putUChar("tpaD", _tpaDay);
  _prefs.putUChar("tpaH", _tpaHour);
  _prefs.putUChar("tpaM", _tpaMinute);
  _prefs.end();
}

// ============================================================================
// SERIAL COMMANDS
// ============================================================================

void WebManager::processSerialCommands() {
  if (!Serial.available())
    return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  if (cmd.length() == 0)
    return;

  if (cmd == "help" || cmd == "?") {
    _printHelp();
  } else if (cmd == "status") {
    _printStatus();
  } else if (cmd == "tpa") {
    Serial.println("[CMD] Starting TPA cycle...");
    if (_water)
      _water->startTPA();
  } else if (cmd == "abort") {
    Serial.println("[CMD] Aborting TPA...");
    if (_water)
      _water->abortTPA();
  } else if (cmd == "maint") {
    if (_safety) {
      if (_safety->isMaintenanceMode()) {
        _safety->exitMaintenance();
      } else {
        _safety->enterMaintenance();
      }
    }
  } else if (cmd.startsWith("fert_time ")) {
    int h = cmd.substring(10, 12).toInt();
    int m = cmd.substring(13, 15).toInt();
    if (h >= 0 && h <= 23 && m >= 0 && m <= 59) {
      _fertHour = h;
      _fertMinute = m;
      _saveParams();
      Serial.printf("[CMD] Fert schedule set to %02d:%02d\n", h, m);
    }
  } else if (cmd.startsWith("tpa_time ")) {
    int d = cmd.substring(9, 10).toInt();
    int h = cmd.substring(11, 13).toInt();
    int m = cmd.substring(14, 16).toInt();
    if (d >= 0 && d <= 6 && h >= 0 && h <= 23 && m >= 0 && m <= 59) {
      _tpaDay = d;
      _tpaHour = h;
      _tpaMinute = m;
      _saveParams();
      Serial.printf("[CMD] TPA schedule set to day %d, %02d:%02d\n", d, h, m);
    }
  } else if (cmd.startsWith("dose ")) {
    int ch = cmd.substring(5, 6).toInt();
    float ml = cmd.substring(7).toFloat();
    if (ch >= 1 && ch <= 4 && ml > 0) {
      _fert->setDoseML(ch - 1, ml);
      _fert->saveState();
      Serial.printf("[CMD] Fert CH%d dose set to %.1f ml\n", ch, ml);
    }
  } else if (cmd.startsWith("reset_stock ")) {
    int ch = cmd.substring(12, 13).toInt();
    float ml = cmd.substring(14).toFloat();
    if (ch >= 1 && ch <= 5 && ml > 0) {
      _fert->resetStock(ch - 1, ml);
      Serial.printf("[CMD] Stock CH%d reset to %.0f ml\n", ch, ml);
    }
  } else if (cmd == "drain_target") {
    if (_safety) {
      float dist = _safety->readUltrasonic();
      Serial.printf("[CMD] Current ultrasonic: %.1f cm\n", dist);
    }
  } else if (cmd.startsWith("set_drain ")) {
    float cm = cmd.substring(10).toFloat();
    if (cm > 0 && _water) {
      _water->setDrainTargetCm(cm);
      Serial.printf("[CMD] Drain target set to %.1f cm\n", cm);
    }
  } else if (cmd.startsWith("set_refill ")) {
    float cm = cmd.substring(11).toFloat();
    if (cm > 0 && _water) {
      _water->setRefillTargetCm(cm);
      Serial.printf("[CMD] Refill target set to %.1f cm\n", cm);
    }
  } else if (cmd == "canister_on") {
    digitalWrite(PIN_CANISTER, HIGH);
    Serial.println("[CMD] Canister ON.");
  } else if (cmd == "canister_off") {
    digitalWrite(PIN_CANISTER, LOW);
    Serial.println("[CMD] Canister OFF.");
  } else if (cmd == "emergency_stop") {
    if (_safety)
      _safety->emergencyShutdown();
  } else {
    Serial.printf("[CMD] Unknown: '%s'. Type 'help'.\n", cmd.c_str());
  }
}

// ============================================================================
// SERIAL UI
// ============================================================================

void WebManager::_printHelp() {
  Serial.println("\n--- Serial Commands ---");
  Serial.println("  help / ?          - Show this help");
  Serial.println("  status            - Print full system status");
  Serial.println("  tpa               - Start TPA cycle now");
  Serial.println("  abort             - Abort current TPA");
  Serial.println("  maint             - Toggle maintenance mode (30 min)");
  Serial.println("  fert_time HH:MM  - Set fertilization schedule");
  Serial.println("  tpa_time D HH:MM - Set TPA schedule (D=0-6, 0=Sun)");
  Serial.println("  dose CH ML       - Set dose for CH 1-4 (ml)");
  Serial.println("  reset_stock CH ML - Reset stock CH 1-5 (5=prime)");
  Serial.println("  set_drain CM     - Set drain target (cm)");
  Serial.println("  set_refill CM    - Set refill target (cm)");
  Serial.println("  drain_target      - Read current ultrasonic distance");
  Serial.println("  canister_on/off   - Manual canister control");
  Serial.println("  emergency_stop    - Shutdown all outputs");
  Serial.println("------------------------\n");
}

void WebManager::_printStatus() {
  Serial.println("\n=== System Status ===");
  if (_time) {
    Serial.printf("Time: %s\n", _time->getFormattedTime().c_str());
  }
  if (_safety) {
    Serial.printf("Water: %.1f cm | Emergency: %s | Maintenance: %s\n",
                  _safety->getLastDistance(),
                  _safety->isEmergency() ? "YES" : "no",
                  _safety->isMaintenanceMode() ? "YES" : "no");
  }
  if (_water) {
    Serial.printf("TPA: %s | Canister: %s\n", _water->getStateName(),
                  _water->isCanisterOn() ? "ON" : "OFF");
  }
  Serial.printf("Schedule: Fert=%02d:%02d | TPA=day%d %02d:%02d\n", _fertHour,
                _fertMinute, _tpaDay, _tpaHour, _tpaMinute);
  if (_fert) {
    for (uint8_t i = 0; i < NUM_FERTS; i++) {
      Serial.printf("CH%d: dose=%.1f ml, stock=%.0f ml\n", i + 1,
                    _fert->getDoseML(i), _fert->getStockML(i));
    }
    Serial.printf("Prime: dose=%.1f ml, stock=%.0f ml\n",
                  _fert->getDoseML(NUM_FERTS), _fert->getStockML(NUM_FERTS));
  }
  Serial.println("=====================\n");
}

#include "WebManager.h"
#include "FertManager.h"
#include "SafetyWatchdog.h"
#include "TimeManager.h"
#include "WaterManager.h"
#include <LittleFS.h>
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
  String ipStr = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString()
                                               : WiFi.softAPIP().toString();
  Serial.println("[Web] Dashboard at http://" + ipStr);
#else
  Serial.println("[Web] Web server disabled.");
#endif

  _printHelp();
  Serial.printf("[Web] TPA Schedule: day%d %02d:%02d\n", _tpaDay, _tpaHour,
                _tpaMinute);
}

// ============================================================================
// UPDATE (call from loop)
// ============================================================================

void WebManager::update() {
#ifdef USE_WEBSERVER
  // Send SSE telemetry every 3 seconds to reduce network congestion
  unsigned long now = millis();
  if ((now - _lastSSEMs) >= 3000 && _events.count() > 0) {
    _lastSSEMs = now;
    String json = _buildStatusJSON();
    _events.send(json.c_str(), "status", millis());
  }
#endif
  _updateTelemetry();
}

// ============================================================================
// STATUS JSON
// ============================================================================

String WebManager::_buildStatusJSON() {
  String json;
  json.reserve(1200); // Prevent heap fragmentation and speed up concatenation
  json += "{";

  // WiFi Connection Status
  json += "\"wifiConnected\":" +
          String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";

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
  json += "\"tpaDay\":" + String(_tpaDay) + ",";
  json += "\"tpaHour\":" + String(_tpaHour) + ",";
  json += "\"tpaMinute\":" + String(_tpaMinute) + ",";

  // Stocks
  json += "\"stocks\":[";
  if (_fert) {
    for (uint8_t i = 0; i < NUM_FERTS + 1; i++) {
      if (i > 0)
        json += ",";
      json += "{\"stock\":" + String(_fert->getStockML(i), 0) + ",\"name\":\"" +
              _fert->getName(i) + "\"" + ",\"doses\":[" +
              String(_fert->getDoseML(i, 0), 1) + "," +
              String(_fert->getDoseML(i, 1), 1) + "," +
              String(_fert->getDoseML(i, 2), 1) + "," +
              String(_fert->getDoseML(i, 3), 1) + "," +
              String(_fert->getDoseML(i, 4), 1) + "," +
              String(_fert->getDoseML(i, 5), 1) + "," +
              String(_fert->getDoseML(i, 6), 1) + "]" +
              ",\"sH\":" + String(_fert->getSchedHour(i)) +
              ",\"sM\":" + String(_fert->getSchedMinute(i)) +
              ",\"fR\":" + String(_fert->getFlowRate(i), 2) +
              ",\"pwm\":" + String(_fert->getPWM(i)) + "}";
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
  // ---- Dashboard React App (LittleFS) ----
  _server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

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

  // ---- GET /api/wifi/scan ----
  _server.on("/api/wifi/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
    int n = WiFi.scanComplete();
    if (n == -2) {
      // Scan hasn't started yet, trigger it
      WiFi.scanNetworks(true); // async scan
      request->send(202, "application/json", "{\"status\":\"scanning\"}");
    } else if (n == -1) {
      // Still scanning
      request->send(202, "application/json", "{\"status\":\"scanning\"}");
    } else if (n == 0) {
      request->send(200, "application/json", "{\"networks\":[]}");
      WiFi.scanDelete();
    } else {
      String json = "{\"networks\":[";
      for (int i = 0; i < n; ++i) {
        if (i > 0)
          json += ",";
        json += "{\"ssid\":\"" + WiFi.SSID(i) +
                "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
      }
      json += "]}";
      request->send(200, "application/json", json);
      WiFi.scanDelete();
    }
  });

  // ---- POST /api/wifi (Form Data: ssid, pass) ----
  _server.on("/api/wifi", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("ssid", true) && request->hasParam("pass", true)) {
      String ssid = request->getParam("ssid", true)->value();
      String pass = request->getParam("pass", true)->value();

      Preferences pref;
      pref.begin("wifi", false);
      pref.putString("ssid", ssid);
      pref.putString("pass", pass);
      pref.end();

      Serial.println(
          "[Web] WiFi credentials updated via dashboard. Restarting...");
      request->send(200, "application/json", "{\"ok\":true}");

      // Give the server time to send the response before rebooting
      delay(500);
      ESP.restart();
    } else {
      request->send(400, "application/json", "{\"error\":\"Missing params\"}");
    }
  });

  // ---- POST /api/schedule (JSON body - only TPA now) ----
  _server.on(
      "/api/schedule", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
      [this](AsyncWebServerRequest *request, uint8_t *data, size_t len,
             size_t index, size_t total) {
        String body = String((char *)data).substring(0, len);
        bool changed = false;
        int val;

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
          Serial.printf("[Web] TPA Schedule updated: day%d %02d:%02d\n",
                        _tpaDay, _tpaHour, _tpaMinute);
        }
        request->send(200, "application/json", "{\"ok\":true}");
      });

  // ---- Individual Fert Schedule ----
  _server.on(
      "/api/fert/schedule", HTTP_POST, [](AsyncWebServerRequest *request) {},
      NULL,
      [this](AsyncWebServerRequest *request, uint8_t *data, size_t len,
             size_t index, size_t total) {
        String body = String((char *)data).substring(0, len);
        int ch = _extractInt(body, "channel");
        int h = _extractInt(body, "hour");
        int m = _extractInt(body, "minute");
        float doses[7] = {0};
        bool hasDoses = _extractFloatArray(body, "doses", doses, 7);

        if (ch >= 0 && ch <= 4 && h >= 0 && h <= 23 && m >= 0 && m <= 59 &&
            hasDoses && _fert) {
          _fert->setScheduleTime(ch, h, m);
          for (uint8_t d = 0; d < 7; d++) {
            _fert->setDoseML(ch, d, doses[d]);
          }
          _fert->saveState();
          Serial.printf("[Web] CH%d Schedule updated -> time: %02d:%02d\n",
                        ch + 1, h, m);
        }
        request->send(200, "application/json", "{\"ok\":true}");
      });

  // ---- Pump Calibration: Toggle Prime ----
  _server.on(
      "/api/fert/pump", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
      [this](AsyncWebServerRequest *request, uint8_t *data, size_t len,
             size_t index, size_t total) {
        String body = String((char *)data).substring(0, len);
        int ch = _extractInt(body, "channel");
        int st = _extractInt(body, "state");

        if (ch >= 0 && ch <= 4 && _fert) {
          _fert->manualPump(ch, st == 1);
        }
        request->send(200, "application/json", "{\"ok\":true}");
      });

  // ---- Pump Calibration: Run 3 Seconds ----
  _server.on(
      "/api/fert/run3s", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
      [this](AsyncWebServerRequest *request, uint8_t *data, size_t len,
             size_t index, size_t total) {
        String body = String((char *)data).substring(0, len);
        int ch = _extractInt(body, "channel");

        if (ch >= 0 && ch <= 4 && _fert) {
          // Block and pulse
          _fert->manualPump(ch, true);
          unsigned long start = millis();
          while ((millis() - start) < 3000) {
            delay(10);
          }
          _fert->manualPump(ch, false);
        }
        request->send(200, "application/json", "{\"ok\":true}");
      });

  // ---- Pump Calibration: Save Flow Rate ----
  _server.on(
      "/api/fert/calibrate", HTTP_POST, [](AsyncWebServerRequest *request) {},
      NULL,
      [this](AsyncWebServerRequest *request, uint8_t *data, size_t len,
             size_t index, size_t total) {
        String body = String((char *)data).substring(0, len);
        int ch = _extractInt(body, "channel");

        int startIdx = body.indexOf("\"ml\":");
        if (startIdx != -1 && ch >= 0 && ch <= 4 && _fert) {
          startIdx += 5;
          int endIdx = body.indexOf(",", startIdx);
          if (endIdx == -1)
            endIdx = body.indexOf("}", startIdx);
          if (endIdx != -1) {
            float measuredML = body.substring(startIdx, endIdx).toFloat();
            if (measuredML > 0.1f) {
              float newRate = measuredML / 3.0f; // 3 seconds baseline
              _fert->setFlowRate(ch, newRate);
              _fert->saveState();
              Serial.printf("[Web] CH%d flow rate calibrated to %.2f mL/s\n",
                            ch + 1, newRate);
            }
          }
        }
        request->send(200, "application/json", "{\"ok\":true}");
      });

  // Obsolete: /api/dose replaced by full /api/fert/schedule usage.

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

  // ---- POST /api/fert/name (JSON body: {"channel": 0, "name": "Potássio"})
  // ----
  _server.on(
      "/api/fert/name", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
      [this](AsyncWebServerRequest *request, uint8_t *data, size_t len,
             size_t index, size_t total) {
        String body = String((char *)data).substring(0, len);
        int ch = _extractInt(body, "channel");
        String nameStr = _extractString(body, "name");

        if (ch >= 0 && ch <= 4 && nameStr.length() > 0 && _fert) {
          _fert->setName(ch, nameStr);
        }
        request->send(200, "application/json", "{\"ok\":true}");
      });

  // ---- POST /api/fert/pwm (JSON body: {"channel": 0, "pwm": 255})
  _server.on(
      "/api/fert/pwm", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
      [this](AsyncWebServerRequest *request, uint8_t *data, size_t len,
             size_t index, size_t total) {
        String body = String((char *)data).substring(0, len);
        int ch = _extractInt(body, "channel");
        int pwmValue = _extractInt(body, "pwm");

        if (ch >= 0 && ch <= 4 && pwmValue >= 0 && pwmValue <= 255 && _fert) {
          _fert->setPWM(ch, pwmValue);
          Serial.printf("[Web] CH%d PWM set to %d\n", ch + 1, pwmValue);
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

String WebManager::_extractString(const String &json, const char *key) {
  String search = String("\"") + key + "\":\"";
  int startIdx = json.indexOf(search);
  if (startIdx < 0)
    return "";
  startIdx += search.length();
  int endIdx = json.indexOf("\"", startIdx);
  if (endIdx < 0)
    return "";
  return json.substring(startIdx, endIdx);
}

bool WebManager::_extractFloatArray(const String &json, const char *key,
                                    float *outArray, uint8_t expectedSize) {
  String search = String("\"") + key + "\":[";
  int startIdx = json.indexOf(search);
  if (startIdx < 0)
    return false;
  startIdx += search.length();

  int endIdx = json.indexOf("]", startIdx);
  if (endIdx < 0)
    return false;

  String arrayStr = json.substring(startIdx, endIdx);
  int lastComma = 0;
  for (uint8_t i = 0; i < expectedSize; i++) {
    int nextComma = arrayStr.indexOf(",", lastComma);
    if (nextComma == -1) {
      outArray[i] = arrayStr.substring(lastComma).toFloat();
      break;
    } else {
      outArray[i] = arrayStr.substring(lastComma, nextComma).toFloat();
      lastComma = nextComma + 1;
    }
  }
  return true;
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
  _tpaDay = _prefs.getUChar("tpaD", DEFAULT_TPA_DAY);
  _tpaHour = _prefs.getUChar("tpaH", DEFAULT_TPA_HOUR);
  _tpaMinute = _prefs.getUChar("tpaM", DEFAULT_TPA_MINUTE);
  _prefs.end();
}

void WebManager::_saveParams() {
  _prefs.begin("aqua", false);
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
    Serial.println("[CMD] fert_time is obsolete. Use individual channel "
                   "scheduling via web.");
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
    Serial.println(
        "[CMD] dose is obsolete. Use individual channel scheduling via web.");
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
  Serial.printf("Schedule: TPA=day%d %02d:%02d\n", _tpaDay, _tpaHour,
                _tpaMinute);
  if (_fert) {
    for (uint8_t i = 0; i < NUM_FERTS; i++) {
      Serial.printf("CH%d: dose[Sun]=%.1f ml, stock=%.0f ml, rate=%.2f mL/s\n",
                    i + 1, _fert->getDoseML(i, 0), _fert->getStockML(i),
                    _fert->getFlowRate(i));
    }
    Serial.printf("Prime: dose[Sun]=%.1f ml, stock=%.0f ml, rate=%.2f mL/s\n",
                  _fert->getDoseML(NUM_FERTS, 0), _fert->getStockML(NUM_FERTS),
                  _fert->getFlowRate(NUM_FERTS));
  }
  Serial.println("=====================\n");
}

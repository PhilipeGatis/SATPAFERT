#include "RainMakerManager.h"
#include "FertManager.h"
#include "SafetyWatchdog.h"
#include "TimeManager.h"
#include "WaterManager.h"
#include <Preferences.h>

// Static NVS namespace for RainMaker params
static Preferences rmPrefs;

RainMakerManager::RainMakerManager()
    : _time(nullptr), _water(nullptr), _fert(nullptr), _safety(nullptr),
      _fertHour(DEFAULT_FERT_HOUR), _fertMinute(DEFAULT_FERT_MINUTE),
      _tpaDay(DEFAULT_TPA_DAY), _tpaHour(DEFAULT_TPA_HOUR),
      _tpaMinute(DEFAULT_TPA_MINUTE), _lastTelemetryMs(0) {}

void RainMakerManager::begin(TimeManager *time, WaterManager *water,
                             FertManager *fert, SafetyWatchdog *safety) {
  _time = time;
  _water = water;
  _fert = fert;
  _safety = safety;

  _loadParams();

#ifdef USE_RAINMAKER
  _setupRainMaker();
#else
  Serial.println(
      "[RainMaker] RainMaker disabled. Using Serial command interface.");
  _printHelp();
#endif

  Serial.printf("[RainMaker] Schedule: Fert=%02d:%02d | TPA=day%d %02d:%02d\n",
                _fertHour, _fertMinute, _tpaDay, _tpaHour, _tpaMinute);
}

void RainMakerManager::updateTelemetry() {
  unsigned long now = millis();
  if ((now - _lastTelemetryMs) < TELEMETRY_INTERVAL_MS)
    return;
  _lastTelemetryMs = now;

  // Print telemetry to Serial
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

#ifdef USE_RAINMAKER
  // TODO: Update RainMaker params for cloud telemetry
#endif
}

void RainMakerManager::processSerialCommands() {
#ifdef USE_RAINMAKER
  return; // RainMaker handles parameter changes
#endif

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
    // Format: fert_time HH:MM
    int h = cmd.substring(10, 12).toInt();
    int m = cmd.substring(13, 15).toInt();
    if (h >= 0 && h <= 23 && m >= 0 && m <= 59) {
      _fertHour = h;
      _fertMinute = m;
      _saveParams();
      Serial.printf("[CMD] Fert schedule set to %02d:%02d\n", h, m);
    } else {
      Serial.println("[CMD] Invalid format. Use: fert_time HH:MM");
    }
  } else if (cmd.startsWith("tpa_time ")) {
    // Format: tpa_time D HH:MM (D=0-6 day of week)
    int d = cmd.substring(9, 10).toInt();
    int h = cmd.substring(11, 13).toInt();
    int m = cmd.substring(14, 16).toInt();
    if (d >= 0 && d <= 6 && h >= 0 && h <= 23 && m >= 0 && m <= 59) {
      _tpaDay = d;
      _tpaHour = h;
      _tpaMinute = m;
      _saveParams();
      Serial.printf("[CMD] TPA schedule set to day %d, %02d:%02d\n", d, h, m);
    } else {
      Serial.println("[CMD] Invalid format. Use: tpa_time D HH:MM");
    }
  } else if (cmd.startsWith("dose ")) {
    // Format: dose CH ML (CH=1-4, ML=volume)
    int ch = cmd.substring(5, 6).toInt();
    float ml = cmd.substring(7).toFloat();
    if (ch >= 1 && ch <= 4 && ml > 0) {
      _fert->setDoseML(ch - 1, ml);
      _fert->saveState();
      Serial.printf("[CMD] Fert CH%d dose set to %.1f ml\n", ch, ml);
    } else {
      Serial.println("[CMD] Invalid format. Use: dose CH ML (CH=1-4)");
    }
  } else if (cmd.startsWith("reset_stock ")) {
    // Format: reset_stock CH ML (CH=1-5, 5=prime)
    int ch = cmd.substring(12, 13).toInt();
    float ml = cmd.substring(14).toFloat();
    if (ch >= 1 && ch <= 5 && ml > 0) {
      _fert->resetStock(ch - 1, ml);
      Serial.printf("[CMD] Stock CH%d reset to %.0f ml\n", ch, ml);
    } else {
      Serial.println("[CMD] Invalid format. Use: reset_stock CH ML (CH=1-5)");
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
    Serial.printf("[CMD] Unknown command: '%s'. Type 'help' for list.\n",
                  cmd.c_str());
  }
}

// ============================================================================
// PRIVATE
// ============================================================================

void RainMakerManager::_loadParams() {
  rmPrefs.begin("rmparams", false);
  _fertHour = rmPrefs.getUChar("fertH", DEFAULT_FERT_HOUR);
  _fertMinute = rmPrefs.getUChar("fertM", DEFAULT_FERT_MINUTE);
  _tpaDay = rmPrefs.getUChar("tpaD", DEFAULT_TPA_DAY);
  _tpaHour = rmPrefs.getUChar("tpaH", DEFAULT_TPA_HOUR);
  _tpaMinute = rmPrefs.getUChar("tpaM", DEFAULT_TPA_MINUTE);
}

void RainMakerManager::_saveParams() {
  rmPrefs.putUChar("fertH", _fertHour);
  rmPrefs.putUChar("fertM", _fertMinute);
  rmPrefs.putUChar("tpaD", _tpaDay);
  rmPrefs.putUChar("tpaH", _tpaHour);
  rmPrefs.putUChar("tpaM", _tpaMinute);
  Serial.println("[RainMaker] Parameters saved to NVS.");
}

void RainMakerManager::_printStatus() {
  Serial.println("\n========== SYSTEM STATUS ==========");
  Serial.printf("Fert Schedule: %02d:%02d\n", _fertHour, _fertMinute);
  Serial.printf("TPA  Schedule: day %d, %02d:%02d\n", _tpaDay, _tpaHour,
                _tpaMinute);

  if (_fert) {
    DateTime now = _time ? _time->now() : DateTime(2026, 1, 1);
    Serial.printf("Dosed Today: %s\n",
                  _fert->wasDosedToday(now) ? "YES" : "NO");
    for (uint8_t i = 0; i < NUM_FERTS; i++) {
      Serial.printf("  CH%d: dose=%.1f ml, stock=%.0f ml\n", i + 1,
                    _fert->getDoseML(i), _fert->getStockML(i));
    }
    Serial.printf("  Prime: dose=%.1f ml, stock=%.0f ml\n",
                  _fert->getDoseML(NUM_FERTS), _fert->getStockML(NUM_FERTS));
  }
  if (_water) {
    Serial.printf("TPA State: %s\n", _water->getStateName());
    Serial.printf("Canister: %s\n", _water->isCanisterOn() ? "ON" : "OFF");
  }
  if (_safety) {
    Serial.printf("Water Level: %.1f cm\n", _safety->getLastDistance());
    Serial.printf("Optical: %s | Float: %s\n",
                  _safety->isOpticalHigh() ? "HIGH" : "low",
                  _safety->isReservoirFull() ? "FULL" : "empty");
    Serial.printf("Emergency: %s | Maintenance: %s\n",
                  _safety->isEmergency() ? "YES" : "no",
                  _safety->isMaintenanceMode() ? "YES" : "no");
  }
  Serial.println("====================================\n");
}

void RainMakerManager::_printHelp() {
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

// ============================================================================
// RAINMAKER (conditional compilation)
// ============================================================================

#ifdef USE_RAINMAKER
#include <Device.h>
#include <RMaker.h>

// Instance pointer for static callback
static RainMakerManager *_rmInstance = nullptr;

void RainMakerManager::_setupRainMaker() {
  _rmInstance = this;

  // Initialize RainMaker node
  Node my_node = RMaker.initNode("AquariumController");

  // --- Fertilization Device ---
  Device *fertDevice = new Device("Fertilization", "esp.device.other", NULL);
  fertDevice->addNameParam();
  fertDevice->addPowerParam(true);

  // Add parameters
  Param fertH("FertHour", "esp.param.custom",
              value(static_cast<int>(_fertHour)),
              PROP_FLAG_READ | PROP_FLAG_WRITE);
  fertH.addBounds(value(0), value(23), value(1));
  fertH.addUIType(ESP_RMAKER_UI_SLIDER);
  fertDevice->addParam(fertH);

  Param fertM("FertMinute", "esp.param.custom",
              value(static_cast<int>(_fertMinute)),
              PROP_FLAG_READ | PROP_FLAG_WRITE);
  fertM.addBounds(value(0), value(59), value(1));
  fertM.addUIType(ESP_RMAKER_UI_SLIDER);
  fertDevice->addParam(fertM);

  for (int i = 0; i < NUM_FERTS; i++) {
    char name[16];
    snprintf(name, sizeof(name), "Dose%d_mL", i + 1);
    Param doseP(name, "esp.param.custom", value(_fert->getDoseML(i)),
                PROP_FLAG_READ | PROP_FLAG_WRITE);
    doseP.addBounds(value(0.0f), value(50.0f), value(0.5f));
    doseP.addUIType(ESP_RMAKER_UI_SLIDER);
    fertDevice->addParam(doseP);

    snprintf(name, sizeof(name), "Stock%d_mL", i + 1);
    Param stockP(name, "esp.param.custom", value(_fert->getStockML(i)),
                 PROP_FLAG_READ);
    fertDevice->addParam(stockP);
  }

  Param resetStock("ResetStock", "esp.param.custom", value(false),
                   PROP_FLAG_READ | PROP_FLAG_WRITE);
  resetStock.addUIType(ESP_RMAKER_UI_TOGGLE);
  fertDevice->addParam(resetStock);

  fertDevice->addCb(_writeCallback);
  my_node.addDevice(*fertDevice);

  // --- TPA Device ---
  Device *tpaDevice = new Device("TPA", "esp.device.other", NULL);
  tpaDevice->addNameParam();

  Param tpaD("TPADay", "esp.param.custom", value(static_cast<int>(_tpaDay)),
             PROP_FLAG_READ | PROP_FLAG_WRITE);
  tpaD.addBounds(value(0), value(6), value(1));
  tpaD.addUIType(ESP_RMAKER_UI_SLIDER);
  tpaDevice->addParam(tpaD);

  Param tpaH("TPAHour", "esp.param.custom", value(static_cast<int>(_tpaHour)),
             PROP_FLAG_READ | PROP_FLAG_WRITE);
  tpaH.addBounds(value(0), value(23), value(1));
  tpaH.addUIType(ESP_RMAKER_UI_SLIDER);
  tpaDevice->addParam(tpaH);

  Param tpaM("TPAMinute", "esp.param.custom",
             value(static_cast<int>(_tpaMinute)),
             PROP_FLAG_READ | PROP_FLAG_WRITE);
  tpaM.addBounds(value(0), value(59), value(1));
  tpaM.addUIType(ESP_RMAKER_UI_SLIDER);
  tpaDevice->addParam(tpaM);

  Param maintP("Maintenance", "esp.param.custom", value(false),
               PROP_FLAG_READ | PROP_FLAG_WRITE);
  maintP.addUIType(ESP_RMAKER_UI_TOGGLE);
  tpaDevice->addParam(maintP);

  Param startTPA("StartTPA", "esp.param.custom", value(false),
                 PROP_FLAG_READ | PROP_FLAG_WRITE);
  startTPA.addUIType(ESP_RMAKER_UI_TOGGLE);
  tpaDevice->addParam(startTPA);

  tpaDevice->addCb(_writeCallback);
  my_node.addDevice(*tpaDevice);

  // --- Sensors Device (read-only telemetry) ---
  Device *sensorsDevice = new Device("Sensors", "esp.device.other", NULL);
  sensorsDevice->addNameParam();

  Param waterLvl("WaterLevel_cm", "esp.param.custom", value(0.0f),
                 PROP_FLAG_READ);
  sensorsDevice->addParam(waterLvl);

  Param optSensor("OpticalSensor", "esp.param.custom", value(false),
                  PROP_FLAG_READ);
  sensorsDevice->addParam(optSensor);

  Param floatSw("ReservoirFloat", "esp.param.custom", value(false),
                PROP_FLAG_READ);
  sensorsDevice->addParam(floatSw);

  Param canStatus("CanisterStatus", "esp.param.custom", value(true),
                  PROP_FLAG_READ);
  sensorsDevice->addParam(canStatus);

  Param sysTime("SystemTime", "esp.param.custom", value("--"), PROP_FLAG_READ);
  sensorsDevice->addParam(sysTime);

  Param tpaState("TPAState", "esp.param.custom", value("IDLE"), PROP_FLAG_READ);
  sensorsDevice->addParam(tpaState);

  my_node.addDevice(*sensorsDevice);

  // Enable BLE provisioning
  RMaker.enableOTA(OTA_USING_PARAMS);
  RMaker.enableTZService();
  RMaker.enableSchedule();

  RMaker.start();

  WiFiProv.beginProvision(WIFI_PROV_SCHEME_BLE,
                          WIFI_PROV_SCHEME_HANDLER_FREE_BTDM,
                          WIFI_PROV_SECURITY_1, "aquarium01", "AquaControl");
}

void RainMakerManager::_writeCallback(Device *device, Param *param,
                                      const param_val_t val, void *priv_data,
                                      write_ctx_t *ctx) {
  if (!_rmInstance)
    return;

  String devName = device->getDeviceName();
  String paramName = param->getParamName();

  Serial.printf("[RainMaker] Write: %s.%s\n", devName.c_str(),
                paramName.c_str());

  if (devName == "Fertilization") {
    if (paramName == "FertHour") {
      _rmInstance->_fertHour = val.val.i;
      _rmInstance->_saveParams();
    } else if (paramName == "FertMinute") {
      _rmInstance->_fertMinute = val.val.i;
      _rmInstance->_saveParams();
    } else if (paramName.startsWith("Dose") && paramName.endsWith("_mL")) {
      int ch = paramName.substring(4, 5).toInt() - 1;
      if (ch >= 0 && ch < NUM_FERTS && _rmInstance->_fert) {
        _rmInstance->_fert->setDoseML(ch, val.val.f);
        _rmInstance->_fert->saveState();
      }
    } else if (paramName == "ResetStock") {
      if (val.val.b && _rmInstance->_fert) {
        for (int i = 0; i < NUM_FERTS; i++) {
          _rmInstance->_fert->resetStock(i, DEFAULT_STOCK_ML);
        }
      }
    }
  } else if (devName == "TPA") {
    if (paramName == "TPADay") {
      _rmInstance->_tpaDay = val.val.i;
      _rmInstance->_saveParams();
    } else if (paramName == "TPAHour") {
      _rmInstance->_tpaHour = val.val.i;
      _rmInstance->_saveParams();
    } else if (paramName == "TPAMinute") {
      _rmInstance->_tpaMinute = val.val.i;
      _rmInstance->_saveParams();
    } else if (paramName == "Maintenance") {
      if (_rmInstance->_safety) {
        if (val.val.b)
          _rmInstance->_safety->enterMaintenance();
        else
          _rmInstance->_safety->exitMaintenance();
      }
    } else if (paramName == "StartTPA") {
      if (val.val.b && _rmInstance->_water) {
        _rmInstance->_water->startTPA();
      }
    }
  }

  param->updateAndReport(val);
}
#endif // USE_RAINMAKER

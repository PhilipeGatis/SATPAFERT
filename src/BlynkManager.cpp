// Must be before #include <BlynkSimpleEsp32.h>
#include "BlynkManager.h"
#include "FertManager.h"
#include "SafetyWatchdog.h"
#include "TimeManager.h"
#include "WaterManager.h"
#include <Preferences.h>

#ifdef USE_BLYNK
#include <BlynkSimpleEsp32.h>
#include <WiFi.h>
#endif

// ---- NVS for schedule params ----
static Preferences blynkPrefs;

// ---- Static instance for Blynk callbacks ----
static BlynkManager *_blynkInstance = nullptr;

// ============================================================================
// CONSTRUCTOR
// ============================================================================

BlynkManager::BlynkManager()
    : _time(nullptr), _water(nullptr), _fert(nullptr), _safety(nullptr),
      _fertHour(DEFAULT_FERT_HOUR), _fertMinute(DEFAULT_FERT_MINUTE),
      _tpaDay(DEFAULT_TPA_DAY), _tpaHour(DEFAULT_TPA_HOUR),
      _tpaMinute(DEFAULT_TPA_MINUTE), _lastTelemetryMs(0),
      _emergencyNotified(false), _tpaCompleteNotified(false) {}

// ============================================================================
// BEGIN
// ============================================================================

void BlynkManager::begin(TimeManager *time, WaterManager *water,
                         FertManager *fert, SafetyWatchdog *safety) {
  _time = time;
  _water = water;
  _fert = fert;
  _safety = safety;
  _blynkInstance = this;

  _loadParams();

#ifdef USE_BLYNK
  // WiFi is already initialized in main.cpp setup()
  Blynk.config(BLYNK_AUTH_TOKEN);
  Blynk.connect();
  Serial.println("[Blynk] Blynk connection initiated.");
#else
  Serial.println("[Blynk] Blynk disabled. Using Serial command interface.");
#endif

  _printHelp();

  Serial.printf("[Blynk] Schedule: Fert=%02d:%02d | TPA=day%d %02d:%02d\n",
                _fertHour, _fertMinute, _tpaDay, _tpaHour, _tpaMinute);
}

// ============================================================================
// UPDATE (call from loop)
// ============================================================================

void BlynkManager::update() {
#ifdef USE_BLYNK
  Blynk.run();
#endif
  _updateTelemetry();
}

// ============================================================================
// TELEMETRY
// ============================================================================

void BlynkManager::_updateTelemetry() {
  unsigned long now = millis();
  if ((now - _lastTelemetryMs) < TELEMETRY_INTERVAL_MS)
    return;
  _lastTelemetryMs = now;

  // ---- Serial telemetry ----
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

  // ---- Blynk telemetry ----
#ifdef USE_BLYNK
  if (!Blynk.connected())
    return;

  // Water level gauge
  if (_safety) {
    Blynk.virtualWrite(VPIN_WATER_LEVEL, _safety->getLastDistance());
    Blynk.virtualWrite(VPIN_EMERGENCY, _safety->isEmergency() ? 255 : 0);
    Blynk.virtualWrite(VPIN_OPTICAL, _safety->isOpticalHigh() ? 255 : 0);
    Blynk.virtualWrite(VPIN_FLOAT, _safety->isReservoirFull() ? 255 : 0);
  }

  // TPA state
  if (_water) {
    Blynk.virtualWrite(VPIN_TPA_STATE, _water->getStateName());
    Blynk.virtualWrite(VPIN_CANISTER, _water->isCanisterOn() ? 255 : 0);

    // Reset maintenance toggle if auto-expired
    if (_safety && !_safety->isMaintenanceMode()) {
      Blynk.virtualWrite(VPIN_MAINTENANCE, 0);
    }
  }

  // System time
  if (_time) {
    Blynk.virtualWrite(VPIN_SYSTEM_TIME, _time->getFormattedTime());
  }

  // Stock levels
  if (_fert) {
    Blynk.virtualWrite(VPIN_STOCK_CH1, _fert->getStockML(0));
    Blynk.virtualWrite(VPIN_STOCK_CH2, _fert->getStockML(1));
    Blynk.virtualWrite(VPIN_STOCK_CH3, _fert->getStockML(2));
    Blynk.virtualWrite(VPIN_STOCK_CH4, _fert->getStockML(3));
    Blynk.virtualWrite(VPIN_STOCK_PRIME, _fert->getStockML(NUM_FERTS));
  }

  // ---- Push notifications ----
  // Emergency
  if (_safety && _safety->isEmergency() && !_emergencyNotified) {
    Blynk.logEvent("emergency", "⚠️ EMERGÊNCIA! Sensores detectaram risco!");
    _emergencyNotified = true;
  }
  if (_safety && !_safety->isEmergency()) {
    _emergencyNotified = false;
  }

  // TPA complete
  if (_water && _water->getState() == TPAState::COMPLETE &&
      !_tpaCompleteNotified) {
    Blynk.logEvent("tpa_complete", "✅ TPA finalizada com sucesso!");
    _tpaCompleteNotified = true;
  }
  if (_water && _water->getState() != TPAState::COMPLETE) {
    _tpaCompleteNotified = false;
  }

  // Stock low warning (< 50ml)
  if (_fert) {
    for (uint8_t i = 0; i < NUM_FERTS + 1; i++) {
      if (_fert->getStockML(i) > 0 && _fert->getStockML(i) < 50.0f) {
        char msg[64];
        snprintf(msg, sizeof(msg), "⚠️ Estoque CH%d baixo: %.0f ml", i + 1,
                 _fert->getStockML(i));
        Blynk.logEvent("stock_low", msg);
      }
    }
  }
#endif
}

// ============================================================================
// BLYNK CALLBACKS (virtual pin write handlers)
// ============================================================================

#ifdef USE_BLYNK

// --- Schedule params from app ---
BLYNK_WRITE(VPIN_FERT_HOUR) {
  if (_blynkInstance) {
    _blynkInstance->_fertHour = param.asInt();
    _blynkInstance->_saveParams();
    Serial.printf("[Blynk] Fert hour set to %d\n", param.asInt());
  }
}

BLYNK_WRITE(VPIN_FERT_MINUTE) {
  if (_blynkInstance) {
    _blynkInstance->_fertMinute = param.asInt();
    _blynkInstance->_saveParams();
    Serial.printf("[Blynk] Fert minute set to %d\n", param.asInt());
  }
}

BLYNK_WRITE(VPIN_TPA_DAY) {
  if (_blynkInstance) {
    _blynkInstance->_tpaDay = param.asInt();
    _blynkInstance->_saveParams();
    Serial.printf("[Blynk] TPA day set to %d\n", param.asInt());
  }
}

BLYNK_WRITE(VPIN_TPA_HOUR) {
  if (_blynkInstance) {
    _blynkInstance->_tpaHour = param.asInt();
    _blynkInstance->_saveParams();
    Serial.printf("[Blynk] TPA hour set to %d\n", param.asInt());
  }
}

BLYNK_WRITE(VPIN_TPA_MINUTE) {
  if (_blynkInstance) {
    _blynkInstance->_tpaMinute = param.asInt();
    _blynkInstance->_saveParams();
    Serial.printf("[Blynk] TPA minute set to %d\n", param.asInt());
  }
}

// --- Action buttons ---
BLYNK_WRITE(VPIN_START_TPA) {
  if (param.asInt() == 1 && _blynkInstance && _blynkInstance->_water) {
    Serial.println("[Blynk] TPA start requested!");
    _blynkInstance->_water->startTPA();
  }
}

BLYNK_WRITE(VPIN_ABORT_TPA) {
  if (param.asInt() == 1 && _blynkInstance && _blynkInstance->_water) {
    Serial.println("[Blynk] TPA abort requested!");
    _blynkInstance->_water->abortTPA();
  }
}

BLYNK_WRITE(VPIN_MAINTENANCE) {
  if (_blynkInstance && _blynkInstance->_safety) {
    if (param.asInt() == 1) {
      Serial.println("[Blynk] Maintenance ON");
      _blynkInstance->_safety->enterMaintenance();
    } else {
      Serial.println("[Blynk] Maintenance OFF");
      _blynkInstance->_safety->exitMaintenance();
    }
  }
}

// --- Dose config from app ---
BLYNK_WRITE(VPIN_DOSE_CH1) {
  if (_blynkInstance && _blynkInstance->_fert) {
    _blynkInstance->_fert->setDoseML(0, param.asFloat());
    _blynkInstance->_fert->saveState();
    Serial.printf("[Blynk] Dose CH1 = %.1f ml\n", param.asFloat());
  }
}

BLYNK_WRITE(VPIN_DOSE_CH2) {
  if (_blynkInstance && _blynkInstance->_fert) {
    _blynkInstance->_fert->setDoseML(1, param.asFloat());
    _blynkInstance->_fert->saveState();
    Serial.printf("[Blynk] Dose CH2 = %.1f ml\n", param.asFloat());
  }
}

BLYNK_WRITE(VPIN_DOSE_CH3) {
  if (_blynkInstance && _blynkInstance->_fert) {
    _blynkInstance->_fert->setDoseML(2, param.asFloat());
    _blynkInstance->_fert->saveState();
    Serial.printf("[Blynk] Dose CH3 = %.1f ml\n", param.asFloat());
  }
}

BLYNK_WRITE(VPIN_DOSE_CH4) {
  if (_blynkInstance && _blynkInstance->_fert) {
    _blynkInstance->_fert->setDoseML(3, param.asFloat());
    _blynkInstance->_fert->saveState();
    Serial.printf("[Blynk] Dose CH4 = %.1f ml\n", param.asFloat());
  }
}

BLYNK_WRITE(VPIN_DOSE_PRIME) {
  if (_blynkInstance && _blynkInstance->_fert) {
    _blynkInstance->_fert->setDoseML(NUM_FERTS, param.asFloat());
    _blynkInstance->_fert->saveState();
    Serial.printf("[Blynk] Dose Prime = %.1f ml\n", param.asFloat());
  }
}

// --- Sync current values when app connects ---
BLYNK_CONNECTED() {
  if (_blynkInstance) {
    _blynkInstance->syncToApp();
  }
}

#endif // USE_BLYNK

// ============================================================================
// SYNC STATE TO APP (public — called from BLYNK_CONNECTED)
// ============================================================================

void BlynkManager::syncToApp() {
#ifdef USE_BLYNK
  if (!Blynk.connected())
    return;
  Serial.println("[Blynk] Syncing state to app...");

  Blynk.virtualWrite(VPIN_FERT_HOUR, _fertHour);
  Blynk.virtualWrite(VPIN_FERT_MINUTE, _fertMinute);
  Blynk.virtualWrite(VPIN_TPA_DAY, _tpaDay);
  Blynk.virtualWrite(VPIN_TPA_HOUR, _tpaHour);
  Blynk.virtualWrite(VPIN_TPA_MINUTE, _tpaMinute);

  if (_fert) {
    Blynk.virtualWrite(VPIN_DOSE_CH1, _fert->getDoseML(0));
    Blynk.virtualWrite(VPIN_DOSE_CH2, _fert->getDoseML(1));
    Blynk.virtualWrite(VPIN_DOSE_CH3, _fert->getDoseML(2));
    Blynk.virtualWrite(VPIN_DOSE_CH4, _fert->getDoseML(3));
    Blynk.virtualWrite(VPIN_DOSE_PRIME, _fert->getDoseML(NUM_FERTS));
  }

  if (_safety) {
    Blynk.virtualWrite(VPIN_MAINTENANCE, _safety->isMaintenanceMode() ? 1 : 0);
  }
#endif
}

// ============================================================================
// PUSH NOTIFICATIONS
// ============================================================================

void BlynkManager::notifyEmergency(const char *message) {
#ifdef USE_BLYNK
  if (Blynk.connected()) {
    Blynk.logEvent("emergency", message);
  }
#endif
  Serial.printf("[ALERT] %s\n", message);
}

void BlynkManager::notifyTPAComplete() {
#ifdef USE_BLYNK
  if (Blynk.connected()) {
    Blynk.logEvent("tpa_complete", "TPA cycle completed successfully!");
  }
#endif
  Serial.println("[ALERT] TPA complete!");
}

void BlynkManager::notifyStockLow(uint8_t channel, float remaining) {
#ifdef USE_BLYNK
  if (Blynk.connected()) {
    char msg[64];
    snprintf(msg, sizeof(msg), "Stock CH%d low: %.0f ml", channel + 1,
             remaining);
    Blynk.logEvent("stock_low", msg);
  }
#endif
  Serial.printf("[ALERT] Stock CH%d low: %.0f ml\n", channel + 1, remaining);
}

// ============================================================================
// NVS PERSISTENCE
// ============================================================================

void BlynkManager::_loadParams() {
  blynkPrefs.begin("blkparams", false);
  _fertHour = blynkPrefs.getUChar("fertH", DEFAULT_FERT_HOUR);
  _fertMinute = blynkPrefs.getUChar("fertM", DEFAULT_FERT_MINUTE);
  _tpaDay = blynkPrefs.getUChar("tpaD", DEFAULT_TPA_DAY);
  _tpaHour = blynkPrefs.getUChar("tpaH", DEFAULT_TPA_HOUR);
  _tpaMinute = blynkPrefs.getUChar("tpaM", DEFAULT_TPA_MINUTE);
}

void BlynkManager::_saveParams() {
  blynkPrefs.putUChar("fertH", _fertHour);
  blynkPrefs.putUChar("fertM", _fertMinute);
  blynkPrefs.putUChar("tpaD", _tpaDay);
  blynkPrefs.putUChar("tpaH", _tpaHour);
  blynkPrefs.putUChar("tpaM", _tpaMinute);
  Serial.println("[Blynk] Parameters saved to NVS.");
}

// ============================================================================
// SERIAL COMMAND INTERFACE (always active — works with or without Blynk)
// ============================================================================

void BlynkManager::processSerialCommands() {
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
    } else {
      Serial.println("[CMD] Invalid format. Use: fert_time HH:MM");
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
    } else {
      Serial.println("[CMD] Invalid format. Use: tpa_time D HH:MM");
    }
  } else if (cmd.startsWith("dose ")) {
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
    Serial.printf("[CMD] Unknown: '%s'. Type 'help'.\n", cmd.c_str());
  }
}

// ============================================================================
// PRINT HELPERS
// ============================================================================

void BlynkManager::_printStatus() {
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
#ifdef USE_BLYNK
  Serial.printf("Blynk: %s\n",
                Blynk.connected() ? "CONNECTED" : "disconnected");
#else
  Serial.println("Blynk: DISABLED");
#endif
  Serial.println("====================================\n");
}

void BlynkManager::_printHelp() {
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

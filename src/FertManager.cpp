#include "FertManager.h"

FertManager::FertManager() {
  for (uint8_t i = 0; i < NUM_FERTS + 1; i++) {
    for (uint8_t d = 0; d < 7; d++) {
      _doseML[i][d] =
          0.0f; // Default 0 for all days to prevent accidental dosing
    }
    _stockML[i] = DEFAULT_STOCK_ML;
    _names[i] = (i < NUM_FERTS) ? String("CH") + String(i + 1) : "Prime";
    _lastDoseKey[i] = 0;
    _schedHour[i] = DEFAULT_FERT_HOUR;
    _schedMinute[i] = DEFAULT_FERT_MINUTE;
    _flowRateMLps[i] = FLOW_RATE_ML_PER_SEC; // Default 1.5 mL/s
    _pwm[i] = 255;
  }
}

void FertManager::begin() {
  _prefs.begin("fert", false); // RW mode
  _loadState();

  // Initialize LEDC (hardware PWM) for the pump pins
  for (uint8_t i = 0; i < NUM_FERTS + 1; i++) {
    ledcSetup(i, 5000, 8); // Channel i, 5kHz, 8-bit resolution (0-255)
    ledcAttachPin(_pinForChannel(i), i);
    ledcWrite(i, 0); // Initialize OFF
  }

  Serial.println("[Fert] Manager initialized.");
  Serial.printf("[Fert] Last dose key: %u\n", _lastDoseKey[0]);
  for (uint8_t i = 0; i < NUM_FERTS; i++) {
    Serial.printf("[Fert] CH%d ('%s'): stock=%.1f ml\n", i + 1,
                  _names[i].c_str(), _stockML[i]);
  }
  Serial.printf("[Fert] Prime ('%s'): stock=%.1f ml\n",
                _names[NUM_FERTS].c_str(), _stockML[NUM_FERTS]);
}

void FertManager::update(DateTime now) {
  uint32_t todayKey = _dateKey(now);
  uint8_t currentDow = now.dayOfTheWeek();
  uint8_t currentHour = now.hour();
  uint8_t currentMinute = now.minute();

  for (uint8_t i = 0; i < NUM_FERTS + 1; i++) {
    // Check if it's the right time (minute precision)
    if (currentHour == _schedHour[i] && currentMinute == _schedMinute[i]) {
      // Check if it was already dosed today
      if (_lastDoseKey[i] != todayKey) {
        float ds = _doseML[i][currentDow];
        if (ds > 0 && _stockML[i] >= ds) {
          Serial.printf("[Fert] Scheduled auto-dose CH%d: %.1f ml\n", i + 1,
                        ds);
          if (doseChannel(i, ds)) {
            _stockML[i] -= ds;
            if (_stockML[i] < 0)
              _stockML[i] = 0;
            _markDosed(i, now);
            saveState();
          }
        } else if (ds > 0) {
          Serial.printf(
              "[Fert] Skipping CH%d: Insufficient stock (%.1f < %.1f)\n", i + 1,
              _stockML[i], ds);
        } else if (ds <= 0) {
          _markDosed(
              i,
              now); // Volume is 0 for today, just mark as checked to prevent
                    // loop repeats if someone sets it via UI during the minute
        }
      }
    }
  }
}

bool FertManager::doseChannel(uint8_t ch, float ml) {
  if (ch > NUM_FERTS)
    return false;
  if (ml <= 0)
    return false;

  uint8_t pin = _pinForChannel(ch);
  float rate = _flowRateMLps[ch];
  if (rate <= 0)
    rate = FLOW_RATE_ML_PER_SEC; // Fallback safety

  unsigned long durationMs = (unsigned long)((ml / rate) * 1000.0f);
  unsigned long timeout =
      (ch == NUM_FERTS) ? TIMEOUT_PRIME_MS : TIMEOUT_FERT_MS;

  // Cap to timeout
  if (durationMs > timeout) {
    Serial.printf("[Fert] WARNING: dose duration %lu ms exceeds timeout %lu "
                  "ms. Capping.\n",
                  durationMs, timeout);
    durationMs = timeout;
  }

  Serial.printf("[Fert] Activating pin %d for %lu ms (Rate: %.2f mL/s)\n", pin,
                durationMs, rate);
  ledcWrite(ch, _pwm[ch]);

  unsigned long start = millis();
  while ((millis() - start) < durationMs) {
    delay(10); // Yield to watchdog
  }

  ledcWrite(ch, 0);
  return true;
}

void FertManager::manualPump(uint8_t ch, bool state) {
  if (ch > NUM_FERTS)
    return;
  ledcWrite(ch, state ? _pwm[ch] : 0);
  Serial.printf("[Fert] Manual pump CH%d set to %s (PWM: %d)\n", ch + 1,
                state ? "ON" : "OFF", state ? _pwm[ch] : 0);
}

void FertManager::setPWM(uint8_t ch, uint8_t pwm) {
  if (ch <= NUM_FERTS) {
    _pwm[ch] = pwm;
    saveState();
  }
}

void FertManager::setDoseML(uint8_t ch, uint8_t dayOfWeek, float ml) {
  if (ch <= NUM_FERTS && dayOfWeek < 7) {
    _doseML[ch][dayOfWeek] = ml;
  }
}

float FertManager::getDoseML(uint8_t ch, uint8_t dayOfWeek) const {
  return (ch <= NUM_FERTS && dayOfWeek < 7) ? _doseML[ch][dayOfWeek] : 0.0f;
}

void FertManager::setScheduleTime(uint8_t ch, uint8_t hour, uint8_t minute) {
  if (ch <= NUM_FERTS) {
    _schedHour[ch] = hour;
    _schedMinute[ch] = minute;
  }
}

void FertManager::setFlowRate(uint8_t ch, float mlPerSec) {
  if (ch <= NUM_FERTS && mlPerSec > 0.01f) {
    _flowRateMLps[ch] = mlPerSec;
  }
}

float FertManager::getStockML(uint8_t ch) const {
  return (ch <= NUM_FERTS) ? _stockML[ch] : 0;
}

void FertManager::setStockML(uint8_t ch, float ml) {
  if (ch <= NUM_FERTS) {
    _stockML[ch] = ml;
  }
}

void FertManager::resetStock(uint8_t ch, float ml) {
  if (ch <= NUM_FERTS) {
    _stockML[ch] = ml;
    saveState();
    Serial.printf("[Fert] Stock CH%d reset to %.1f ml\n", ch + 1, ml);
  }
}

String FertManager::getName(uint8_t ch) const {
  if (ch <= NUM_FERTS) {
    return _names[ch];
  }
  return "";
}

void FertManager::setName(uint8_t ch, const String &name) {
  if (ch <= NUM_FERTS) {
    // Truncate name to save NVS space (max 15 chars)
    String safeName = name.substring(0, 15);
    _names[ch] = safeName;
    saveState();
    Serial.printf("[Fert] CH%d renamed to '%s'\n", ch + 1, safeName.c_str());
  }
}

void FertManager::saveState() {
  for (uint8_t i = 0; i < NUM_FERTS + 1; i++) {
    char key[16];

    for (uint8_t d = 0; d < 7; d++) {
      snprintf(key, sizeof(key), "d%d_%d", i, d); // e.g. d0_0, d0_6
      _prefs.putFloat(key, _doseML[i][d]);
    }

    snprintf(key, sizeof(key), "stock%d", i);
    _prefs.putFloat(key, _stockML[i]);

    snprintf(key, sizeof(key), "name%d", i);
    _prefs.putString(key, _names[i]);

    snprintf(key, sizeof(key), "lk%d", i); // Last key
    _prefs.putUInt(key, _lastDoseKey[i]);

    snprintf(key, sizeof(key), "sH%d", i); // Schedule Hour
    _prefs.putUChar(key, _schedHour[i]);

    snprintf(key, sizeof(key), "sM%d", i); // Schedule Minute
    _prefs.putUChar(key, _schedMinute[i]);

    snprintf(key, sizeof(key), "fR%d", i); // Flow Rate
    _prefs.putFloat(key, _flowRateMLps[i]);

    snprintf(key, sizeof(key), "pwm%d", i); // PWM Config
    _prefs.putUChar(key, _pwm[i]);
  }
}

bool FertManager::wasDosedToday(DateTime now) const {
  // Simplification: return true if CH1 was dosed today (telemetry mostly)
  return _dateKey(now) == _lastDoseKey[0];
}

// ============================================================================
// PRIVATE
// ============================================================================

uint32_t FertManager::_dateKey(DateTime dt) const {
  // Unique key per day: year * 1000 + dayOfYear
  // dayOfYear approximation using month*31+day (good enough for dedup)
  return (uint32_t)dt.year() * 1000 + (uint32_t)dt.month() * 31 +
         (uint32_t)dt.day();
}

void FertManager::_loadState() {
  for (uint8_t i = 0; i < NUM_FERTS + 1; i++) {
    char key[16];

    // Backwards Compatibility: Read legacy single-dose to use as fallback
    snprintf(key, sizeof(key), "dose%d", i);
    float legacyDose = _prefs.getFloat(key, (i == NUM_FERTS) ? DEFAULT_PRIME_ML
                                                             : DEFAULT_DOSE_ML);

    // If a channel was previously disabled via schedule bitmask, respect it
    snprintf(key, sizeof(key), "sD%d", i);
    uint8_t legacyMask = _prefs.getUChar(key, 127);

    for (uint8_t d = 0; d < 7; d++) {
      snprintf(key, sizeof(key), "d%d_%d", i, d);
      float defaultDose = ((legacyMask & (1 << d)) != 0) ? legacyDose : 0.0f;
      _doseML[i][d] = _prefs.getFloat(key, defaultDose);
    }

    snprintf(key, sizeof(key), "stock%d", i);
    _stockML[i] = _prefs.getFloat(key, DEFAULT_STOCK_ML);

    snprintf(key, sizeof(key), "name%d", i);
    String defaultName =
        (i < NUM_FERTS) ? String("CH") + String(i + 1) : "Prime";
    _names[i] = _prefs.getString(key, defaultName);

    snprintf(key, sizeof(key), "lk%d", i);
    _lastDoseKey[i] = _prefs.getUInt(key, 0);

    snprintf(key, sizeof(key), "sH%d", i);
    _schedHour[i] = _prefs.getUChar(key, DEFAULT_FERT_HOUR);

    snprintf(key, sizeof(key), "sM%d", i);
    _schedMinute[i] = _prefs.getUChar(key, DEFAULT_FERT_MINUTE);

    snprintf(key, sizeof(key), "fR%d", i);
    _flowRateMLps[i] = _prefs.getFloat(key, FLOW_RATE_ML_PER_SEC);

    snprintf(key, sizeof(key), "pwm%d", i);
    _pwm[i] = _prefs.getUChar(key, 255);
  }
}

void FertManager::_markDosed(uint8_t ch, DateTime now) {
  if (ch <= NUM_FERTS) {
    _lastDoseKey[ch] = _dateKey(now);

    char key[16];
    snprintf(key, sizeof(key), "lk%d", ch);
    _prefs.putUInt(key, _lastDoseKey[ch]);
  }
}

uint8_t FertManager::_pinForChannel(uint8_t ch) const {
  if (ch < NUM_FERTS) {
    return FERT_PINS[ch];
  }
  if (ch == NUM_FERTS) {
    return PIN_PRIME;
  }
  return 0; // Invalid
}

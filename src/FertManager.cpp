#include "FertManager.h"

FertManager::FertManager() : _lastDoseKey(0) {
  for (uint8_t i = 0; i < NUM_FERTS + 1; i++) {
    _doseML[i] = (i < NUM_FERTS) ? DEFAULT_DOSE_ML : DEFAULT_PRIME_ML;
    _stockML[i] = DEFAULT_STOCK_ML;
  }
}

void FertManager::begin() {
  _prefs.begin("fert", false); // RW mode
  _loadState();
  Serial.println("[Fert] Manager initialized.");
  Serial.printf("[Fert] Last dose key: %u\n", _lastDoseKey);
  for (uint8_t i = 0; i < NUM_FERTS; i++) {
    Serial.printf("[Fert] CH%d ('%s'): dose=%.1f ml, stock=%.1f ml\n", i + 1,
                  _names[i].c_str(), _doseML[i], _stockML[i]);
  }
  Serial.printf("[Fert] Prime ('%s'): dose=%.1f ml, stock=%.1f ml\n",
                _names[NUM_FERTS].c_str(), _doseML[NUM_FERTS],
                _stockML[NUM_FERTS]);
}

bool FertManager::checkAndDose(DateTime now, uint8_t schedHour,
                               uint8_t schedMinute) {
  // Check if we're in the schedule window (exact minute match)
  if (now.hour() != schedHour || now.minute() != schedMinute) {
    return false;
  }

  // Deduplication: check if already dosed today
  if (wasDosedToday(now)) {
    return false;
  }

  Serial.println("[Fert] === Starting daily fertilization ===");

  bool allOk = true;
  for (uint8_t i = 0; i < NUM_FERTS; i++) {
    if (_doseML[i] <= 0 || _stockML[i] <= 0) {
      Serial.printf("[Fert] CH%d: skipped (dose=%.1f, stock=%.1f)\n", i + 1,
                    _doseML[i], _stockML[i]);
      continue;
    }

    float actualDose = min(_doseML[i], _stockML[i]);
    Serial.printf("[Fert] CH%d: dosing %.1f ml...\n", i + 1, actualDose);

    if (doseChannel(i, actualDose)) {
      _stockML[i] -= actualDose;
      Serial.printf("[Fert] CH%d: done. Stock remaining: %.1f ml\n", i + 1,
                    _stockML[i]);
    } else {
      Serial.printf("[Fert] CH%d: TIMEOUT during dosing!\n", i + 1);
      allOk = false;
    }

    delay(500); // Brief pause between channels
  }

  // Mark today as dosed and persist
  _markDosed(now);
  saveState();

  Serial.println("[Fert] === Daily fertilization complete ===");
  return allOk;
}

bool FertManager::doseChannel(uint8_t ch, float ml) {
  if (ch > NUM_FERTS)
    return false;
  if (ml <= 0)
    return false;

  uint8_t pin = _pinForChannel(ch);
  unsigned long durationMs =
      (unsigned long)((ml / FLOW_RATE_ML_PER_SEC) * 1000.0f);
  unsigned long timeout =
      (ch == NUM_FERTS) ? TIMEOUT_PRIME_MS : TIMEOUT_FERT_MS;

  // Cap to timeout
  if (durationMs > timeout) {
    Serial.printf("[Fert] WARNING: dose duration %lu ms exceeds timeout %lu "
                  "ms. Capping.\n",
                  durationMs, timeout);
    durationMs = timeout;
  }

  Serial.printf("[Fert] Activating pin %d for %lu ms\n", pin, durationMs);
  digitalWrite(pin, HIGH);

  unsigned long start = millis();
  while ((millis() - start) < durationMs) {
    delay(10); // Yield to watchdog
  }

  digitalWrite(pin, LOW);
  return true;
}

void FertManager::setDoseML(uint8_t ch, float ml) {
  if (ch <= NUM_FERTS) {
    _doseML[ch] = ml;
  }
}

float FertManager::getDoseML(uint8_t ch) const {
  return (ch <= NUM_FERTS) ? _doseML[ch] : 0;
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
  _prefs.putUInt("lastDoseKey", _lastDoseKey);
  for (uint8_t i = 0; i < NUM_FERTS + 1; i++) {
    char keyStock[12];
    snprintf(keyStock, sizeof(keyStock), "stock%d", i);
    _prefs.putFloat(keyStock, _stockML[i]);

    char keyName[12];
    snprintf(keyName, sizeof(keyName), "name%d", i);
    _prefs.putString(keyName, _names[i]);
  }
}

bool FertManager::wasDosedToday(DateTime now) const {
  return _dateKey(now) == _lastDoseKey;
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
  _lastDoseKey = _prefs.getUInt("lastDoseKey", 0);
  for (uint8_t i = 0; i < NUM_FERTS + 1; i++) {
    char keyStock[12];
    snprintf(keyStock, sizeof(keyStock), "stock%d", i);
    _stockML[i] = _prefs.getFloat(keyStock, DEFAULT_STOCK_ML);

    char keyName[12];
    snprintf(keyName, sizeof(keyName), "name%d", i);
    String defaultName =
        (i < NUM_FERTS) ? String("CH") + String(i + 1) : "Prime";
    _names[i] = _prefs.getString(keyName, defaultName);
  }
}

void FertManager::_markDosed(DateTime now) {
  _lastDoseKey = _dateKey(now);
  _prefs.putUInt("lastDoseKey", _lastDoseKey);
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

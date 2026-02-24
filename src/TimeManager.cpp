#include "TimeManager.h"

TimeManager::TimeManager()
    : _timeClient(_ntpUDP, "pool.ntp.org", UTC_OFFSET_BRASILIA),
      _rtcConnected(false), _lastNtpSync(0) {}

void TimeManager::begin() {
  // Initialize I2C and RTC
  Wire.begin(); // SDA=21, SCL=22 (ESP32 defaults)

  if (!_rtc.begin()) {
    Serial.println("[Time] RTC DS3231 not found! Using NTP only.");
    _rtcConnected = false;
  } else {
    _rtcConnected = true;
    Serial.println("[Time] RTC DS3231 detected.");

    if (_rtc.lostPower()) {
      Serial.println("[Time] RTC lost power, needs sync.");
    }
  }

  // Start NTP client
  _timeClient.begin();
  _timeClient.setTimeOffset(UTC_OFFSET_BRASILIA);

  // Attempt initial NTP sync
  syncWithNTP();
}

void TimeManager::update() {
  unsigned long now = millis();

  // Periodic NTP resync
  if ((now - _lastNtpSync) >= NTP_SYNC_INTERVAL_MS) {
    syncWithNTP();
  }
}

bool TimeManager::syncWithNTP() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Time] No Wi-Fi, skipping NTP sync.");
    return false;
  }

  Serial.println("[Time] Syncing with NTP...");
  _timeClient.update();

  unsigned long epoch = _timeClient.getEpochTime();
  if (epoch < 1000000) {
    Serial.println("[Time] NTP returned invalid epoch.");
    return false;
  }

  if (_rtcConnected) {
    DateTime ntpTime(epoch);
    _rtc.adjust(ntpTime);
    Serial.println("[Time] RTC adjusted from NTP.");
  }

  _lastNtpSync = millis();
  return true;
}

DateTime TimeManager::now() {
  if (_rtcConnected) {
    return _rtc.now();
  }

  // Fallback: use NTP epoch directly
  _timeClient.update();
  unsigned long epoch = _timeClient.getEpochTime();
  return DateTime(epoch);
}

bool TimeManager::isDailyScheduleTime(uint8_t hour, uint8_t minute) {
  DateTime current = now();
  // Match hour and minute exactly (within a 60-second window)
  return (current.hour() == hour && current.minute() == minute);
}

bool TimeManager::isWeeklyScheduleDay(uint8_t dayOfWeek, uint8_t hour,
                                      uint8_t minute) {
  DateTime current = now();
  // dayOfWeek: 0=Sunday, 1=Monday, ..., 6=Saturday
  return (current.dayOfTheWeek() == dayOfWeek && current.hour() == hour &&
          current.minute() == minute);
}

String TimeManager::getFormattedTime() {
  DateTime dt = now();
  char buf[22];
  snprintf(buf, sizeof(buf), "%04d/%02d/%02d %02d:%02d:%02d", dt.year(),
           dt.month(), dt.day(), dt.hour(), dt.minute(), dt.second());
  return String(buf);
}

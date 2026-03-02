#include "DisplayManager.h"
#include "FertManager.h"
#include "SafetyWatchdog.h"
#include "TimeManager.h"
#include "WaterManager.h"
#include "WebManager.h"
#include <WiFi.h>

// =============================================================================
// CONSTRUCTOR
// =============================================================================
DisplayManager::DisplayManager()
    : _display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET), _time(nullptr),
      _water(nullptr), _fert(nullptr), _safety(nullptr), _web(nullptr),
      _currentPage(0), _lastPageSwitch(0) {}

// =============================================================================
// BEGIN
// =============================================================================
void DisplayManager::begin(TimeManager *time, WaterManager *water,
                           FertManager *fert, SafetyWatchdog *safety,
                           WebManager *web) {
  _time = time;
  _water = water;
  _fert = fert;
  _safety = safety;
  _web = web;

  if (!_display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("[Display] SSD1306 init FAILED!");
    return;
  }

  Serial.println("[Display] SSD1306 initialized OK.");

  // Splash screen
  _display.clearDisplay();
  _display.setTextColor(SSD1306_WHITE);

  _display.setTextSize(2);
  _display.setCursor(10, 10);
  _display.println(F("SATPAFERT"));

  _display.setTextSize(1);
  _display.setCursor(30, 35);
  _display.println(F("v3.0.0"));
  _display.setCursor(15, 50);
  _display.println(F("Aquarium System"));

  _display.display();

  _lastPageSwitch = millis();
}

// =============================================================================
// UPDATE — cycles pages every PAGE_CYCLE_MS
// =============================================================================
void DisplayManager::update() {
  unsigned long now = millis();
  if (now - _lastPageSwitch < PAGE_CYCLE_MS) {
    return;
  }
  _lastPageSwitch = now;

  _display.clearDisplay();

  switch (_currentPage) {
  case 0:
    _drawNetworkPage();
    break;
  case 1:
    _drawAquariumPage();
    break;
  case 2:
    _drawStockPage();
    break;
  case 3:
    _drawSchedulePage();
    break;
  }

  _display.display();
  _currentPage = (_currentPage + 1) % NUM_PAGES;
}

// =============================================================================
// HELPER — draw page header with title and separator line
// =============================================================================
void DisplayManager::_drawHeader(const char *title) {
  _display.setTextSize(1);
  _display.setTextColor(SSD1306_WHITE);
  _display.setCursor(0, 0);
  _display.println(title);
  _display.drawLine(0, 10, SCREEN_WIDTH - 1, 10, SSD1306_WHITE);
}

// =============================================================================
// PAGE 1 — Network Info
// =============================================================================
void DisplayManager::_drawNetworkPage() {
  _drawHeader(">> REDE");

  _display.setCursor(0, 14);

  if (WiFi.status() == WL_CONNECTED) {
    _display.print(F("WiFi: "));
    _display.println(WiFi.SSID());

    _display.print(F("IP: "));
    _display.println(WiFi.localIP());

    _display.print(F("RSSI: "));
    _display.print(WiFi.RSSI());
    _display.println(F(" dBm"));
  } else {
    _display.println(F("WiFi: Desconectado"));

    _display.print(F("AP IP: "));
    _display.println(WiFi.softAPIP());

    _display.print(F("AP SSID: "));
    _display.println(F(AP_SSID));
  }

  // Page indicator
  _display.setCursor(110, 56);
  _display.print(F("1/4"));
}

// =============================================================================
// PAGE 2 — Aquarium Status
// =============================================================================
void DisplayManager::_drawAquariumPage() {
  _drawHeader(">> AQUARIO");

  _display.setCursor(0, 14);

  // Volume
  uint32_t vol = _web->getAquariumVolume();
  _display.print(F("Volume: "));
  _display.print(vol);
  _display.println(F(" L"));

  // Water level (distance from sensor)
  float dist = _safety->getLastDistance();
  _display.print(F("Nivel: "));
  if (dist >= 0) {
    _display.print(dist, 1);
    _display.println(F(" cm"));
  } else {
    _display.println(F("-- cm"));
  }

  // TPA state
  _display.print(F("TPA: "));
  _display.println(_water->getStateName());

  // Canister
  _display.print(F("Canister: "));
  _display.println(_water->isCanisterOn() ? "ON" : "OFF");

  _display.setCursor(110, 56);
  _display.print(F("2/4"));
}

// =============================================================================
// PAGE 3 — Fertilizer & Prime Stock
// =============================================================================
void DisplayManager::_drawStockPage() {
  _drawHeader(">> ESTOQUE (mL)");

  _display.setCursor(0, 14);

  // Fert 1-4 on two lines (compact)
  for (uint8_t ch = 0; ch < NUM_FERTS; ch++) {
    String name = _fert->getName(ch);
    if (name.length() == 0) {
      name = "F" + String(ch + 1);
    }
    // Truncate name to 4 chars for space
    if (name.length() > 4)
      name = name.substring(0, 4);

    _display.print(name);
    _display.print(F(":"));
    _display.print(_fert->getStockML(ch), 0);

    if (ch < NUM_FERTS - 1) {
      _display.print(F("  "));
    }
  }
  _display.println();

  // Prime
  _display.println();
  _display.print(F("Prime: "));
  _display.print(_fert->getStockML(NUM_FERTS), 0);
  _display.println(F(" mL"));

  _display.setCursor(110, 56);
  _display.print(F("3/4"));
}

// =============================================================================
// PAGE 4 — Schedules
// =============================================================================
void DisplayManager::_drawSchedulePage() {
  _drawHeader(">> AGENDAMENTOS");

  _display.setCursor(0, 14);

  // Next fertilization (show CH1 schedule as reference)
  _display.print(F("Fert: "));
  for (uint8_t ch = 0; ch < NUM_FERTS; ch++) {
    uint8_t h = _fert->getSchedHour(ch);
    uint8_t m = _fert->getSchedMinute(ch);
    if (ch > 0)
      _display.print(F(" "));
    if (h < 10)
      _display.print('0');
    _display.print(h);
    _display.print(':');
    if (m < 10)
      _display.print('0');
    _display.print(m);
  }
  _display.println();

  // TPA schedule
  _display.println();
  _display.print(F("TPA: "));
  uint8_t th = _web->getTpaHour();
  uint8_t tm = _web->getTpaMinute();
  if (th < 10)
    _display.print('0');
  _display.print(th);
  _display.print(':');
  if (tm < 10)
    _display.print('0');
  _display.print(tm);

  uint16_t interval = _web->getTpaInterval();
  if (interval > 0) {
    _display.print(F(" (a cada "));
    _display.print(interval);
    _display.print(F("d)"));
  }
  _display.println();

  // Current time
  _display.println();
  DateTime now = _time->now();
  _display.print(F("Agora: "));
  if (now.hour() < 10)
    _display.print('0');
  _display.print(now.hour());
  _display.print(':');
  if (now.minute() < 10)
    _display.print('0');
  _display.print(now.minute());
  _display.print(':');
  if (now.second() < 10)
    _display.print('0');
  _display.print(now.second());

  _display.setCursor(110, 56);
  _display.print(F("4/4"));
}

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
// INIT HARDWARE — call early in setup() to show boot screen immediately
// =============================================================================
bool DisplayManager::initHardware() {
  if (!_display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("[Display] SSD1306 init FAILED!");
    return false;
  }

  Serial.println("[Display] SSD1306 128x32 initialized OK.");

  // Quick white flash — confirms panel works
  _display.clearDisplay();
  _display.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
  _display.display();
  delay(500);

  // Splash screen (128x32: 4 lines of 8px each)
  _display.clearDisplay();
  _display.setTextColor(SSD1306_WHITE);
  _display.setTextSize(2);
  _display.setCursor(30, 0);
  _display.print(F("IARA"));
  _display.setTextSize(1);
  _display.setCursor(30, 20);
  _display.print(F("v3.0.0"));
  _display.display();
  return true;
}

// =============================================================================
// SHOW BOOT STATUS — display boot progress on OLED
// =============================================================================
void DisplayManager::showBootStatus(const char *line1, const char *line2) {
  _display.clearDisplay();
  _display.setTextColor(SSD1306_WHITE);
  _display.setTextSize(1);
  _display.setCursor(0, 0);
  _display.print(F("IARA > "));
  _display.println(line1);
  if (line2) {
    _display.setCursor(0, 12);
    _display.println(line2);
  }
  _display.display();
}

// =============================================================================
// BEGIN — full init with manager pointers (call after all managers ready)
// =============================================================================
void DisplayManager::begin(TimeManager *time, WaterManager *water,
                           FertManager *fert, SafetyWatchdog *safety,
                           WebManager *web) {
  _time = time;
  _water = water;
  _fert = fert;
  _safety = safety;
  _web = web;

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

  // Draw persistent water level bar in yellow zone on ALL pages
  static const char *pageNames[] = {"REDE", "AQUARIO", "ESTOQUE", "AGENDA"};
  _drawHeader(pageNames[_currentPage]);

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
// HELPER — yellow zone: water level bar + page title (y=0 to 8)
// =============================================================================
void DisplayManager::_drawHeader(const char *title) {
  _display.setTextSize(1);
  _display.setTextColor(SSD1306_WHITE);

  // Water level bar: sensor distance inverted (less dist = more water)
  // Sensor range: 0cm (full) to 30cm (empty)
  float dist = _safety->getLastDistance();
  const float maxDist = 30.0f; // cm — sensor at top, water below

  if (dist >= 0) {
    float pct = 1.0f - (dist / maxDist); // invert: less distance = more water
    if (pct > 1.0f)
      pct = 1.0f;
    if (pct < 0.0f)
      pct = 0.0f;

    // Progress bar: x=0 to 83 (leave room for title)
    uint8_t barW = 84;
    _display.drawRect(0, 0, barW, 8, SSD1306_WHITE);
    uint8_t fillW = (uint8_t)(pct * (barW - 2));
    if (fillW > 0) {
      _display.fillRect(1, 1, fillW, 6, SSD1306_WHITE);
    }

    // Percentage label inside bar
    int pctVal = (int)(pct * 100);
    char buf[5];
    snprintf(buf, sizeof(buf), "%d%%", pctVal);
    uint8_t textW = strlen(buf) * 6;
    uint8_t tx = (barW - textW) / 2;
    _display.setTextColor(fillW > (barW / 2) ? SSD1306_BLACK : SSD1306_WHITE);
    _display.setCursor(tx, 0);
    _display.print(buf);
    _display.setTextColor(SSD1306_WHITE);
  } else {
    // No sensor reading
    _display.setCursor(0, 0);
    _display.print(F("Nivel: --"));
  }

  // Page title right-aligned
  uint8_t titleW = strlen(title) * 6;
  _display.setCursor(SCREEN_WIDTH - titleW, 0);
  _display.print(title);
}

// =============================================================================
// PAGE 1 — Network Info
// =============================================================================
void DisplayManager::_drawNetworkPage() {

  if (WiFi.status() == WL_CONNECTED) {
    _display.setCursor(0, 10);
    _display.println(WiFi.localIP());
    _display.print(F("RSSI:"));
    _display.print(WiFi.RSSI());
    _display.print(F("dBm"));
  } else {
    _display.setCursor(0, 10);
    _display.println(F("WiFi: OFF"));
    _display.print(F("AP: "));
    _display.print(WiFi.softAPIP());
  }
}

// =============================================================================
// PAGE 2 — Aquarium Status
// =============================================================================
void DisplayManager::_drawAquariumPage() {

  _display.setCursor(0, 10);
  float dist = _safety->getLastDistance();
  _display.print(F("Nivel:"));
  if (dist >= 0) {
    _display.print(dist, 1);
    _display.print(F("cm"));
  } else {
    _display.print(F("--"));
  }
  _display.print(F(" Can:"));
  _display.println(_water->isCanisterOn() ? "ON" : "OFF");

  _display.print(F("TPA: "));
  _display.print(_water->getStateName());
}

// =============================================================================
// PAGE 3 — Fertilizer & Prime Stock
// =============================================================================
void DisplayManager::_drawStockPage() {
  _display.setTextSize(1);
  _display.setTextColor(SSD1306_WHITE);

  // Bar chart: 5 channels (F1-F4 + Prime)
  const uint8_t numBars = NUM_FERTS + 1;
  const uint8_t barW = 20;
  const uint8_t gap = (SCREEN_WIDTH - numBars * barW) / (numBars + 1);
  const uint8_t barTop = 10;
  const uint8_t barH = SCREEN_HEIGHT - barTop; // 22px
  const float maxStock = 500.0f;

  // --- Pass 1: Draw bars in normal rotation (0) ---
  for (uint8_t i = 0; i < numBars; i++) {
    uint8_t x = gap + i * (barW + gap);
    float stock = _fert->getStockML(i);
    float pct = stock / maxStock;
    if (pct > 1.0f)
      pct = 1.0f;
    if (pct < 0.0f)
      pct = 0.0f;
    uint8_t fillH = (uint8_t)(pct * barH);

    // Outlined bar
    _display.drawRect(x, barTop, barW, barH, SSD1306_WHITE);

    // Filled portion from bottom up
    if (fillH > 0) {
      _display.fillRect(x + 1, barTop + barH - fillH, barW - 2, fillH,
                        SSD1306_WHITE);
    }
  }

  // --- Pass 2: Draw rotated percentage text ---
  // Rotation 3 (90° CCW): text reads bottom-to-top
  // Physical (px, py) from logical: px = ly, py = 31 - lx
  _display.setRotation(3);
  _display.setTextSize(1);

  for (uint8_t i = 0; i < numBars; i++) {
    uint8_t barX = gap + i * (barW + gap);
    float stock = _fert->getStockML(i);
    float pct = stock / maxStock;
    if (pct > 1.0f)
      pct = 1.0f;
    if (pct < 0.0f)
      pct = 0.0f;
    uint8_t fillH = (uint8_t)(pct * barH);
    int pctVal = (int)(pct * 100);

    char pctStr[5];
    snprintf(pctStr, sizeof(pctStr), "%d", pctVal);
    uint8_t len = strlen(pctStr);

    // Center text in bar area (barTop to SCREEN_HEIGHT)
    // In rotation 3: lx maps to physical y, ly maps to physical x
    // Physical bar spans y: barTop to SCREEN_HEIGHT-1
    // So lx range: (32-SCREEN_HEIGHT) to (32-barTop-1) = 0 to 21 for 32px
    // height
    uint8_t textPxH = len * 6;
    uint8_t lx = (SCREEN_HEIGHT - barTop - textPxH) / 2;
    uint8_t ly = barX + (barW - 8) / 2;

    // Text color: black on filled (>50%), white on empty
    if (fillH > barH / 2) {
      _display.setTextColor(SSD1306_BLACK);
    } else {
      _display.setTextColor(SSD1306_WHITE);
    }

    _display.setCursor(lx, ly);
    _display.print(pctStr);
  }

  _display.setRotation(0); // Restore normal rotation
  _display.setTextColor(SSD1306_WHITE);
}

// =============================================================================
// PAGE 4 — Schedules
// =============================================================================
void DisplayManager::_drawSchedulePage() {

  // Line 2: TPA schedule
  _display.setCursor(0, 10);
  _display.print(F("TPA:"));
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
    _display.print(F(" /"));
    _display.print(interval);
    _display.print(F("d"));
  }

  // Line 3: Current time
  _display.setCursor(0, 22);
  DateTime now = _time->now();
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
}

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
#ifdef USE_SSD1306
DisplayManager::DisplayManager()
    : _display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET), _time(nullptr),
      _water(nullptr), _fert(nullptr), _safety(nullptr), _web(nullptr),
      _currentPage(0), _lastPageSwitch(0) {}
#else
DisplayManager::DisplayManager()
    : _display(PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_MOSI, PIN_TFT_SCK, PIN_TFT_RST),
      _time(nullptr), _water(nullptr), _fert(nullptr), _safety(nullptr),
      _web(nullptr), _currentPage(0), _lastPageSwitch(0) {}
#endif

// =============================================================================
// INIT HARDWARE
// =============================================================================
bool DisplayManager::initHardware() {
#ifdef USE_SSD1306
  if (!_display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("[Display] SSD1306 init FAILED!");
    return false;
  }
  Serial.println("[Display] SSD1306 128x32 initialized OK.");

  _display.clearDisplay();
  _display.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
  _display.display();
  delay(500);

  _display.clearDisplay();
  _display.setTextColor(SSD1306_WHITE);
  _display.setTextSize(2);
  _display.setCursor(30, 0);
  _display.print(F("IARA"));
  _display.setTextSize(1);
  _display.setCursor(30, 20);
  _display.print(F("v3.0.0"));
  _display.display();

#else
  _display.initR(INITR_BLACKTAB);
  _display.setRotation(1); // Landscape: 160×128
  Serial.println("[Display] ST7735 128x160 initialized OK.");

  // Color splash screen
  _display.fillScreen(COL_BG);
  _display.setTextSize(3);
  _display.setTextColor(COL_ACCENT);
  uint8_t tw = 4 * 18; // "IARA" = 4 chars × 18px
  _display.setCursor((160 - tw) / 2, 30);
  _display.print(F("IARA"));

  _display.setTextSize(1);
  _display.setTextColor(COL_DIM);
  _display.setCursor(55, 70);
  _display.print(F("v3.0.0"));

  // Accent bar
  _display.fillRect(30, 90, 100, 2, COL_ACCENT);

  _display.setTextColor(COL_GOOD);
  _display.setCursor(35, 100);
  _display.print(F("Aquarium Control"));
#endif

  return true;
}

// =============================================================================
// SHOW BOOT STATUS
// =============================================================================
void DisplayManager::showBootStatus(const char *line1, const char *line2) {
#ifdef USE_SSD1306
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

#else
  _display.fillScreen(COL_BG);

  // Header bar
  _display.fillRect(0, 0, 160, 20, COL_ACCENT);
  _display.setTextSize(1);
  _display.setTextColor(COL_BG);
  _display.setCursor(4, 6);
  _display.print(F("IARA > Boot"));

  // Status text
  _display.setTextColor(COL_GOOD);
  _display.setTextSize(1);
  _display.setCursor(8, 40);
  _display.print(line1);
  if (line2) {
    _display.setCursor(8, 58);
    _display.setTextColor(COL_DIM);
    _display.print(line2);
  }
#endif
}

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

#ifdef USE_SSD1306
  _display.clearDisplay();
#else
  _display.fillScreen(COL_BG);
#endif

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

#ifdef USE_SSD1306
  _display.display();
#endif
  _currentPage = (_currentPage + 1) % NUM_PAGES;
}

// =============================================================================
// HEADER — water level bar + page title
// =============================================================================
void DisplayManager::_drawHeader(const char *title) {
  float dist = _safety->getLastDistance();
  const float maxDist = 30.0f;

#ifdef USE_SSD1306
  _display.setTextSize(1);
  _display.setTextColor(SSD1306_WHITE);

  if (dist >= 0) {
    float pct = 1.0f - (dist / maxDist);
    if (pct > 1.0f)
      pct = 1.0f;
    if (pct < 0.0f)
      pct = 0.0f;

    uint8_t barW = 84;
    _display.drawRect(0, 0, barW, 8, SSD1306_WHITE);
    uint8_t fillW = (uint8_t)(pct * (barW - 2));
    if (fillW > 0) {
      _display.fillRect(1, 1, fillW, 6, SSD1306_WHITE);
    }

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
    _display.setCursor(0, 0);
    _display.print(F("Nivel: --"));
  }

  uint8_t titleW = strlen(title) * 6;
  _display.setCursor(SCREEN_WIDTH - titleW, 0);
  _display.print(title);

#else
  // ── TFT Header: accent bar with title + water level bar ──
  _display.fillRect(0, 0, 160, 22, COL_ACCENT);
  _display.setTextSize(1);
  _display.setTextColor(COL_BG);
  _display.setCursor(4, 7);
  _display.print(title);

  // Water level bar on the right side of header
  if (dist >= 0) {
    float pct = 1.0f - (dist / maxDist);
    if (pct > 1.0f)
      pct = 1.0f;
    if (pct < 0.0f)
      pct = 0.0f;

    uint8_t barX = 80;
    uint8_t barW = 70;
    uint8_t barH = 10;
    uint8_t barY = 6;
    _display.drawRect(barX, barY, barW, barH, COL_BG);
    uint8_t fillW = (uint8_t)(pct * (barW - 2));
    uint16_t fillCol =
        pct > 0.5f ? COL_GOOD : (pct > 0.2f ? COL_WARN : COL_ERR);
    if (fillW > 0) {
      _display.fillRect(barX + 1, barY + 1, fillW, barH - 2, fillCol);
    }

    int pctVal = (int)(pct * 100);
    char buf[5];
    snprintf(buf, sizeof(buf), "%d%%", pctVal);
    uint8_t textW = strlen(buf) * 6;
    uint8_t tx = barX + (barW - textW) / 2;
    _display.setTextColor(COL_BG);
    _display.setCursor(tx, barY + 1);
    _display.print(buf);
  } else {
    _display.setCursor(80, 7);
    _display.setTextColor(COL_BG);
    _display.print(F("Nivel: --"));
  }

  // Thin separator line
  _display.drawFastHLine(0, 23, 160, COL_BAR_BG);
#endif
}

// =============================================================================
// PAGE 1 — Network Info
// =============================================================================
void DisplayManager::_drawNetworkPage() {
#ifdef USE_SSD1306
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

#else
  uint8_t y = 30;

  if (WiFi.status() == WL_CONNECTED) {
    // Status indicator
    _display.fillCircle(12, y + 4, 5, COL_GOOD);
    _display.setTextColor(COL_GOOD);
    _display.setTextSize(1);
    _display.setCursor(22, y);
    _display.print(F("CONECTADO"));

    // IP in large font
    _display.setTextSize(2);
    _display.setTextColor(COL_TEXT);
    y += 18;
    _display.setCursor(4, y);
    _display.print(WiFi.localIP());

    // SSID + RSSI
    _display.setTextSize(1);
    _display.setTextColor(COL_DIM);
    y += 22;
    _display.setCursor(4, y);
    _display.print(WiFi.SSID());

    y += 14;
    _display.setCursor(4, y);
    _display.print(F("RSSI: "));
    int rssi = WiFi.RSSI();
    _display.setTextColor(rssi > -60 ? COL_GOOD
                                     : (rssi > -80 ? COL_WARN : COL_ERR));
    _display.print(rssi);
    _display.print(F(" dBm"));
  } else {
    _display.fillCircle(12, y + 4, 5, COL_ERR);
    _display.setTextColor(COL_ERR);
    _display.setTextSize(1);
    _display.setCursor(22, y);
    _display.print(F("DESCONECTADO"));

    _display.setTextSize(2);
    _display.setTextColor(COL_TEXT);
    y += 20;
    _display.setCursor(4, y);
    _display.print(F("AP Mode"));

    _display.setTextSize(1);
    _display.setTextColor(COL_DIM);
    y += 22;
    _display.setCursor(4, y);
    _display.print(WiFi.softAPIP());
  }
#endif
}

// =============================================================================
// PAGE 2 — Aquarium Status
// =============================================================================
void DisplayManager::_drawAquariumPage() {
#ifdef USE_SSD1306
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

#else
  uint8_t y = 30;
  float dist = _safety->getLastDistance();

  // Water level — large text
  _display.setTextSize(1);
  _display.setTextColor(COL_DIM);
  _display.setCursor(4, y);
  _display.print(F("NIVEL DA AGUA"));

  y += 12;
  _display.setTextSize(2);
  _display.setTextColor(COL_TEXT);
  _display.setCursor(4, y);
  if (dist >= 0) {
    _display.print(dist, 1);
    _display.setTextSize(1);
    _display.print(F(" cm"));
  } else {
    _display.print(F("-- cm"));
  }

  // TPA state
  y += 26;
  _display.setTextSize(1);
  _display.setTextColor(COL_DIM);
  _display.setCursor(4, y);
  _display.print(F("TPA"));

  y += 12;
  const char *state = _water->getStateName();
  // Color code by state
  uint16_t stateCol = COL_GOOD; // IDLE default
  if (_water->isRunning())
    stateCol = COL_WARN;
  if (_water->getState() == TPAState::ERROR)
    stateCol = COL_ERR;

  _display.setTextSize(2);
  _display.setTextColor(stateCol);
  _display.setCursor(4, y);
  _display.print(state);

  // Canister status
  y += 24;
  _display.setTextSize(1);
  _display.setTextColor(COL_DIM);
  _display.setCursor(4, y);
  _display.print(F("CANISTER: "));
  bool canOn = _water->isCanisterOn();
  _display.setTextColor(canOn ? COL_GOOD : COL_ERR);
  _display.print(canOn ? F("ON") : F("OFF"));
#endif
}

// =============================================================================
// PAGE 3 — Fertilizer & Prime Stock
// =============================================================================
void DisplayManager::_drawStockPage() {
#ifdef USE_SSD1306
  _display.setTextSize(1);
  _display.setTextColor(SSD1306_WHITE);

  const uint8_t numBars = NUM_FERTS + 1;
  const uint8_t barW = 20;
  const uint8_t gap = (SCREEN_WIDTH - numBars * barW) / (numBars + 1);
  const uint8_t barTop = 10;
  const uint8_t barH = SCREEN_HEIGHT - barTop;
  const float maxStock = 500.0f;

  for (uint8_t i = 0; i < numBars; i++) {
    uint8_t x = gap + i * (barW + gap);
    float stock = _fert->getStockML(i);
    float pct = stock / maxStock;
    if (pct > 1.0f)
      pct = 1.0f;
    if (pct < 0.0f)
      pct = 0.0f;
    uint8_t fillH = (uint8_t)(pct * barH);

    _display.drawRect(x, barTop, barW, barH, SSD1306_WHITE);
    if (fillH > 0) {
      _display.fillRect(x + 1, barTop + barH - fillH, barW - 2, fillH,
                        SSD1306_WHITE);
    }
  }

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

    uint8_t textPxH = len * 6;
    uint8_t lx = (SCREEN_HEIGHT - barTop - textPxH) / 2;
    uint8_t ly = barX + (barW - 8) / 2;

    if (fillH > barH / 2) {
      _display.setTextColor(SSD1306_BLACK);
    } else {
      _display.setTextColor(SSD1306_WHITE);
    }

    _display.setCursor(lx, ly);
    _display.print(pctStr);
  }

  _display.setRotation(0);
  _display.setTextColor(SSD1306_WHITE);

#else
  // ── TFT Color Bar Chart ──
  const uint8_t numBars = NUM_FERTS + 1;
  const uint8_t barW = 22;
  const uint8_t gap = (160 - numBars * barW) / (numBars + 1);
  const uint8_t barTop = 30;
  const uint8_t barH = 75;
  const float maxStock = 500.0f;

  static const char *labels[] = {"F1", "F2", "F3", "F4", "PR"};

  for (uint8_t i = 0; i < numBars; i++) {
    uint8_t x = gap + i * (barW + gap);
    float stock = _fert->getStockML(i);
    float pct = stock / maxStock;
    if (pct > 1.0f)
      pct = 1.0f;
    if (pct < 0.0f)
      pct = 0.0f;
    uint8_t fillH = (uint8_t)(pct * barH);

    // Bar outline
    _display.drawRect(x, barTop, barW, barH, COL_BAR_BG);

    // Color-coded fill
    uint16_t fillCol =
        pct > 0.5f ? COL_GOOD : (pct > 0.2f ? COL_WARN : COL_ERR);
    if (fillH > 0) {
      _display.fillRect(x + 1, barTop + barH - fillH, barW - 2, fillH, fillCol);
    }

    // Percentage label above bar
    int pctVal = (int)(pct * 100);
    char buf[5];
    snprintf(buf, sizeof(buf), "%d%%", pctVal);
    uint8_t tw = strlen(buf) * 6;
    _display.setTextSize(1);
    _display.setTextColor(fillCol);
    _display.setCursor(x + (barW - tw) / 2, barTop - 10);
    _display.print(buf);

    // Channel label below bar
    _display.setTextColor(COL_DIM);
    uint8_t lw = strlen(labels[i]) * 6;
    _display.setCursor(x + (barW - lw) / 2, barTop + barH + 4);
    _display.print(labels[i]);
  }
#endif
}

// =============================================================================
// PAGE 4 — Schedules
// =============================================================================
void DisplayManager::_drawSchedulePage() {
#ifdef USE_SSD1306
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

#else
  uint8_t y = 32;

  // Current time — large
  DateTime now = _time->now();
  _display.setTextSize(3);
  _display.setTextColor(COL_TEXT);

  char timeBuf[9];
  snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", now.hour(), now.minute(),
           now.second());
  uint8_t tw = 8 * 18; // 8 chars × 18px (size 3)
  _display.setCursor((160 - tw) / 2, y);
  _display.print(timeBuf);

  // TPA schedule
  y += 36;
  _display.setTextSize(1);
  _display.setTextColor(COL_DIM);
  _display.setCursor(4, y);
  _display.print(F("PROXIMA TPA"));

  y += 14;
  uint8_t th = _web->getTpaHour();
  uint8_t tm = _web->getTpaMinute();
  uint16_t interval = _web->getTpaInterval();

  _display.setTextSize(2);
  _display.setTextColor(COL_ACCENT);
  _display.setCursor(4, y);

  char tpaBuf[12];
  snprintf(tpaBuf, sizeof(tpaBuf), "%02d:%02d", th, tm);
  _display.print(tpaBuf);

  if (interval > 0) {
    _display.setTextSize(1);
    _display.setTextColor(COL_DIM);
    _display.setCursor(70, y + 4);
    _display.print(F("/ "));
    _display.print(interval);
    _display.print(F(" dias"));
  }
#endif
}

#pragma once

#include "Config.h"
#include <Adafruit_GFX.h>
#include <Arduino.h>

#ifdef USE_SSD1306
// ── Wokwi: keep SSD1306 OLED (I2C) ──
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#else
// ── Real hardware: ST7735 TFT (SPI) ──
#include <Adafruit_ST7735.h>
#include <SPI.h>
#endif

// Forward declarations
class TimeManager;
class WaterManager;
class FertManager;
class SafetyWatchdog;
class WebManager;

/// @brief Manages the display with auto-cycling pages.
/// Compiles for either SSD1306 (Wokwi) or ST7735 (real hardware).
class DisplayManager {
public:
  DisplayManager();

  /// Early hardware init — call before WiFi to show boot screen immediately
  bool initHardware();

  /// Show a boot progress line on the display (call during setup steps)
  void showBootStatus(const char *line1, const char *line2 = nullptr);

  /// Full initialization with manager pointers (call after all managers ready)
  void begin(TimeManager *time, WaterManager *water, FertManager *fert,
             SafetyWatchdog *safety, WebManager *web);

  /// Update display — call from loop(). Cycles pages every PAGE_CYCLE_MS.
  void update();

private:
#ifdef USE_SSD1306
  Adafruit_SSD1306 _display;
  static constexpr uint8_t SCREEN_WIDTH = 128;
  static constexpr uint8_t SCREEN_HEIGHT = 32;
  static constexpr int8_t OLED_RESET = -1;
  static constexpr uint8_t OLED_ADDR = 0x3C;
#else
  Adafruit_ST7735 _display;
  static constexpr uint8_t SCREEN_WIDTH = 128;
  static constexpr uint8_t SCREEN_HEIGHT = 160;

  // ── Color Palette ──
  static constexpr uint16_t COL_BG = 0x0000;     // Black
  static constexpr uint16_t COL_TEXT = 0xFFFF;   // White
  static constexpr uint16_t COL_DIM = 0xAD55;    // Light gray
  static constexpr uint16_t COL_ACCENT = 0x04FF; // Cyan-blue
  static constexpr uint16_t COL_GOOD = 0x07E0;   // Green
  static constexpr uint16_t COL_WARN = 0xFFE0;   // Yellow
  static constexpr uint16_t COL_ERR = 0xF800;    // Red
  static constexpr uint16_t COL_BAR_BG = 0x4208; // Dark gray
#endif

  // Manager pointers
  TimeManager *_time;
  WaterManager *_water;
  FertManager *_fert;
  SafetyWatchdog *_safety;
  WebManager *_web;

  // Page cycling
  uint8_t _currentPage;
  unsigned long _lastPageSwitch;
  static constexpr uint8_t NUM_PAGES = 4;
  static constexpr unsigned long PAGE_CYCLE_MS = 5000; // 5 seconds

  // Page drawing methods
  void _drawNetworkPage();
  void _drawAquariumPage();
  void _drawStockPage();
  void _drawSchedulePage();

  // Helper
  void _drawHeader(const char *title);
};

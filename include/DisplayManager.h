#pragma once

#include "Config.h"
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

// Forward declarations
class TimeManager;
class WaterManager;
class FertManager;
class SafetyWatchdog;
class WebManager;

/// @brief Manages the SSD1306 OLED display with auto-cycling pages.
class DisplayManager {
public:
  DisplayManager();

  /// Initialize display hardware (I2C address 0x3C)
  void begin(TimeManager *time, WaterManager *water, FertManager *fert,
             SafetyWatchdog *safety, WebManager *web);

  /// Update display — call from loop(). Cycles pages every PAGE_CYCLE_MS.
  void update();

private:
  Adafruit_SSD1306 _display;

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

  // Display dimensions
  static constexpr uint8_t SCREEN_WIDTH = 128;
  static constexpr uint8_t SCREEN_HEIGHT = 64;
  static constexpr int8_t OLED_RESET = -1; // No reset pin
  static constexpr uint8_t OLED_ADDR = 0x3C;

  // Page drawing methods
  void _drawNetworkPage();
  void _drawAquariumPage();
  void _drawStockPage();
  void _drawSchedulePage();

  // Helper
  void _drawHeader(const char *title);
};

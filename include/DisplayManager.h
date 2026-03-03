#pragma once

#include "Config.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
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

  /// Early hardware init — call before WiFi to show boot screen immediately
  bool initHardware();

  /// Show a boot progress line on the OLED (call during setup steps)
  void showBootStatus(const char *line1, const char *line2 = nullptr);

  /// Full initialization with manager pointers (call after all managers ready)
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
  static constexpr uint8_t SCREEN_HEIGHT = 32;
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

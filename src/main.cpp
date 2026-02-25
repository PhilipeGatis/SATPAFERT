// =============================================================================
// AQUARIUM AUTOMATION FIRMWARE - ESP32
// =============================================================================
// TPA (Troca Parcial de Água), Fertilization & Filtration Controller
// Priority: Flood prevention and temporal precision
//
// Architecture: OOP with 5 manager classes
//   - SafetyWatchdog: sensor reads, overflow detection, emergency actions
//   - TimeManager:    RTC DS3231 + NTP synchronization
//   - WaterManager:   TPA state machine (6 states)
//   - FertManager:    Daily dosing with NVS deduplication
//   - WebManager:     Embedded web dashboard + Serial command interface
// =============================================================================

#include "Config.h"
#include "FertManager.h"
#include "SafetyWatchdog.h"
#include "TimeManager.h"
#include "WaterManager.h"
#include "WebManager.h"
#include <Arduino.h>

// ---- Global instances ----
SafetyWatchdog safety;
TimeManager timeMgr;
WaterManager waterMgr;
FertManager fertMgr;
WebManager webMgr;

// ---- Scheduling state ----
bool fertDoneThisMinute = false; // Prevent re-triggering within same minute
uint8_t lastFertMinute = 255;
bool tpaDoneThisMinute = false;
uint8_t lastTPAMinute = 255;

// =============================================================================
// SETUP
// =============================================================================
void setup() {
  // --- Step 1: Initialize all output pins LOW FIRST (safety critical) ---
  for (uint8_t i = 0; i < NUM_OUTPUT_PINS; i++) {
    pinMode(OUTPUT_PINS[i], OUTPUT);
    digitalWrite(OUTPUT_PINS[i], LOW);
  }

  // --- Step 2: Serial ---
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n==========================================");
  Serial.println("  AQUARIUM AUTOMATION - ESP32 Firmware");
  Serial.println("  v3.0.0 - Web Dashboard");
  Serial.println("==========================================\n");

  // --- Step 3: WiFi (must be before NTP/WebServer) ---
  Serial.printf("[WiFi] SSID: '%s'\n", WIFI_SSID);
  Serial.printf("[WiFi] PASS: '%s' (len=%d)\n", WIFI_PASSWORD,
                strlen(WIFI_PASSWORD));
  Serial.print("[WiFi] Connecting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  {
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 40) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" OK!");
    Serial.print("[WiFi] IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(" FAILED — starting AP mode");
    WiFi.mode(WIFI_AP_STA); // AP + keep trying STA in background
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    Serial.printf("[WiFi] AP started: SSID='%s' PASS='%s'\n", AP_SSID,
                  AP_PASSWORD);
    Serial.print("[WiFi] AP IP: ");
    Serial.println(WiFi.softAPIP());
    // Removed immediate WiFi.begin() because it disrupts the SoftAP network.
  }

  // --- Step 4: Safety Watchdog (sensors) ---
  safety.begin();

  // --- Step 5: Time Manager (RTC + NTP — needs WiFi) ---
  timeMgr.begin();

  // --- Step 5: Fertilizer Manager (NVS state) ---
  fertMgr.begin();

  // --- Step 6: Water Manager (TPA state machine) ---
  waterMgr.begin(&safety, &fertMgr);

  // --- Step 7: Web Dashboard + Serial UI ---
  webMgr.begin(&timeMgr, &waterMgr, &fertMgr, &safety);

  // --- Step 8: Canister filter ON by default ---
  digitalWrite(PIN_CANISTER, HIGH);
  Serial.println("[Main] Canister filter ON (default).");

  Serial.println("\n[Main] === System Ready ===\n");
}

// =============================================================================
// LOOP
// =============================================================================
void loop() {
  // ---- 1. SAFETY (highest priority, runs every 500ms) ----
  safety.update();

  // If in emergency, skip all scheduling and just process commands
  if (safety.isEmergency()) {
    webMgr.processSerialCommands();
    webMgr.update(); // keep web server alive
    delay(100);
    return;
  }

  // ---- 2. TIME SYNC (periodic NTP re-sync) ----
  timeMgr.update();

  // ---- 3. SERIAL COMMANDS + WEB ----
  webMgr.processSerialCommands();

  // ---- 4. SCHEDULING (only if not in maintenance and not running TPA) ----
  if (!safety.isMaintenanceMode()) {
    DateTime now = timeMgr.now();
    uint8_t currentMinute = now.minute();

    // --- Fertilization schedule ---
    uint8_t fertH = webMgr.getFertHour();
    uint8_t fertM = webMgr.getFertMinute();

    // Reset trigger flag when minute changes
    if (currentMinute != lastFertMinute) {
      fertDoneThisMinute = false;
      lastFertMinute = currentMinute;
    }

    if (!fertDoneThisMinute && !waterMgr.isRunning()) {
      if (timeMgr.isDailyScheduleTime(fertH, fertM)) {
        Serial.println("[Main] Fertilization schedule triggered!");
        fertMgr.checkAndDose(now, fertH, fertM);
        fertDoneThisMinute = true;
      }
    }

    // --- TPA schedule ---
    uint8_t tpaD = webMgr.getTPADay();
    uint8_t tpaH = webMgr.getTPAHour();
    uint8_t tpaM = webMgr.getTPAMinute();

    if (currentMinute != lastTPAMinute) {
      tpaDoneThisMinute = false;
      lastTPAMinute = currentMinute;
    }

    if (!tpaDoneThisMinute && !waterMgr.isRunning()) {
      if (timeMgr.isWeeklyScheduleDay(tpaD, tpaH, tpaM)) {
        Serial.println("[Main] TPA schedule triggered!");
        waterMgr.startTPA();
        tpaDoneThisMinute = true;
      }
    }
  }

  // ---- 5. TPA STATE MACHINE ----
  waterMgr.update();

  // If TPA just completed, record timestamp
  if (waterMgr.getState() == TPAState::COMPLETE) {
    waterMgr.setLastTPATime(timeMgr.getFormattedTime());
  }

  // ---- 6. WEB DASHBOARD + TELEMETRY ----
  webMgr.update();

  // ---- 7. YIELD ----
  delay(50); // ~20 Hz loop, fast enough for safety, gentle on CPU
}

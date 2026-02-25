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
//   - BlynkManager:   Blynk IoT app + Serial command interface
// =============================================================================

#include "BlynkManager.h"
#include "Config.h"
#include "FertManager.h"
#include "SafetyWatchdog.h"
#include "TimeManager.h"
#include "WaterManager.h"
#include <Arduino.h>

// ---- Global instances ----
SafetyWatchdog safety;
TimeManager timeMgr;
WaterManager waterMgr;
FertManager fertMgr;
BlynkManager blynkMgr;

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
  Serial.println("  v2.0.0 - Blynk IoT");
  Serial.println("==========================================\n");

  // --- Step 3: WiFi (must be before NTP/Blynk) ---
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
    Serial.println(" FAILED (will retry)");
  }

  // --- Step 4: Safety Watchdog (sensors) ---
  safety.begin();

  // --- Step 5: Time Manager (RTC + NTP — needs WiFi) ---
  timeMgr.begin();

  // --- Step 5: Fertilizer Manager (NVS state) ---
  fertMgr.begin();

  // --- Step 6: Water Manager (TPA state machine) ---
  waterMgr.begin(&safety, &fertMgr);

  // --- Step 7: Blynk IoT + Serial UI (also handles WiFi) ---
  blynkMgr.begin(&timeMgr, &waterMgr, &fertMgr, &safety);

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
    blynkMgr.processSerialCommands();
    blynkMgr.update(); // keep Blynk alive for notifications
    delay(100);
    return;
  }

  // ---- 2. TIME SYNC (periodic NTP re-sync) ----
  timeMgr.update();

  // ---- 3. SERIAL COMMANDS + BLYNK ----
  blynkMgr.processSerialCommands();

  // ---- 4. SCHEDULING (only if not in maintenance and not running TPA) ----
  if (!safety.isMaintenanceMode()) {
    DateTime now = timeMgr.now();
    uint8_t currentMinute = now.minute();

    // --- Fertilization schedule ---
    uint8_t fertH = blynkMgr.getFertHour();
    uint8_t fertM = blynkMgr.getFertMinute();

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
    uint8_t tpaD = blynkMgr.getTPADay();
    uint8_t tpaH = blynkMgr.getTPAHour();
    uint8_t tpaM = blynkMgr.getTPAMinute();

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

  // ---- 6. BLYNK + TELEMETRY ----
  blynkMgr.update();

  // ---- 7. YIELD ----
  delay(50); // ~20 Hz loop, fast enough for safety, gentle on CPU
}

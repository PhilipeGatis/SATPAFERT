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
#include <esp_wifi.h>

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
  Preferences wifiPref;
  wifiPref.begin("wifi", true); // true = readonly
  String savedSSID = wifiPref.getString("ssid", String(WIFI_SSID));
  String savedPass = wifiPref.getString("pass", String(WIFI_PASSWORD));
  wifiPref.end();

  Serial.printf("[WiFi] SSID: '%s'\n", savedSSID.c_str());
  Serial.printf("[WiFi] PASS: len=%d\n", savedPass.length());
  Serial.print("[WiFi] Connecting");

  // Add Event Listener to catch specific disconnect reasons
  WiFi.onEvent([](arduino_event_id_t event, arduino_event_info_t info) {
    if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
      Serial.printf("\n[WiFi Event] Disconnected! Reason Code: %d\n",
                    info.wifi_sta_disconnected.reason);
    }
  });

  WiFi.mode(WIFI_STA);         // Set STA mode first
  WiFi.disconnect(true, true); // Clean previous connections
  delay(500);

  // Advanced Router Compatibility Fixes (for TIM / Smart Routers)
  WiFi.setSleep(
      false); // Disable sleep for better compatibility with some routers
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);

  // Active Scan Workaround: Force the radio to wake up, populate the BSSID
  // cache, and extract the exact BSSID/Channel to avoid Reason 201 (NO AP FOUND
  // / Band Steering)
  Serial.print(" Scanning...");
  int n = WiFi.scanNetworks();
  delay(100);

  uint8_t targetBSSID[6] = {0};
  int32_t targetChannel = 0;
  bool bssidFound = false;

  for (int i = 0; i < n; ++i) {
    if (WiFi.SSID(i) == savedSSID) {
      memcpy(targetBSSID, WiFi.BSSID(i), 6);
      targetChannel = WiFi.channel(i);
      bssidFound = true;
      Serial.printf("\n[WiFi] Found Target BSSID: "
                    "%02X:%02X:%02X:%02X:%02X:%02X on Channel: %d\n",
                    targetBSSID[0], targetBSSID[1], targetBSSID[2],
                    targetBSSID[3], targetBSSID[4], targetBSSID[5],
                    targetChannel);
      break;
    }
  }

  // CRITICAL FIX: To prevent Reason 15 (4-way handshake timeout) on
  // TIM routers we need to set the WiFi config explicitly to disable Protected
  // Management Frames (PMF) because the ESP32 WPA3 implementation sometimes
  // fails to negotiate with modern mixed-mode routers.

  wifi_config_t wifi_config = {};
  strlcpy((char *)wifi_config.sta.ssid, savedSSID.c_str(),
          sizeof(wifi_config.sta.ssid));
  strlcpy((char *)wifi_config.sta.password, savedPass.c_str(),
          sizeof(wifi_config.sta.password));

  // Force WPA2 and Disable PMF completely to bypass the handshake hang
  wifi_config.sta.pmf_cfg.capable = false;
  wifi_config.sta.pmf_cfg.required = false;

  esp_wifi_set_config(WIFI_IF_STA, &wifi_config);

  // Re-attempt standard connection, locking to BSSID and Channel if found
  if (bssidFound) {
    WiFi.begin(savedSSID.c_str(), savedPass.c_str(), targetChannel, targetBSSID,
               true);
  } else {
    WiFi.begin(savedSSID.c_str(), savedPass.c_str());
  }

  {
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED &&
           attempts < 60) { // Incremented timeout (Reason 39)
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
    Serial.println(" FAILED!");
    Serial.print("[WiFi] Error Code: ");
    switch (WiFi.status()) {
    case WL_NO_SHIELD:
      Serial.println("WL_NO_SHIELD");
      break;
    case WL_IDLE_STATUS:
      Serial.println("WL_IDLE_STATUS");
      break;
    case WL_NO_SSID_AVAIL:
      Serial.println("WL_NO_SSID_AVAIL");
      break;
    case WL_SCAN_COMPLETED:
      Serial.println("WL_SCAN_COMPLETED");
      break;
    case WL_CONNECT_FAILED:
      Serial.println("WL_CONNECT_FAILED");
      break;
    case WL_CONNECTION_LOST:
      Serial.println("WL_CONNECTION_LOST");
      break;
    case WL_DISCONNECTED:
      Serial.println("WL_DISCONNECTED");
      break;
    default:
      Serial.printf("UNKNOWN (%d)\n", WiFi.status());
      break;
    }

    Serial.println("[WiFi] Starting AP mode fallback");
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

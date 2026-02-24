#pragma once
// ============================================================================
// WiFiProv.h Mock
// ============================================================================
#include "Arduino.h"

#define WIFI_PROV_SCHEME_BLE 0
#define WIFI_PROV_SCHEME_HANDLER_FREE_BTDM 0
#define WIFI_PROV_SECURITY_1 0

class MockWiFiProv {
public:
  void beginProvision(int scheme, int handler, int security, const char *pop,
                      const char *name) {}
};

extern MockWiFiProv WiFiProv;

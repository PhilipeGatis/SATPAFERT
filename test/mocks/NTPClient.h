#pragma once
// ============================================================================
// NTPClient.h Mock for Native Unit Tests
// ============================================================================

#include "Arduino.h"
#include "WiFiUdp.h"

class NTPClient {
public:
  NTPClient(WiFiUDP &udp, const char *server = "pool.ntp.org", long offset = 0)
      : _offset(offset), _epoch(0) {}

  void begin() {}
  void setTimeOffset(long offset) { _offset = offset; }
  bool update() { return true; }
  unsigned long getEpochTime() { return _epoch + _offset; }
  String getFormattedTime() { return String("12:00:00"); }

  // Mock control
  void mock_setEpoch(unsigned long epoch) { _epoch = epoch; }

private:
  long _offset;
  unsigned long _epoch;
};

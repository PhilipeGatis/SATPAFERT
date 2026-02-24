#pragma once
// ============================================================================
// Wire.h Mock for Native Unit Tests
// ============================================================================

class MockWire {
public:
  void begin() {}
  void begin(int sda, int scl) {}
};

extern MockWire Wire;

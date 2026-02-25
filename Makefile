.PHONY: all build-front upload-fs upload-fw monitor clean

# Default target runs everything in order
all: build-front upload-fs upload-fw

# 1. Build the React frontend using Vite
build-front:
	@echo "==> Building React Frontend..."
	cd frontend && npm run build

# 2. Upload the React static files to ESP32 LittleFS
upload-fs:
	@echo "==> Uploading LittleFS image (React SPA)..."
	pio run -e esp32dev -t uploadfs

# 3. Build and upload the C++ Firmware
upload-fw:
	@echo "==> Uploading C++ Firmware..."
	pio run -e esp32dev -t upload

# Open the Serial Monitor
monitor:
	pio device monitor

# Clean PlatformIO and Frontend builds
clean:
	pio run -t clean
	rm -rf data/*

#!/bin/bash
# ============================================================================
# Code Coverage Report Generator
# Usage: ./scripts/coverage.sh
# ============================================================================

set -e

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$PROJECT_DIR/.pio/build/coverage"
COVERAGE_DIR="$PROJECT_DIR/coverage"

echo "=== Code Coverage Report ==="
echo ""

# Step 1: Clean previous coverage data
echo "[1/5] Cleaning previous coverage data..."
find "$BUILD_DIR" -name "*.gcda" -delete 2>/dev/null || true
rm -rf "$COVERAGE_DIR"

# Step 2: Run tests with coverage env
echo "[2/5] Running tests with coverage instrumentation..."
cd "$PROJECT_DIR"
pio test -e coverage

# Step 3: Capture coverage data
echo "[3/5] Capturing coverage data with lcov..."
lcov --capture \
    --directory "$BUILD_DIR" \
    --output-file "$BUILD_DIR/coverage_raw.info" \
    --ignore-errors inconsistent,gcov,format \
    --rc lcov_branch_coverage=1

# Step 4: Filter — keep only project source files (src/ and include/)
echo "[4/5] Filtering coverage to project sources..."
lcov --extract "$BUILD_DIR/coverage_raw.info" \
    "$PROJECT_DIR/src/*" \
    "$PROJECT_DIR/include/*" \
    --output-file "$BUILD_DIR/coverage.info" \
    --ignore-errors unused,inconsistent \
    --rc lcov_branch_coverage=1

# Step 5: Generate HTML report
echo "[5/5] Generating HTML report..."
mkdir -p "$COVERAGE_DIR"
genhtml "$BUILD_DIR/coverage.info" \
    --output-directory "$COVERAGE_DIR" \
    --title "SATPAFERT - Aquarium Firmware Coverage" \
    --legend \
    --branch-coverage \
    --ignore-errors inconsistent,category \
    --rc lcov_branch_coverage=1

echo ""
echo "=== Coverage Summary ==="
lcov --summary "$BUILD_DIR/coverage.info" \
    --rc lcov_branch_coverage=1 2>&1 || true

echo ""
echo "HTML report: $COVERAGE_DIR/index.html"
echo "Open with: open $COVERAGE_DIR/index.html"

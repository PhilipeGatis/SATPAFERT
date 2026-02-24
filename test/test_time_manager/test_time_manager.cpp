// ============================================================================
// TimeManager Unit Tests
// Tests: DateTime validation, schedule matching logic, time formatting
// NOTE: These tests validate the scheduling logic through DateTime directly,
//       without linking TimeManager.cpp (which requires I2C/NTP stubs).
// ============================================================================

#include "Arduino.h"
#include "RTClib.h" // DateTime mock
#include <unity.h>

void setUp() {
  mock_reset_pins();
  mock_millis_value = 0;
}

void tearDown() {}

// Helper: simulate isDailyScheduleTime logic (same as TimeManager)
bool isDailyScheduleTime(DateTime dt, uint8_t hour, uint8_t minute) {
  return (dt.hour() == hour && dt.minute() == minute);
}

// Helper: simulate isWeeklyScheduleDay logic (same as TimeManager)
bool isWeeklyScheduleDay(DateTime dt, uint8_t dayOfWeek, uint8_t hour,
                         uint8_t minute) {
  return (dt.dayOfTheWeek() == dayOfWeek && dt.hour() == hour &&
          dt.minute() == minute);
}

// ----------------------------------------------------------------------------
// DateTime (mock) Tests — verify mock is correct before using it
// ----------------------------------------------------------------------------

void test_datetime_components() {
  DateTime dt(2026, 2, 24, 9, 30, 45);
  TEST_ASSERT_EQUAL(2026, dt.year());
  TEST_ASSERT_EQUAL(2, dt.month());
  TEST_ASSERT_EQUAL(24, dt.day());
  TEST_ASSERT_EQUAL(9, dt.hour());
  TEST_ASSERT_EQUAL(30, dt.minute());
  TEST_ASSERT_EQUAL(45, dt.second());
}

void test_datetime_day_of_week_sunday() {
  // Feb 22, 2026 is a Sunday (0)
  DateTime dt(2026, 2, 22, 0, 0, 0);
  TEST_ASSERT_EQUAL(0, dt.dayOfTheWeek());
}

void test_datetime_day_of_week_tuesday() {
  // Feb 24, 2026 is a Tuesday (2)
  DateTime dt(2026, 2, 24, 0, 0, 0);
  TEST_ASSERT_EQUAL(2, dt.dayOfTheWeek());
}

void test_datetime_day_of_week_saturday() {
  // Feb 28, 2026 is a Saturday (6)
  DateTime dt(2026, 2, 28, 0, 0, 0);
  TEST_ASSERT_EQUAL(6, dt.dayOfTheWeek());
}

// ----------------------------------------------------------------------------
// Schedule: Daily
// ----------------------------------------------------------------------------

void test_daily_schedule_match() {
  DateTime dt(2026, 2, 24, 9, 0, 0);
  TEST_ASSERT_TRUE(isDailyScheduleTime(dt, 9, 0));
}

void test_daily_schedule_no_match_wrong_hour() {
  DateTime dt(2026, 2, 24, 10, 0, 0);
  TEST_ASSERT_FALSE(isDailyScheduleTime(dt, 9, 0));
}

void test_daily_schedule_no_match_wrong_minute() {
  DateTime dt(2026, 2, 24, 9, 1, 0);
  TEST_ASSERT_FALSE(isDailyScheduleTime(dt, 9, 0));
}

void test_daily_schedule_midnight() {
  DateTime dt(2026, 2, 24, 0, 0, 0);
  TEST_ASSERT_TRUE(isDailyScheduleTime(dt, 0, 0));
}

void test_daily_schedule_end_of_day() {
  DateTime dt(2026, 2, 24, 23, 59, 0);
  TEST_ASSERT_TRUE(isDailyScheduleTime(dt, 23, 59));
}

// ----------------------------------------------------------------------------
// Schedule: Weekly
// ----------------------------------------------------------------------------

void test_weekly_schedule_match() {
  // Tuesday Feb 24, 2026 at 10:00
  DateTime dt(2026, 2, 24, 10, 0, 0);
  TEST_ASSERT_TRUE(isWeeklyScheduleDay(dt, 2, 10, 0)); // Tuesday=2
}

void test_weekly_schedule_wrong_day() {
  // Tuesday but schedule is for Sunday
  DateTime dt(2026, 2, 24, 10, 0, 0);
  TEST_ASSERT_FALSE(isWeeklyScheduleDay(dt, 0, 10, 0)); // Sunday=0
}

void test_weekly_schedule_right_day_wrong_time() {
  DateTime dt(2026, 2, 24, 11, 0, 0);
  TEST_ASSERT_FALSE(isWeeklyScheduleDay(dt, 2, 10, 0));
}

void test_weekly_schedule_sunday() {
  // Feb 22, 2026 is Sunday
  DateTime dt(2026, 2, 22, 10, 0, 0);
  TEST_ASSERT_TRUE(isWeeklyScheduleDay(dt, 0, 10, 0));
}

// ----------------------------------------------------------------------------
// Time Formatting
// ----------------------------------------------------------------------------

void test_format_time() {
  DateTime dt(2026, 2, 24, 9, 5, 3);
  char buf[22];
  snprintf(buf, sizeof(buf), "%04d/%02d/%02d %02d:%02d:%02d", dt.year(),
           dt.month(), dt.day(), dt.hour(), dt.minute(), dt.second());

  TEST_ASSERT_EQUAL_STRING("2026/02/24 09:05:03", buf);
}

void test_format_time_midnight() {
  DateTime dt(2026, 1, 1, 0, 0, 0);
  char buf[22];
  snprintf(buf, sizeof(buf), "%04d/%02d/%02d %02d:%02d:%02d", dt.year(),
           dt.month(), dt.day(), dt.hour(), dt.minute(), dt.second());

  TEST_ASSERT_EQUAL_STRING("2026/01/01 00:00:00", buf);
}

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char **argv) {
  UNITY_BEGIN();

  // DateTime mock validation
  RUN_TEST(test_datetime_components);
  RUN_TEST(test_datetime_day_of_week_sunday);
  RUN_TEST(test_datetime_day_of_week_tuesday);
  RUN_TEST(test_datetime_day_of_week_saturday);

  // Daily schedule
  RUN_TEST(test_daily_schedule_match);
  RUN_TEST(test_daily_schedule_no_match_wrong_hour);
  RUN_TEST(test_daily_schedule_no_match_wrong_minute);
  RUN_TEST(test_daily_schedule_midnight);
  RUN_TEST(test_daily_schedule_end_of_day);

  // Weekly schedule
  RUN_TEST(test_weekly_schedule_match);
  RUN_TEST(test_weekly_schedule_wrong_day);
  RUN_TEST(test_weekly_schedule_right_day_wrong_time);
  RUN_TEST(test_weekly_schedule_sunday);

  // Formatting
  RUN_TEST(test_format_time);
  RUN_TEST(test_format_time_midnight);

  UNITY_END();
  return 0;
}

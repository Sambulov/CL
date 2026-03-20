#include "CodeLib.h"

static uint32_t _days_since_1970(int16_t year, uint8_t month, uint8_t day) {
  uint32_t days = 0;
  for (int16_t y = 1970; y < year; y++)
    days += is_leap_year(y) ? 366 : 365;
  for (uint8_t m = 1; m < month; m++)
    days += days_in_month(year, m);
  return days + (day - 1);
}

timestamp_t datetime_to_timestamp(datetime_t *dt) {
  if(!dt) return (timestamp_t){0, 0};
  uint32_t total_seconds_local = 
    dt->seconds + 
    dt->minutes * 60 + 
    dt->hours * 3600;
  int32_t timezone_offset = 
    dt->time_zone_hours * 3600 + 
    dt->time_zone_minutes * 60;
  int32_t total_seconds_utc = total_seconds_local - timezone_offset;
  uint32_t days = _days_since_1970(dt->year, dt->month, dt->day);
  if (total_seconds_utc < 0) {
    days -= 1;
    total_seconds_utc += SECONDS_IN_DAY;
  } else if (total_seconds_utc >= (SECONDS_IN_DAY)) {
    days += 1;
    total_seconds_utc -= SECONDS_IN_DAY;
  }
  return (timestamp_t){
    .time = days * SECONDS_IN_DAY + total_seconds_utc,
    .timezone = (dt->time_zone_hours * 3600) + (dt->time_zone_minutes * 60)
  };
}

void timestamp_to_datetime(timestamp_t ts, datetime_t *dt) {
  mem_set(dt, 0, sizeof(datetime_t));
  uint32_t days = ts.time / SECONDS_IN_DAY;
  int32_t seconds_in_day = ts.time % SECONDS_IN_DAY;
  {
    dt->year = 1970;
    uint32_t days_in_year;
    while (days >= (days_in_year = (is_leap_year(dt->year) ? 366 : 365))) {
        days -= days_in_year;
        dt->year++;
    }
  }
  dt->month = 1;
  uint8_t month_days;
  while (days >= (month_days = days_in_month(dt->year, dt->month))) {
    days -= month_days;
    dt->month++;
  }
  dt->day = days + 1;
  dt->seconds = seconds_in_day % 60;
  seconds_in_day /= 60;
  dt->minutes = seconds_in_day % 60;
  seconds_in_day /= 60;
  dt->hours = seconds_in_day;
  dt->week_day = (4 + _days_since_1970(dt->year, dt->month, dt->day)) % 7;
  dt->time_zone_hours = ts.timezone / 3600;
  dt->time_zone_minutes = (ts.timezone % 3600 / 60);
}

#ifndef DATE_TIME_H_INCLUDED
#define DATE_TIME_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#define SECONDS_IN_DAY           86400
#define SECONDS_IN_HOUR          3600

typedef struct {
  uint8_t seconds;
  uint8_t minutes;
  uint8_t hours;
  uint8_t day;
  uint8_t month;
  uint16_t year;
  uint8_t week_day;
  int8_t time_zone_hours;
  int8_t time_zone_minutes;
} datetime_t;

typedef struct {
  uint32_t time;
  int32_t timezone;
} timestamp_t;

static inline uint8_t is_leap_year(uint16_t year) {
  return (!(year & 3) && (year % 100)) || !(year % 400);
}

static inline uint8_t days_in_month(uint16_t year, uint8_t month) {
  static const uint8_t days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  if((month - 1) > 11) return 0;
  if ((month == 2) && is_leap_year(year))
    return 29;
  return days[month - 1];
}

timestamp_t datetime_to_timestamp(datetime_t *dt);
void timestamp_to_datetime(timestamp_t ts, datetime_t *dt);


#ifdef __cplusplus
}
#endif

#endif /* DATE_TIME_H_INCLUDED */

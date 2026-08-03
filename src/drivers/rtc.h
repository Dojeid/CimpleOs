#ifndef RTC_H
#define RTC_H

#include <stdint.h>

#define RTC_ADDR_PORT 0x70
#define RTC_DATA_PORT 0x71

#define RTC_REG_SECONDS   0x00
#define RTC_REG_MINUTES   0x02
#define RTC_REG_HOURS     0x04
#define RTC_REG_DAY       0x07
#define RTC_REG_MONTH     0x08
#define RTC_REG_YEAR      0x09
#define RTC_REG_STATUS_A  0x0A
#define RTC_REG_STATUS_B  0x0B

typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} rtc_time_t;

void rtc_init(void);
void rtc_read(rtc_time_t* t);
uint32_t rtc_get_unix_epoch(void);
const char* rtc_day_of_week_str(rtc_time_t* t);
const char* rtc_month_str(rtc_time_t* t);
void rtc_format_time(rtc_time_t* t, char* buf);   // "HH:MM:SS" into buf[9+]
void rtc_format_date(rtc_time_t* t, char* buf);   // "DD Mon YYYY" into buf[16+]

#endif // RTC_H

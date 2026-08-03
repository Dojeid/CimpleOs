#include "drivers/rtc.h"
#include "lib/printf.h"
#include <stdint.h>

extern void outb(uint16_t port, uint8_t val);
extern uint8_t inb(uint16_t port);

static uint8_t rtc_read_reg(uint8_t reg) {
    outb(RTC_ADDR_PORT, 0x80 | reg);
    return inb(RTC_DATA_PORT);
}

static void rtc_wait_update(void) {
    while (rtc_read_reg(RTC_REG_STATUS_A) & 0x80) {
        // Wait
    }
}

static uint8_t bcd_to_bin(uint8_t val) {
    return ((val >> 4) * 10) + (val & 0x0F);
}

void rtc_read(rtc_time_t* t) {
    uint8_t status_b;
    uint8_t last_seconds, last_minutes, last_hours, last_day, last_month, last_year;
    
    rtc_wait_update();
    
    t->seconds = rtc_read_reg(RTC_REG_SECONDS);
    t->minutes = rtc_read_reg(RTC_REG_MINUTES);
    t->hours = rtc_read_reg(RTC_REG_HOURS);
    t->day = rtc_read_reg(RTC_REG_DAY);
    t->month = rtc_read_reg(RTC_REG_MONTH);
    t->year = rtc_read_reg(RTC_REG_YEAR);
    
    do {
        last_seconds = t->seconds;
        last_minutes = t->minutes;
        last_hours = t->hours;
        last_day = t->day;
        last_month = t->month;
        last_year = t->year;

        rtc_wait_update();
        
        t->seconds = rtc_read_reg(RTC_REG_SECONDS);
        t->minutes = rtc_read_reg(RTC_REG_MINUTES);
        t->hours = rtc_read_reg(RTC_REG_HOURS);
        t->day = rtc_read_reg(RTC_REG_DAY);
        t->month = rtc_read_reg(RTC_REG_MONTH);
        t->year = rtc_read_reg(RTC_REG_YEAR);
        
    } while (last_seconds != t->seconds || last_minutes != t->minutes ||
             last_hours != t->hours || last_day != t->day ||
             last_month != t->month || last_year != t->year);

    status_b = rtc_read_reg(RTC_REG_STATUS_B);
    
    // Fix 12-hour AM/PM flag if set — strip bit 7
    if (!(status_b & 0x04)) {
        t->seconds = bcd_to_bin(t->seconds);
        t->minutes = bcd_to_bin(t->minutes);
        t->hours   = bcd_to_bin(t->hours & 0x7F); // mask off AM/PM bit before BCD decode
        t->day     = bcd_to_bin(t->day);
        t->month   = bcd_to_bin(t->month);
        t->year    = bcd_to_bin(t->year);
    }

    t->year += 2000;
}

uint32_t rtc_get_unix_epoch(void) {
    rtc_time_t t;
    rtc_read(&t);
    
    // Simplified conversion, does not accurately handle leap years across all boundaries
    uint32_t days = (t.year - 1970) * 365 + (t.year - 1969) / 4;
    for (int i = 1; i < t.month; i++) {
        days += 30; // Approximation for simplicity
    }
    days += (t.day - 1);
    
    return ((days * 24 + t.hours) * 60 + t.minutes) * 60 + t.seconds;
}

// Tomohiko Sakamoto's algorithm — returns 0=Sun, 1=Mon, ..., 6=Sat
const char* rtc_day_of_week_str(rtc_time_t* t) {
    static const char* days[] = {
        "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
    };
    static const int t_table[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    int y = (int)t->year;
    int m = (int)t->month;
    int d = (int)t->day;
    if (m < 3) y--;
    int dow = (y + y/4 - y/100 + y/400 + t_table[m-1] + d) % 7;
    if (dow < 0) dow += 7;
    return days[dow];
}

const char* rtc_month_str(rtc_time_t* t) {
    static const char* months[] = {
        "Unknown", "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    };
    if (t->month >= 1 && t->month <= 12) {
        return months[t->month];
    }
    return months[0];
}

void rtc_init(void) {
    rtc_time_t t;
    rtc_read(&t);
    // Verify RTC is sane (year should be >= 2020)
    if (t.year < 2020 || t.year > 2100) {
        // Fallback: set a known-good time
        t.year = 2026; t.month = 1; t.day = 1;
        t.hours = 0;   t.minutes = 0; t.seconds = 0;
    }
}

// Format time as "HH:MM:SS" into buf (must be at least 9 bytes)
void rtc_format_time(rtc_time_t* t, char* buf) {
    // Manual format without sprintf to avoid dependency
    buf[0] = '0' + t->hours   / 10; buf[1] = '0' + t->hours   % 10; buf[2] = ':';
    buf[3] = '0' + t->minutes / 10; buf[4] = '0' + t->minutes % 10; buf[5] = ':';
    buf[6] = '0' + t->seconds / 10; buf[7] = '0' + t->seconds % 10; buf[8] = '\0';
}

// Format date as "DD Mon YYYY" into buf (must be at least 16 bytes)
void rtc_format_date(rtc_time_t* t, char* buf) {
    static const char* months_short[] = {
        "???", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    const char* mon = (t->month >= 1 && t->month <= 12) ? months_short[t->month] : months_short[0];
    buf[0] = '0' + t->day / 10;  buf[1] = '0' + t->day % 10; buf[2] = ' ';
    buf[3] = mon[0]; buf[4] = mon[1]; buf[5] = mon[2]; buf[6] = ' ';
    uint16_t y = t->year;
    buf[7]  = '0' + (y / 1000);     buf[8]  = '0' + (y / 100) % 10;
    buf[9]  = '0' + (y / 10)  % 10; buf[10] = '0' + y % 10; buf[11] = '\0';
}

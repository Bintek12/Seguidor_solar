// rtc.h
#ifndef RTC_H
#define RTC_H

#include <stdint.h>

struct LocalClock {
	volatile uint8_t seconds;
	volatile uint8_t minutes;
	volatile uint8_t hours;
};

extern LocalClock clock;

void initTimer1();
struct DateTime {
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
};

void initRTC();
DateTime getDateTime();
void reloj_local();

#endif

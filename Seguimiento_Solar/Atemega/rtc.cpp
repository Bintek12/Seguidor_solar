// rtc.cpp
#include "rtc.h"

LocalClock clock = {0,0,0};  
	


// Aquí iría la comunicación I2C con DS3231
void initRTC() {
	// Inicializar bus I2C y RTC
}
DateTime getDateTime() {
	DateTime dt;
	// Leer registros del DS3231
	dt.year   = 2026;
	dt.month  = 8;
	dt.day    = 5;
	dt.hour   = 16;
	dt.minute = 14;
	dt.second = 0;
	return dt;
}

void reloj_local(){
	static uint8_t ticks = 0;
	ticks++;
	if (ticks >= 2) {   // 2 × 524 ms ? 1.048 s
		ticks = 0;
		clock.seconds++;
		if (clock.seconds >= 60) {
			clock.seconds = 0;
			clock.minutes++;
			if (clock.minutes >= 60) {
				clock.minutes = 0;
				clock.hours++;
				if (clock.hours >= 24) {
					clock.hours = 0;
				}
			}
		}
}	
}
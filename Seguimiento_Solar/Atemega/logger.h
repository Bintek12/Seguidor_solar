// logger.h
#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>

struct LogEntry {  // para almecenar en data log las intensidades de os LDR
	uint16_t ldrLeft;
	uint16_t ldrRight;
};

void logData(uint16_t left, uint16_t right);

extern LogEntry logBuffer[60];
extern uint8_t logIndex;

#endif

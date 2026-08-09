#include "logger.h"

LogEntry logBuffer[60]; // 60 minutos
uint8_t logIndex = 0;

void logData(uint16_t left, uint16_t right) {
	logBuffer[logIndex].ldrLeft = left;
	logBuffer[logIndex].ldrRight = right;
	logIndex = (logIndex + 1) % 60;
}

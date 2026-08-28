#ifndef DEBUGSERIAL_H
#define DEBUGSERIAL_H

#include "config.h"
#include <stdint.h>

class DebugSerial {
	public:
	DebugSerial();
	void init(uint32_t baud);
	void print(const char* str);
	void print(int num);
	void print(unsigned int num);
	void print(float num, int digits = 2);
	void println(const char* str);
	void println(int num);
	void println();

	private:
	void enableRS485(bool enable);
	void sendByte(uint8_t data);
	void printNumber(long num);
};

#endif
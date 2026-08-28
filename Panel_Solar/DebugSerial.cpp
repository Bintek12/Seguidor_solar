#define F_CPU 8000000UL   // Ajusta a tu frecuencia real
#include "DebugSerial.h"
#include "Config.h"
#include <avr/io.h>
#include <stdlib.h>   // para ltoa, dtostrf
#include <math.h>     // para dtostrf

DebugSerial::DebugSerial() {}

void DebugSerial::enableRS485(bool enable) {
    if (enable) PORTD |= (1 << RS485_EN);
    else PORTD &= ~(1 << RS485_EN);
}

void DebugSerial::sendByte(uint8_t data) {
    while (!(UCSR0A & (1 << UDRE0))) {}
    UDR0 = data;
}

void DebugSerial::init(uint32_t baud) {
	// Configurar RS485 enable como salida
	DDRD |= (1 << RS485_EN);
	enableRS485(false);

	// Configurar UART (modo normal, divisor 16)
	uint16_t ubrr = (F_CPU / (16UL * baud)) - 1;
	UBRR0H = (uint8_t)(ubrr >> 8);
	UBRR0L = (uint8_t)ubrr;

	// Habilitar transmisor, receptor e interrupción de recepción
	UCSR0B = (1 << TXEN0) | (1 << RXEN0) | (1 << RXCIE0);

	// Configurar frame: 8 bits, 1 stop, sin paridad
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

/*
void DebugSerial::init(uint32_t baud) {
    // Configurar RS485 enable como salida
    DDRD |= (1 << RS485_EN);
    enableRS485(false);

    // Configurar UART (modo normal, divisor 16)
    uint16_t ubrr = (F_CPU / (16UL * baud)) - 1;
    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)ubrr;
    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}
*/
void DebugSerial::print(const char* str) {
    enableRS485(true);
    while (*str) sendByte(*str++);
    enableRS485(false);
}

void DebugSerial::printNumber(long num) {
    char buf[12];
    ltoa(num, buf, 10);
    print(buf);
}

void DebugSerial::print(int num) { printNumber(num); }
void DebugSerial::print(unsigned int num) { printNumber(num); }

void DebugSerial::print(float num, int digits) {
    char buf[16];
    dtostrf(num, 1, digits, buf);
    print(buf);
}

void DebugSerial::println(const char* str) {
    print(str);
    print("\r\n");
}

void DebugSerial::println(int num) {
    printNumber(num);
    print("\r\n");
}

void DebugSerial::println() {
    print("\r\n");
}
#ifndef F_CPU
#define F_CPU 8000000UL
#endif


#include "MODBUS_port.h"
#include <avr/io.h>
#include <util/delay.h>

// Pin DE del MAX485
#define DE_DDR   DDRD
#define DE_PORT  PORTD
#define DE_BIT   PD3

// getMillis() ya existe en tu main.cpp
extern uint32_t getMillis(void);

void modbus_port_init(uint32_t baudios) {
	// Configurar UART0 a 8N1, sin interrupciones (nanoMODBUS hace polling)
	uint16_t ubrr = (uint16_t)((F_CPU / (16UL * baudios)) - 1);
	UBRR0H = (uint8_t)(ubrr >> 8);
	UBRR0L = (uint8_t)(ubrr & 0xFF);

	UCSR0A = 0x00;
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);          // RX y TX activos, sin ISR
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);        // 8 bits, 1 stop, sin paridad

	// DE como salida, en reposo escuchando (recepción)
	DE_DDR  |=  (1 << DE_BIT);
	DE_PORT &= ~(1 << DE_BIT);

	(void)UCSR0A;  // lectura de cortesía para limpiar flags
}

int32_t modbus_port_read(uint8_t* buf, uint16_t count, int32_t byte_timeout_ms, void* arg) {
	(void)arg;
	uint16_t i = 0;
	while (i < count) {
		uint32_t start = getMillis();
		while (!(UCSR0A & (1 << RXC0))) {
			if ((int32_t)(getMillis() - start) > byte_timeout_ms) {
				// Devolvemos lo leído hasta ahora; si es 0, nanoMODBUS lo trata como timeout
				return (int32_t)i;
			}
		}
		buf[i++] = UDR0;
	}
	return (int32_t)i;
}

int32_t modbus_port_write(const uint8_t* buf, uint16_t count, int32_t byte_timeout_ms, void* arg) {
	(void)byte_timeout_ms;
	(void)arg;

	// Activar driver RS-485
	DE_PORT |= (1 << DE_BIT);
	_delay_us(50);   // t_setup del MAX485

	for (uint16_t i = 0; i < count; i++) {
		while (!(UCSR0A & (1 << UDRE0))) { /* esperar buffer libre */ }
		UDR0 = buf[i];
	}

	// Esperar a que salga el último byte físicamente
	while (!(UCSR0A & (1 << TXC0))) { /* esperar transmisión completa */ }
	UCSR0A |= (1 << TXC0);   // limpiar flag TXC

	_delay_us(50);           // t_hold del MAX485 antes de volver a RX
	DE_PORT &= ~(1 << DE_BIT);

	return (int32_t)count;
}
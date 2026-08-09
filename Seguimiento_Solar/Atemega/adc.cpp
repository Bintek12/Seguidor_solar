#include "adc.h"
#include <avr/io.h>

void initADC() {
	// Referencia AVcc, prescaler 128
	ADMUX = (1<<REFS0);
	ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
}

uint16_t readADC(uint8_t channel) {
	ADMUX = (ADMUX & 0xF0) | (channel & 0x0F); // seleccionar canal
	ADCSRA |= (1<<ADSC); // iniciar conversión
	while (ADCSRA & (1<<ADSC)); // esperar fin
	return ADC;
}

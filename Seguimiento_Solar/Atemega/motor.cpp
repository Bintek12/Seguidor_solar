#include "motor.h"
#include <avr/io.h>


static uint8_t motorState = 0; // 0=STOP, 1=LEFT, 2=RIGHT

void motorLeft()  { PORTB |= (1<<PB0); PORTB &= ~(1<<PB1); motorState = 1; }
void motorRight() { PORTB |= (1<<PB1); PORTB &= ~(1<<PB0); motorState = 2; }
void motorStop()  { PORTB &= ~((1<<PB0)|(1<<PB1)); motorState = 0; }



void initMotor() {
	// Configurar pines PB0 y PB1 como salida
	DDRB |= (1<<PB0) | (1<<PB1);
	motorStop();
}

uint8_t getMotorState() {
	return motorState;
}

// Finales de carrera: activos en bajo
bool limitLeftActive() {
	return (PINB & (1<<PB2)) == 0;
}

bool limitRightActive() {
	return (PINB & (1<<PB3)) == 0;
}

uint8_t getLimitStatus() {
	uint8_t status = 0;
	if (limitLeftActive())  status |= (1<<0);
	if (limitRightActive()) status |= (1<<1);
	return status;
}

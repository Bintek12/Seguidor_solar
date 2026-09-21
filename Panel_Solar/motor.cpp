#include "Motor.h"
#include <avr/io.h>

Motor::Motor() : moving(false), estado(STOP) {
	init();
}

void Motor::init() {
	MOTOR_DDR |= (1 << MOTOR_POWER) | (1 << MOTOR_DIR);
	PORTB &= ~(1 << PB7);  // Motor apagado
	PORTB &= ~(1 << PB6);  // Dirección ESTE por defecto
	stop();
}

void Motor::girarEste() {
	MOTOR_PORT |= (1 << MOTOR_POWER);
	MOTOR_PORT |= (1 << MOTOR_DIR);   // Dirección = 1 ? Este
	moving = true;
	estado = ESTE;
}

void Motor::girarOeste() {
	MOTOR_PORT |= (1 << MOTOR_POWER);
	MOTOR_PORT &= ~(1 << MOTOR_DIR);  // Dirección = 0 ? Oeste
	moving = true;
	estado = OESTE;
}

void Motor::stop() {
	MOTOR_PORT &= ~(1 << MOTOR_POWER);
	moving = false;
	estado = STOP;
}

bool Motor::isMoving() const {
	return moving;
}
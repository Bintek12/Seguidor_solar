#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>

// Inicialización del sistema de motores
void initMotor();

// Control básico
void motorLeft();   // mover hacia el Este
void motorRight();  // mover hacia el Oeste
void motorStop();   // detener motor

// Estado actual del motor
uint8_t getMotorState(); // 0=STOP, 1=LEFT, 2=RIGHT

// Finales de carrera
bool limitLeftActive();   // sensor límite Este
bool limitRightActive();  // sensor límite Oeste
uint8_t getLimitStatus(); // Bit0=Left, Bit1=Right

#endif



#ifndef CONFIG_H
#define CONFIG_H

//#define F_CPU 8000000UL   // Ajusta a tu frecuencia real

// ---------- Pines del motor (ejemplo: PC3 y PC4) ----------
#define MOTOR_DDR   DDRB
#define MOTOR_PORT  PORTB
#define MOTOR_POWER PB6   // Pin que activa el motor
#define MOTOR_DIR   PB7   // Pin de dirección (1=Este, 0=Oeste)

// ---------- Canales ADC para LDR ----------
#define LDR_ESTE   4      // ADC0
#define LDR_OESTE  5      // ADC1

// ---------- Pines de alarma y límite ----------
#define ALARMA_PIN  0     // PB0
#define LIMITE_PIN  1     // PB1

// ---------- RS485 enable ----------
#define RS485_EN   PD3  // El mismo que el led

// ---------- Constantes de funcionamiento ----------
#define DIFF_THRESHOLD  50
#define NIGHT_THRESHOLD 50





#endif
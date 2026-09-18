
/********************************************************************************************
* PROYECTO Control de seguimiento Solar par paneles
Chip type           : ATmega168A
Program type        : Application
Clock frequency     : 8,000000 MHz
Memory model        : Small
External SRAM size  : 0
Data Stack size     : 256
Compilador:AVR STUDIO 7
*****************************************************

*****************************************************/


#include <avr/io.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
//#include <avr/interrupt.h>
//#include <avr/pgmspace.h>
//#include <avr/eeprom.h>
//#include <math.h>
#define F_CPU (8000000UL)
#include <util/delay.h>

#ifndef MAIN_H
#define MAIN_H

//#define DE     PD7
//#define LED    PD6
//#define OUT0   PC3
//#define OUT1   PC4

#define RX_BUFFER_SIZE 7
#define TX_BUFFER_SIZE 16
#define ADC_BUFFER_SIZE 8
#define DIR_RS485       0x0A   //10 decimal

// Voltage Reference: AREF pin
#define ADC_VREF_5V    0x40
#define ADC_VREF_2V56  0xC0


//definicion de macros:
#define SETBIT(ADDRESS,BIT) (ADDRESS |= (1<<BIT))
#define CLEARBIT(ADDRESS,BIT) (ADDRESS &=~(1<<BIT))



// Variables Globales
struct USART {
	unsigned char rx_index;
	unsigned char tx_index;
	unsigned char rx_buffer[RX_BUFFER_SIZE];
	unsigned char tx_buffer[TX_BUFFER_SIZE];
	unsigned char dirRs485;
	unsigned int checksum;
};
extern struct USART usart;

//FLAGS de secuencias y estado del equipo
struct FLAGS{
	bool datos_listos;//comando recibido por RS 485
	bool remote_control; //control local o remoto
	bool automtico; //habilita el control de temperatura
	bool adc_sample;

};

extern struct FLAGS flags;

//Funciones externas

// Prototipos de funciones
void setup();
void initTimerMillis();
void setupWatchdog();
void Timer1_Init();

void chip_init(void);
void set_defaults(void);
extern void lcd_init(void);
unsigned int read_adc(unsigned char adc_input,uint8_t Vref);
extern void display(void);
extern void display_T(void);
void cmd_decode();
void leer_sensor(void);
void control_temperatura(void);

#endif

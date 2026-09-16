
/********************************************************************************************
* PROYECTO Control de temperatura para Molde
Chip type           : ATmega168A
Program type        : Application
Clock frequency     : 8,000000 MHz
Memory model        : Small
External SRAM size  : 0
Data Stack size     : 256
Compilador:AVR STUDIO 6.1
*****************************************************

*****************************************************/

#include <avr/io.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <math.h>
#define F_CPU (8000000UL)
#include <util/delay.h>
#include "chip_init.c"
#include "rtc.h"

#define DE     PD7
#define LED    PD6
#define OUT0   PC3
#define OUT1   PC4

#define RX_BUFFER_SIZE 36
#define TX_BUFFER_SIZE 16
#define ADC_BUFFER_SIZE 8
#define DIR_RS485       2

//definicion de macros:
#define SETBIT(ADDRESS,BIT) (ADDRESS |= (1<<BIT))
#define CLEARBIT(ADDRESS,BIT) (ADDRESS &=~(1<<BIT))

// Definiciones de canales ADC para los LDR
#define LDR_LEFT   0   // ADC0 (PC0)
#define LDR_RIGHT  1   // ADC1 (PC1)

// Voltage Reference: AREF pin
#define ADC_VREF_5V    0x40
#define ADC_VREF_2V56  0xC0

// Variables Globales

struct USART
{
	unsigned char rx_index;
	unsigned char tx_index;
	unsigned char rx_buffer[RX_BUFFER_SIZE];
	unsigned char tx_buffer[TX_BUFFER_SIZE];
	unsigned char dirRs485;
	unsigned int checksum;
}usart;

//FLAGS de secuencias y estado del equipo
struct FLAGS{
	bool datos_listos;//comando recibido por RS 485
	bool remote_control; //control local o remoto
	bool automtico; //habilita el control de temperatura
	bool adc_sample;

}flags;

struct TCONTROL{
	int16_t consigna;
	int16_t sensor0;
	int16_t sensor1;
	int16_t Kp;
	int16_t Ki;
	int16_t Kd;
	}tcntrl;

volatile unsigned char led_blinking;
volatile unsigned char sample_counter;
int16_t T0_buffer[ADC_BUFFER_SIZE];
int16_t T1_buffer[ADC_BUFFER_SIZE];
int8_t adc_buffer_index;
EEMEM unsigned char active_program_eep;
unsigned char active_program_ram;


DateTime upDateTime();

//Funciones externas
extern  char lcd_buffer_H[16]; // Linea superior del LCD
extern  char lcd_buffer_L[16]; // Linea inferior del LCD
//extern void lcd_goto(); // 1a Linea=0x00, 2a Linea=0x40 
void lcd_putch();
extern void lcd_busy(void);
extern void lcd_init(void);

EEMEM char eeprom_writed;

// Prototipos de funciones
void chip_init(void);
void set_defaults(void);
extern void lcd_init(void);
unsigned int read_adc(unsigned char adc_input,uint8_t Vref);
extern void display(void);
extern void display_T(void);
void display_Hora(void);
void cmd_decode();
void leer_sensor(void);
void control_temperatura(void);

void logData(uint16_t left, uint16_t right);

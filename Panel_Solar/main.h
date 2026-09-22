
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
//#include <math.h>
#include <avr/eeprom.h>
#include <util/crc16.h>

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

// --- Configuración del búfer circular ---
#define NUM_SLOTS        120
#define RECORD_SIZE      3          // 2 bytes valor + 1 byte CRC
#define DATA_BUFFER_START 4
#define STATUS_BUFFER_START (DATA_BUFFER_START + NUM_SLOTS * RECORD_SIZE)
#define EEPROM_MAGIC     0xB8
/*
// Direcciones Modbus para el búfer de datos
#define MODBUS_BUFFER_START_ADDR  100   // Registros holding 100..219
#define MODBUS_CMD_REGISTER       300   // Escribir 1 aquí ? tomar muestra

// Registros Modbus personalizados
#define MODBUS_LOG_BASE_ADDR   100     // 100..219  ? búfer completo
#define MODBUS_INTERVAL_REG    300     // intervalo en segundos (lo escribe el master)
#define MODBUS_COUNT_REG       301     // nº de muestras válidas (informativo)
#define MODBUS_TRIGGER_REG     302     // escribir 1 ? forzar muestra manual
*/
static uint32_t last_sample_ms   = 0;
extern volatile uint32_t system_ms; 

/*
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
*/
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
void init_eeprom_buffer();
uint8_t find_last_slot();
void write_ldr_sample(uint16_t ldr_value);
uint32_t getMillis();

#endif

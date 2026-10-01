
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
#include <avr/eeprom.h>
#include <util/crc16.h>

#define F_CPU (8000000UL)
#include <util/delay.h>

#ifndef MAIN_H
#define MAIN_H



//#define RX_BUFFER_SIZE 7
//#define TX_BUFFER_SIZE 16
#define ADC_BUFFER_SIZE 8
//#define DIR_RS485       0x0A   //10 decimal

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

static uint32_t last_sample_ms   = 0;
extern volatile uint32_t system_ms; 


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
//void control_temperatura(void);
void init_eeprom_buffer();
uint8_t find_last_slot();
void write_ldr_sample(uint16_t ldr_value);
uint32_t getMillis();

#endif

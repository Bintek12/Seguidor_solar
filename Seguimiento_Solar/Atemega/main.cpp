/*****************************************************

Project :  Atmega168A Incubadora
Version :  19.0
Date    : 18/12/2022
Compilador: Atmel Studio 7.0
Author  : Bruno Gonzalez Rojas
Company : Control Automatico




Chip type           : ATmega168A DIL
Program type        : Application
Clock frequency     : 8,000000 MHz (internal RC Oscilator)
Compilador          : AVR GCC
Memory model        : Small
External SRAM size  : 0
Data Stack size     : 256
*****************************************************



*****************************************************/

#include "main.h"
#include "adc.h"
#include "m8_lcd.c"
#include "rtc.h"
#include "logger.h"
#include "tracker.h"
#include "modbus_local.h"
#include <avr/io.h>


//LogEntry logBuffer[60]; // 60 minutos
//uint8_t logIndex = 0;

TrackerMode mode = MODE_HYBRID; // Selección del modo

/*USART Receive interrupt routine
recibe el comando ( 36 bytes) desde la red rs485 */
ISR(USART_RXC_vect){
	unsigned char status;
	status = UCSR0A; //requerido por el hardware (see Atmel datasheet)
	usart.rx_buffer[usart.rx_index] = UDR0;
	usart.rx_index++;
	if(usart.rx_index >= RX_BUFFER_SIZE ){
		usart.rx_index = 0;
		if(usart.rx_buffer[0] == DIR_RS485)//dirRS485
		flags.datos_listos = true;
	}
	TCNT2 = 0;    // Reinicializa Timer de recepción
	TCCR2B = 0x07;  // Arranca temporizador f = Ck/1024  = 15625 Hz
	TIMSK2 |=  (1<<TOIE2);  // habilita interrupción counter2 ovf
	PORTD ^=(1<<PD5);//solo para probar
}

/* Timer 2 overflow interrupt service routine
Perro guardián de la comunicación serie */
ISR(TIMER2_OVF_vect){
	TCNT2 = 0;    // Reinicializa Timer de recepción
	TCCR2B = 0x00;  // detiene temporizador
	TIMSK2 &= ~ (1<<TOIE2);  // desabilita interrupción counter ovf
	usart.rx_index = 0; // Inicializa puntero de recepción
}

/* Timer 0 overflow interrupt service routine
Ocurre con una frecuencia de 7,813 kHz
*/
ISR(TIMER0_OVF_vect){
	//led_blinking++;
	//flags.adc_sample = true;
}

// ISR de overflow cada ~524 ms
ISR(TIMER1_OVF_vect) {
	reloj_local();
	
    led_blinking++;
}
// Timer1 output compare A interrupt service routine
ISR(TIMER1_COMPA_vect){
	CLEARBIT(PORTC,OUT0);
	CLEARBIT(PORTC,OUT1);
}


int main(void){
    chip_init();
	initADC();
     _delay_ms(10);
    lcd_init();
	strcpy(lcd_buffer_H," SEGUIDOR SOLAR ");
    strcpy(lcd_buffer_L,"- BinteK(c) 2026");
	display();
    _delay_ms(2000);
	lcd_clear();
	lcd_init();
	display();
	/*
	// inicialización de Timer1
	TCCR1A = 0;
	TCCR1B = (1<<CS11) | (1<<CS10); // prescaler 64
	TCNT1 = 0;
	TIMSK1 = (1<<TOIE1);
	sei();
	*/
// Variables de control
    flags.datos_listos = false;
	tcntrl.Kp =600;
	tcntrl.consigna= 40;
    
	while (1){
		void leer_sensor();
		int ldrLeft  = tcntrl.sensor0;
		int ldrRight = tcntrl.sensor1;
		//int ldrLeft  = readADC(LDR_LEFT);
		//int ldrRight = readADC(LDR_RIGHT);
		
		
		//DateTime dt  = getDateTime(); // para actualizar d Internet
		display_Hora();
		
		// Cada minuto guardar datos en logger
		if (clock.seconds == 0) {
			logData(ldrLeft, ldrRight);
		}

        // Atender solicitudes Modbus
        
        modbusUpdate(ldrLeft, ldrRight, mode);
		
		 if(flags.datos_listos){
			 flags.datos_listos = false;
			 cmd_decode();
		 }
		 if(flags.adc_sample){
			 flags.adc_sample = false;
			 leer_sensor();
		 }
		 if(led_blinking >= 2){
			 led_blinking = 0;
			 PORTD ^=(1<<LED);
		 }
    }
}

// Read the AD conversion result
unsigned int read_adc(unsigned char adc_input,uint8_t Vref){
	ADMUX=adc_input|Vref;
	_delay_ms(10);
	// Start the AD conversion
	ADCSRA|=0x40;
	// Wait for the AD conversion to complete
	while ((ADCSRA & 0x10)==0);
	ADCSRA|=0x10;
	return ADC;
}

void leer_sensor(void){
	uint16_t utemp;
	float ftemp;
	T0_buffer[adc_buffer_index] = read_adc(0x01,ADC_VREF_5V);
	T1_buffer[adc_buffer_index] = read_adc(0x00,ADC_VREF_5V);
	adc_buffer_index++;
	if(adc_buffer_index>=ADC_BUFFER_SIZE)	{
		utemp = 0;
		for(int i=0;i<ADC_BUFFER_SIZE;i++)
		utemp += T0_buffer[i];
		utemp = utemp /(uint16_t)ADC_BUFFER_SIZE;
		ftemp = ((float)utemp * 4980  / 1023.0);
		tcntrl.sensor0 = (int16_t)ftemp; 
		utemp = 0;
		for(int i=0;i<ADC_BUFFER_SIZE;i++)
		utemp += T1_buffer[i];
		utemp = utemp /(uint16_t)ADC_BUFFER_SIZE;
		ftemp = ((float) utemp * 4980  / 1023.0);
		tcntrl.sensor1 = (int16_t)ftemp;   
		adc_buffer_index = 0;
	}
	flags.adc_sample = true;
}

void display_Hora(void){
	DateTime dt  = upDateTime();
	lcd_goto(0x0A);
	sprintf(lcd_buffer_H, "%02i:%02i", dt.minute, dt.second);
	display();
}

	
//Envia el contenido del buffer de 32 bits al DISPLAY LCD
void display_T(void){
	char *pH;
	char *pL;
	unsigned char k;
	uint8_t entero;
	uint8_t decimal;
	//entero = tcntrl.sensor0 / 10;
	//decimal = tcntrl.sensor0 %10;
	entero = 12 / 10;
	decimal = 12 %10;
	//sprintf(lcd_buffer_H,"azimut %0i%0i: ",entero,decimal);
	//sprintf(lcd_buffer_H, "%02i:%02i", 	dt.minute, dt.second);
	//sprintf(lcd_buffer_H,"%05i oC ",tcntrl.temperatura0);
	//entero = tcntrl.sensor1 / 10;
	//decimal = tcntrl.sensor1 %10;
	entero = 21 / 10;
	decimal = 21 %10;
	sprintf(lcd_buffer_L,"intensidad %0i%0i ",entero,decimal);
	//sprintf(lcd_buffer_L,"%05i oC ",tcntrl.temperatura1);
	pH=&lcd_buffer_H[0];
	//lcd_clear();
	lcd_goto(0x00);
	pH=&lcd_buffer_H[0];
	for(k=0;k<16;k++){
		lcd_busy();
		lcd_putch(*(pH+k));
	}
	pL=&lcd_buffer_L[0];
	lcd_goto(0x40);
	pL=&lcd_buffer_L[0];
	for(k=0;k<16;k++){
		lcd_busy() ;
		lcd_putch(*(pL+k));
	}
}

void cmd_decode(){
	switch(usart.rx_buffer[1]){
		case 1:
		lcd_init();
		break;
		case 2: {
		    for(int i=0;i<16;i++)	{
				lcd_buffer_H[i] = usart.rx_buffer[i+2];
				lcd_buffer_L[i] = usart.rx_buffer[i+18];
		   }
		}   
		break;
		default:
		break;
	}
	
	display();
	usart.rx_buffer[0] = 0;
	usart.rx_buffer[1] = 0;
}

DateTime upDateTime() {
	DateTime dt;
	// Leer registros del DS3231
	dt.year   = 2026;
	dt.month  = 8;
	dt.day    = 5;
	dt.hour   = clock.hours;
	dt.minute = clock.minutes;
	dt.second = clock.seconds;
	return dt;
}
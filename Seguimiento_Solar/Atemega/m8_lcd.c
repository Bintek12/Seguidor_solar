/*  jLCD.C
 *	Implementación para la interfaz de un LCD estándar.
 *	Jose Emilio Ledo Galano
 *
 *	Ver lcd.h para mas info.
 */

#include	"m8_lcd.h"
//#include "declarations.h"


void lcd_init(void)
// Secuencia de inicialización LCD. Es válida para TODOS
// los LCDs (Se han puesto tiempo muy altos).
// SEGUIR SIEMPRE ESTA INICIALIZACIÓN, 
{
	DDRB = 0xFF;
	CLEARBIT(LCD_Port,LCD_RS);		// Modo Comando
	CLEARBIT(LCD_Port,LCD_RW);		// Modo Escritura

	_delay_ms(40);	// Delay inicial poweron (min 15-20ms)
	
	// En estas instrucciones no se puede comprobar BUSY (bloqueo)

	// 0011: Funcion set interface 8bits (en 4bits físicos)
	CLEARBIT(LCD_Port,LCD_D7);
    CLEARBIT(LCD_Port,LCD_D6);
	SETBIT(LCD_Port,LCD_D5);
	SETBIT(LCD_Port,LCD_D4);
	lcd_pulse();	// Escritura 0011
	_delay_ms(5);	// Esperar mas de 4.1ms
	lcd_pulse();	// Escritura 0011
	_delay_ms(1);	// Esperar mas de 100us
	lcd_pulse();	// Escritura 0011
	_delay_ms(5);	// Esperar mas de 4.1ms
	
	// 0010: Funcion set interface 4bits (en 4bits físicos)
    CLEARBIT(LCD_Port,LCD_D7);
    CLEARBIT(LCD_Port,LCD_D6);
	SETBIT(LCD_Port,LCD_D5);
	CLEARBIT(LCD_Port,LCD_D4);
	lcd_pulse();	// Escritura 0010. 4bits
	_delay_us(120);	// Esperar mas de 100us	

	// Ahora se puede comprobar BUSY
	
	lcd_write(0x2C);	// 4 bit mode, 2/16 duty, 5x8 font
	_delay_us(40);
	lcd_write(0x08);	// display off
	_delay_us(40);
        lcd_write(0x0C);	// display on, blink off, curson off
	_delay_us(40);
        lcd_write(0x06);	// entry mode normal
        //_delay_us(40);
	// FIN INICIALIZACIÓN. Ajustar ahora como nos interese.
}

void lcd_putch(U8 c)
// Escribe un carácter en LCD
{
	lcd_busy();		// Espera hasta que LCD no busy

	SETBIT(LCD_Port,LCD_RS);		// Modo Dato
	lcd_write(c);	// Escribe carácter como dato
}

/*
void lcd_puts(const char * s)
// Escribe una cadena (terminada en nulo \0) en el LCD
{
	while(*s)				// Bucle imprimiendo caracteres
		lcd_putch(*s++);	// hasta el nulo.
}
*/

void lcd_command(U8 c)
// Escribe un comando 8 bits al LCD en 2 de 4bits
{
	lcd_busy();		// Espera hasta que LCD no busy

	CLEARBIT(LCD_Port,LCD_RS);			// Modo Comando
	lcd_write(c);	// Escribe carácter como dato
}

void lcd_clear(void)
// Limpia el LCD y va a home
{
	lcd_command(0x01);	// Comando 0x01 CLS y HOME
}

void lcd_goto(U8 pos)
// Ir a una posición del LCD. 1a Linea=0x00, 2a Linea=0x40
{
	lcd_command(0x80|pos);
}

void lcd_busy(void)
// Espera a que el LCD esté libre
{
	static char busyflag;
	
	CLEARBIT(LCD_Port,LCD_RS);			// Modo Comando
	SETBIT(LCD_Port,LCD_RW);			// Modo Lectura

	CLEARBIT(LCD_Port,LCD_T4 );	// Puerto datos como entrada, lectura
	CLEARBIT(LCD_Port,LCD_T5 );
	CLEARBIT(LCD_Port,LCD_T6 );
	CLEARBIT(LCD_Port,LCD_T7 );

	do
	{
		SETBIT(LCD_Port,LCD_EN);				// 1er pulso enable
		_delay_us(LCDPULSEON);	// ON
		busyflag =PIND & (1<<PIND7);	// Leer Busy
		CLEARBIT(LCD_Port,LCD_EN);				// OFF
		_delay_us(LCDPULSEOFF);

		lcd_pulse();			// 2do pulso enable(no leer)
	
	} while (busyflag);		// Hasta que deje de estar Busy
	
	SETBIT(LCD_Port,LCD_T4 );	// Puerto datos como SALIDA
	SETBIT(LCD_Port,LCD_T5 );
	SETBIT(LCD_Port,LCD_T6 );
	SETBIT(LCD_Port,LCD_T7 );
}

void lcd_write(U8 c)
// Escritura de un byte al LCD (no se especifica comando o dato)
{
	// Antes de entrar a esta rutina RS tiene que estar en el modo
	//   adecuado (Comando RS=0 o Dato RS=1)
	CLEARBIT(LCD_Port,LCD_RW);		// Modo Escritura
	LCD_Port =(LCD_Port & 0x0F)| (0xF0 & c) ;	// Escribe nibble alto
	lcd_pulse();		// Enable
	LCD_Port =(LCD_Port & 0x0F)| (c << 4) ;	// Escribe nibble bajo
	lcd_pulse();
}


void lcd_pulse(void)
// Secuencia pulso de Enable
{
	SETBIT(LCD_Port,LCD_EN);			
	_delay_us(LCDPULSEON);
	CLEARBIT(LCD_Port,LCD_EN);		
	_delay_us(LCDPULSEOFF);
}


	
  //Conversión a bcd para visualización

void int_to_bcd(U16 value)
{
     U16 resto;
     //U8[5] digitos;
     digitos.dec_miles = 48 +  value / 10000;
     resto = value % 10000;
     digitos.miles = 48 + resto / 1000;
     resto=resto % 1000;
     digitos.centenas = 48 + resto / 100;
     resto=resto % 100;
     digitos.decenas = 48 + resto / 10;
     resto=resto % 10;
     digitos.unidades = 48 + resto;

 }

void char_to_bcd(U8 c)
{

     U8 resto;
     digitos.centenas = 48 +  c / 100;    //centenas
     resto = c % 100;
     digitos.decenas = 48 +  resto / 10; //decenas
     resto = resto % 10;
     digitos.unidades = 48 +  resto;    //unidades

}

//Envia el contenido del buffer de 32 bits al DISPLAY LCD
void display(void)
{
	char *pH;
	char *pL;
	unsigned char k;
	pH=&lcd_buffer_H[0];
	lcd_goto(0x00);
	pH=&lcd_buffer_H[0];
	for(k=0;k<16;k++)
	{
		lcd_busy();
		lcd_putch(*(pH+k));
	}

	pL=&lcd_buffer_L[0];
	lcd_goto(0x40);
	pL=&lcd_buffer_L[0];
	for(k=0;k<16;k++)
	{
		lcd_busy() ;
		lcd_putch(*(pL+k));
	}

}

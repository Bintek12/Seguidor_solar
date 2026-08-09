/*  m8_lcd.h
 *	Fichero de cabecera para la interfaz de un LCD estándar.
 *	Jose Emilio Ledo  Equilab, S.A.
 *
 * 1. Delays fijos para una frecuencia determinada.
 *	2. El usuario debe establecer la dirección de los puertos y
 *		la inicialización necesaria (puertos digitales vs
 *		analógicos, etc.).
 *	3. Inicialización del LCD con lcd_init(). Esta función configura
 *		el LCD para 4 bits.
 *	4. El usuario debe configurar a su gusto el LCD (Display ON, cursor,
 *		etc.). Para ello dispone de lcd_command(comando), vea el datasheet
 *		del	LCD para ver los comandos disponibles.
 */
/**** NOTA: SE ELIMINAN LAS FUNCIONES NO USADAS  *****/


#include <avr/io.h>
#define F_CPU (8000000UL)
#include <util/delay.h>


// Tiempos (us) del pulso enable (cuanto más altos mas valen para todos
// los LCDs del mercado.
#define LCDPULSEON	50
#define LCDPULSEOFF	50

// Líneas físicas de datos (4bits) y control (3bits)
#define LCD_Port        PORTB
#define LCD_RS		PB0       // Register select
#define LCD_RW		PB1	     // Read/write
#define LCD_EN		PB2	     // Enable
#define LCD_D7		PB7	     // Linea 7 de bus datos 4 bits
#define LCD_D6		PB6	     // Linea 6 de bus datos 4 bits
#define LCD_D5		PB5	     // Linea 5 de bus datos 4 bits
#define LCD_D4		PB4	     // Linea 4 de bus datos 4 bits
#define LCD_T7		DDB7	     // Linea conf 7 de bus datos 4 bits
#define LCD_T6		DDB6	     // Linea conf 6 de bus datos 4 bits
#define LCD_T5		DDB5	     // Linea conf 5 de bus datos 4 bits
#define LCD_T4		DDB4	     // Linea conf 4 de bus datos 4 bits
#define LCD_Busy    PINB7        // Busy Flag del lcd
typedef unsigned char U8;
typedef unsigned int  U16;
char lcd_buffer_H[16]; // Linea superior del LCD
char lcd_buffer_L[16]; // Linea inferior del LCD

//definicion de macros:
#define SETBIT(ADDRESS,BIT) (ADDRESS |= (1<<BIT))
#define CLEARBIT(ADDRESS,BIT) (ADDRESS &=~(1<<BIT))

struct bcd
{
  U8 dec_miles;
  U8 miles;
  U8 centenas;
  U8 decenas;
  U8 unidades;
}digitos;
/* Prototipos */
/* Inicialización del LCD */

extern void lcd_init(void);

extern void display(void);
extern void lcd_init(void);
extern void lcd_goto(U8 pos);

/* Escribe un carácter en LCD */
extern void lcd_putch(U8);

/* Escribe una cadena (terminada en nulo \0) en el LCD */
//extern void lcd_puts(const U8 * s);

/* Escribe un comando al LCD */
void lcd_command(U8);

/* Limpia el LCD y va a home */
void lcd_clear(void);



/* Funciones que normalmente son de uso interno */

/* Espera a que el LCD esté libre */
void lcd_busy(void);

/* Escritura de un Byte al LCD (no se especifica comando o dato) */
void lcd_write(U8);

/* Pulso de enable para W/R */
void lcd_pulse(void);

void int_to_bcd(U16);
void char_to_bcd(U8 );


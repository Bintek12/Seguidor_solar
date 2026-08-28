

#include "Config.h"
#include <avr/io.h>
#include <avr/wdt.h>
#include "Modbus_local.h"
#define F_CPU (8000000UL)
#include <util/delay.h>
#include "Motor.h"
#include "Panel.h"
#include "DebugSerial.h"

#include <avr/interrupt.h>
#include "main.h"

struct USART usart;
struct FLAGS flags; 

// Variables globales de tiempo
extern volatile uint32_t system_ms;

//volatile uint8_t rx_buffer[MODBUS_BUFFER_SIZE];
volatile uint8_t rx_index;

ISR(USART_RX_vect){
	unsigned char status;

	status = UCSR0A; //requerido por el hardware (see Atmel datasheet)
	usart.rx_buffer[usart.rx_index] = UDR0;
	usart.rx_index++;

	if(usart.rx_index >= RX_BUFFER_SIZE )	{
		usart.rx_index = 0;
		if(usart.rx_buffer[0] == usart.dirRs485)//dirRS485
		flags.datos_listos = true;		
	}
	TCNT2 = 0;    // Reinicializa Timer de recepción
	TCCR2B = 0x07;  // Arranca temporizador f = Ck/1024  = 15625 Hz
	TIMSK2 |=  (1<<TOIE2);  // habilita interrupción counter2 ovf
	//PORTD ^=(1<<PD3);//solo para probar
	//PORTD =(1<<PD3);//solo para probar
}

/* Timer 2 overflow interrupt service routine
Perro guardián de la comunicación serie */
ISR(TIMER2_OVF_vect)
{
	TCNT2 = 0;    // Reinicializa Timer de recepción
	TCCR2B = 0x00;  // detiene temporizador
	TIMSK2 &= ~ (1<<TOIE2);  // desabilita interrupción counter ovf
	usart.rx_index = 0; // Inicializa puntero de recepción
}

/*
ISR(USART_RX_vect) {
	uint8_t data = UDR0;                 // Leer byte recibido
	if (rx_index < MODBUS_BUFFER_SIZE) {
		rx_buffer[rx_index++] = data;    // Guardar en buffer
		} else {
		rx_index = 0;                    // Reinicio si desborda
	}
}

*/

uint32_t getMillis();
void initTimerMillis();
void setupWatchdog();
void Timer1_Init();


int main() {
	Motor motor;
	Panel panel;
	DebugSerial debug;
    initTimerMillis();
	setupWatchdog();
	//chip_init();
	ASSR=0x00;
	TCCR2A=0x00;
	TCNT2=0x00;
	OCR2A=0x00;
	TIMSK2= (0<<OCIE2A) | (1<<TOIE2) ;
	Timer1_Init();
	debug.init(9600);
	panel.initPID(1.5, 0.3, 0.05, 100.0, 2.0);
	debug.println("*** SEGUIDOR SOLAR INICIADO ***");
    //uint32_t lastPID = 0;
    uint32_t lastMotor = 0;
	//Modbus_Init();
	usart.dirRs485 = DIR_RS485;     
	sei(); // habilita interrupciones globales
	
	while (1) {
		
		// 1. Lee los LDRs y actualiza eastFiltered / westFiltered (tu código existente)
		panel.leerSensores();                             
		
		// Verificar alarma (prioridad máxima)
		if (panel.isAlarm()) {
			motor.stop();
			debug.println("ALARMA ACTIVA - Motor detenido");
			_delay_ms(500);
			continue;
		}

		// Verificar límite de movimiento
		if (panel.isLimit()) {
			motor.stop();
			debug.println("LÍMITE ALCANZADO - Motor detenido");
			_delay_ms(100);
			continue;
		}
		// 2. Cada 100ms (por ejemplo), llama a la decisión del PID
		static unsigned long lastPID = 0;
		if (getMillis() - lastPID >= 100) { // 100ms = dt=0.1
			lastPID = getMillis();
			// Esto actualiza la variable global 'pidOutput'
			//Direccion dir = panel.decidirDireccion();
			panel.decidirDireccion();
			// Nota: 'dir' solo lo usas para mostrarlo o lógica extra,
			// la acción real la hace 'aplicarControlMotor'.
		}

		// 3. Actualizar motor cada 1 ms (PWM) suave
		if (getMillis() - lastMotor >= 1) { // Actualiza PWM cada 10ms
			lastMotor = getMillis();
			panel.aplicarControlMotor();
		}
		// --- Debug cada 500 ms  ---
		static uint32_t lastDebug = 0;
		if (getMillis() - lastDebug >= 5000) {
			lastDebug = getMillis();
			// Mostrar valores por debug
			/*
			debug.print("Sensor E:");
			debug.print(panel.getEastFiltered());
			debug.print("Sensor W:");
			debug.print(panel.getWestFiltered());
			debug.print(" Err:");
			debug.println(panel.getError());
			debug.print(" Estado: ");
			debug.println(panel.getStatusMessage());
			debug.print(" Temperatura del Panel: ");
			debug.println(panel.readTemperature());
			
			debug.print("ERR:");
			debug.print((int)(panel.getCurrentError() * 100));
			debug.print(" PID:");
			debug.print((int)(panel.getPIDOutput() * 100));
			debug.print(" DUTY:");
			debug.print((int)(fabsf(panel.getPIDOutput()) / 100.0 * 100));
			debug.print("% PB6:");
			debug.print((PORTB & (1 << PB6)) ? "OESTE" : "ESTE");
			debug.print(" PB7:");
			debug.println((PORTB & (1 << PB7)) ? "ON" : "OFF");
			*/
		}   // end if (getMillis() 
		Modbus_Update_Registers(); // refresca datos de sensores
		
		if (flags.datos_listos){ // atiende peticiones Modbus
		   Modbus_Service();  
		   PORTD ^=(1<<PD3);//solo para probar
		   flags.datos_listos = false;
		   usart.rx_index = 0;  
		}      
		// --- ALIMENTAR AL WATCHDOG ---
		// Esta llamada reinicia el contador del WDT.
		// Debe ejecutarse regularmente, al menos una vez cada 2 segundos (en este ejemplo).
		wdt_reset(); // <--- Punto clave para evitar el reinicio[reference:7]
	}	    // end While
}           //  End main

// Función para obtener milisegundos (accesible desde panel.cpp)
uint32_t getMillis() {
	uint32_t val;
	cli();
	val = system_ms;
	sei();
	return val;
}

// Configurar Timer1 para interrupción cada 1 ms con prescaler 64
void initTimerMillis() {
	TCCR1B = 0;
	TCNT1 = 0;
	OCR1A = 124;   // (8e6/64/1000) - 1 = 124
	TCCR1A = 0;
	TCCR1B = (1 << WGM12) | (1 << CS10) | (1 << CS11); // CTC, prescaler 64
	TIMSK1 = (1 << OCIE1A);
	sei();
}

void setupWatchdog() {
	// 1. Limpiar el flag de reinicio por Watchdog (WDRF) en MCUSR
	// Esto es OBLIGATORIO para evitar reinicios inesperados.
	MCUSR &= ~(1 << WDRF);

	// 2. Deshabilitar el WDT durante la configuración inicial.
	// Es una buena práctica para asegurar un inicio limpio.
	wdt_disable();

	// 3. Habilitar el WDT con el tiempo de espera deseado.
	// Ejemplo: 2 segundos (WDTO_2S).
	// Otros valores comunes: WDTO_15MS, WDTO_30MS, WDTO_60MS, WDTO_120MS,
	// WDTO_250MS, WDTO_500MS, WDTO_1S, WDTO_2S, WDTO_4S, WDTO_8S[reference:4].
	wdt_enable(WDTO_2S);
}

void Timer1_Init(void) {
	TCCR1B = (1<<WGM12)|(1<<CS11); // CTC, prescaler 8
	OCR1A = 2000;                  // ~2 ms a 16 MHz
	TIMSK1 = (1<<OCIE1A);          // habilita interrupción
}

void chip_init(void)
{
	// Input/Output Ports initialization
	// Port B initialization
	// Func7=In Func6=In Func5=Out Func4=In Func3=Out Func2=Out Func1=Out Func0=In
	// State7=T State6=T State5=0 State4=T State3=0 State2=0 State1=0 State0=T
	PORTB=0x00;
	DDRB=0x0F;

	// Port C initialization
	// Function: Bit6=In Bit5=In Bit4=Out Bit3=Out Bit2=In Bit1=In Bit0=In
	DDRC=(0<<DDC6) | (0<<DDC5) | (1<<DDC4) | (1<<DDC3) | (0<<DDC2) | (0<<DDC1) | (0<<DDC0);
	// State: Bit6=T Bit5=T Bit4=0 Bit3=0 Bit2=T Bit1=T Bit0=T
	PORTC=0x00;

	// Port D initialization
	// Func7=Out Func6=Out Func5=Out Func4=In Func3=Out Func2=In Func1=Out Func0=In
	// State7=0 State6=0 State5=0 State4=T State3=0 State2=T State1=0 State0=T
	PORTD=0x00;
	DDRD=0xF2;

	// Timer/Counter 0 initialization
	// Clock source: System Clock
	// Clock value: 7,813 kHz
	TCCR0B=(0<<CS02) | (1<<CS01) | (1<<CS00);   //reloj/ 1024
	TCNT0=0x00;
	TIMSK0=(0<<OCIE0B) | (0<<OCIE0A) | (1<<TOIE0);

	// Timer/Counter 1 initialization
	// Clock source: System Clock
	// Clock value: 125 000 kHz
	// Mode: Normal top=0xFFFF
	// OC1A output: Disconnected
	// OC1B output: Disconnected
	// Noise Canceler: Off
	// Input Capture on Falling Edge
	// Timer Period: 2,0972 s
	// Timer1 Overflow Interrupt: On
	// Input Capture Interrupt: Off
	// Compare A Match Interrupt: On
	// Compare B Match Interrupt: Off
	TCCR1A=(0<<COM1A1) | (0<<COM1A0) | (0<<COM1B1) | (0<<COM1B0) | (0<<WGM11) | (0<<WGM10);
	TCCR1B=(0<<ICNC1) | (0<<ICES1) | (0<<WGM13) | (0<<WGM12) | (0<<CS12) | (1<<CS11) | (1<<CS10);
	TCNT1H=0x00;
	TCNT1L=0x00;
	ICR1H=0x00;
	ICR1L=0x00;
	OCR1AH=0x00;
	OCR1AL=0x00;
	OCR1BH=0x00;
	OCR1BL=0x00;
	// Timer(s)/Counter(s) Interrupt(s) initialization
	TIMSK1= (0<<ICIE1) | (1<<OCIE1A) | (0<<OCIE1B) | (1<<TOIE1);
	
	
	// Timer/Counter 2 initialization
	// Clock source: System Clock
	// Clock value: Timer 2 Stopped
	// Mode: Normal top=FFh
	// OC2 output: Disconnected
	ASSR=0x00;
	TCCR2A=0x00;
	TCNT2=0x00;
	OCR2A=0x00;
	TIMSK2= (0<<OCIE2A) | (1<<TOIE2) ;
	
	// USART initialization
	// Communication Parameters: 8 Data, 1 Stop, No Parity
	// USART Receiver: On
	// USART Transmitter: On
	// USART Mode: Asynchronous
	// USART Baud rate: 38400 a 8 MHz
	UCSR0A=0x00;
	UCSR0B=0x98;
	UCSR0C=0x86;
	UBRR0H=0x00;
	UBRR0L=0x0C;
	
	// ADC initialization
	// ADC Clock frequency: 125,000 kHz
	// ADC Voltage Reference: Int.,
	
	ADCSRA |= ((1<<ADPS2)|(1<<ADPS1)|(0<<ADPS0));   //Prescaler 64 para un reloj de conversion 125Khz
	ADMUX=ADC_VREF_5V;              //Avcc(+5v) as voltage reference
	ADCSRB &= ~((1<<ADTS2)|(1<<ADTS1)|(1<<ADTS0));  //ADC in free-running mode
	ADCSRA |= (0<<ADATE);               //Signal source, in this case is not  free-running
	ADCSRA |= (1<<ADEN);                //Power up the ADC
	//ADCSRA |= (0<<ADSC); //Start converting
	
	//ADCSRA= (1<<ADEN) | (0<<ADSC) | (0<<ADATE) | (1<<ADPS2) | (1<<ADPS1) | (0<<ADPS0);
	//ADCSRA=0x87;

	// Watchdog Timer initialization
	// Watchdog Timer Prescaler: OSC/256k (2 seg)
	// Watchdog timeout action: Reset
	//	WDTCSR=(0<<WDIF) | (0<<WDIE) | (0<<WDP3) | (1<<WDCE) | (1<<WDE) | (1<<WDP2) | (1<<WDP1) | (1<<WDP0);
	//	WDTCSR=(0<<WDIF) | (0<<WDIE) | (0<<WDP3) | (0<<WDCE) | (1<<WDE) | (1<<WDP2) | (1<<WDP1) | (1<<WDP0);
	//	asm("WDR");

	// Global enable interrupts
	asm("sei");

}
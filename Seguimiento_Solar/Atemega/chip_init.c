#include <avr/io.h>

// Voltage Reference: AREF pin
#define ADC_VREF_5V    0x40
#define ADC_VREF_2V56  0xC0


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
	TCCR0B=(1<<CS02) | (0<<CS01) | (1<<CS00);   //reloj/ 1024
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

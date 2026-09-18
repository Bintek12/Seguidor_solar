#ifndef F_CPU
#define F_CPU 8000000UL
#endif


#include "MODBUS_port.h"
#include "nanomodbus.h"
#include "Panel.h" // Asumo que tu clase Panel está aquí
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

// Pin DE del MAX485
#define DE_DDR   DDRD
#define DE_PORT  PORTD
#define DE_BIT   PD3

#if MODBUS_BAUDIOS <= 9600
#define TIMER2_CS_BITS  ((1 << CS22) | (1 << CS20))   // prescaler 128 → 4.096 ms
#elif MODBUS_BAUDIOS <= 19200
#define TIMER2_CS_BITS  ((1 << CS22))                  // prescaler 64  → 2.048 ms
#else
#define TIMER2_CS_BITS  ((1 << CS22) | (1 << CS21))   // prescaler 32  → 1.024 ms
#endif
/*    
volatile uint8_t  eco_buf[32];
volatile uint16_t eco_len = 0;
volatile bool     eco_ready = false;
volatile uint8_t eco_flags[32];
volatile uint16_t bytes_modbus = 0;
   */	
// ---------------- Buffer circular de recepción ----------------
#define MODBUS_RX_BUF_SIZE  64

static volatile uint8_t  rx_buf[MODBUS_RX_BUF_SIZE];
static volatile uint16_t rx_head = 0;   // escribe la ISR
static volatile uint16_t rx_tail = 0;   // lee modbus_port_read
volatile bool     frame_ready = false;
volatile uint16_t frame_len   = 0;

// ---------------- Inicialización ----------------
void modbus_timer_init(void) {
	TCCR2A = 0x00;                 // modo normal
	TCCR2B = 0x00;                 // detenido
	TIMSK2 = 0x00;                 // sin interrupciones aún
	TCNT2  = 0;
}

static inline void modbus_timer_restart(void) {
	TCNT2  = 0;
	TIFR2  = (1 << TOV2);          // limpia flag
	TIMSK2 |= (1 << TOIE2);        // habilita overflow
	TCCR2B = TIMER2_CS_BITS;       // arranca
}

static inline void modbus_timer_stop(void) {
	TCCR2B = 0x00;
	TIMSK2 &= ~(1 << TOIE2);
}

// ---------------- ISR de recepción ----------------
ISR(USART_RX_vect){
	uint8_t status = UCSR0A;
	uint8_t data   = UDR0;
	(void)status;

	uint16_t next = (rx_head + 1) % MODBUS_RX_BUF_SIZE;
	if (next != rx_tail) {
		rx_buf[rx_head] = data;
		rx_head = next;
	}
	// Reinicia temporizador de fin de trama
	modbus_timer_restart();
	//PORTD ^=(1<<PD3);
}

// ---------------- ISR del timer2 (fin de trama) ----------------
ISR(TIMER2_OVF_vect) {
	modbus_timer_stop();
    
	uint16_t len;
	if (rx_head >= rx_tail)
	len = rx_head - rx_tail;
	else
	len = MODBUS_RX_BUF_SIZE - rx_tail + rx_head;

	if (len > 0) {
		frame_len   = len;
		frame_ready = true;
		//PORTD ^=(1<<PD3);
	}
}

extern uint32_t getMillis(void);

// Puntero global a tu objeto Panel (o pásalo a través del argumento 'arg')
extern Panel panel;

// Callback para leer Holding Registers (Función 0x03)
// El maestro solicita 'quantity' registros a partir de 'address'.
//static 
nmbs_error read_holding_registers(uint16_t address, uint16_t quantity, uint16_t* registers, uint8_t unit_id, void* arg) {
	// 'arg' podría ser un puntero a tu objeto Panel. Aquí lo usamos directamente.
	// Panel* panel = (Panel*)arg;
	
	for (uint16_t i = 0; i < quantity; i++) {
		switch (address + i) {
			case 0x0000: // Modo de Operación
				registers[i] = 0;//panel.getOperationMode();
			break;
			case 0x0001: // Setpoint de Ángulo
				registers[i] = 0; //(uint16_t)(panel.getAngleSetpoint() * 10.0f);
			break;
			case 0x0002: // Umbral parada (x100)
				registers[i] = (uint16_t)(panel.getStopThreshold() * 100.0f);
			break;
			case 0x0003: // Kp
			  registers[i] = 21; //(uint16_t)(panel.getKp() * 1000.0f);
			break;
			// ... añade todos los casos para tus Holding Registers ...
			default:
			return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS ;
		}
	}
	return NMBS_ERROR_NONE;
}

// Callback para leer Input Registers (Función 0x04)
//static 
nmbs_error read_input_registers(uint16_t address, uint16_t quantity, uint16_t* registers, uint8_t unit_id, void* arg) {
	for (uint16_t i = 0; i < quantity; i++) {
		switch (address + i) {
			case 0x0010: // // Ángulo actual (x10)
			 registers[i] = 0;// (uint16_t)(panel.getCurrentAngle() * 10.0f);
			break;
			case 0x0011: // Voltaje Este
			 registers[i] = panel.getEastFiltered(); // Ya es uint16_t
			break;
			case 0x0020: // Temperatura
			 registers[i] = (uint16_t)(panel.readTemperature(6) * 10.0f);
			break;
			case 0x0030: // Estado de Límites (empaquetado en bits)
			{
				 uint16_t status = 0;
				 if (panel.limiteEste()) status |= (1 << 0);
				 if (panel.limiteOeste()) status |= (1 << 1);
				 if (panel.limiteHorizontal()) status |= (1 << 2);
				 registers[i] = status;
			 break;
		 }
		 default:
		 return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
	 }
 }
 return NMBS_ERROR_NONE;
 }

// Callback para escribir un solo Holding Register (Función 0x06)
static nmbs_error write_single_register(uint16_t address, uint16_t value, uint8_t unit_id, void* arg) {
	switch (address) {
		case 0x0000: // Modo de Operación
		// panel.setOperationMode((OperationMode)value);
		break;
		case 0x0001: // Setpoint de Ángulo
		// panel.setAngleSetpoint(value / 10.0f);
		break;
		case 0x0003: // Kp
		// panel.setKp(value / 1000.0f);
		break;
		default:
		return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
	}
	return NMBS_ERROR_NONE;
}

// Callback para escribir múltiples Holding Registers (Función 0x10)
static nmbs_error write_multiple_registers(uint16_t address, uint16_t quantity, const uint16_t* registers, uint8_t unit_id, void* arg) {
	for (uint16_t i = 0; i < quantity; i++) {
		// Llama a la lógica de escritura individual para cada registro
		nmbs_error err = write_single_register(address + i, registers[i], unit_id, arg);
		if (err != NMBS_ERROR_NONE) {
			return err;
		}
	}
	return NMBS_ERROR_NONE;
}
void modbus_port_init(uint32_t baudios) {
	// Configurar UART0 a 8N1, sin interrupciones (nanoMODBUS hace polling)
	uint16_t ubrr = (uint16_t)((F_CPU / (16UL * baudios)) - 1);
	UBRR0H = (uint8_t)(ubrr >> 8);
	UBRR0L = (uint8_t)(ubrr & 0xFF);

	UCSR0A = 0x00;
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);          // RX y TX activos, sin ISR
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);        // 8 bits, 1 stop, sin paridad

	// DE como salida, en reposo escuchando (recepción)
	DE_DDR  |=  (1 << DE_BIT);
	DE_PORT &= ~(1 << DE_BIT);

	(void)UCSR0A;  // lectura de cortesía para limpiar flags
}

// ---------------- Lectura desde buffer circular ----------------
int32_t modbus_port_read(uint8_t* buf, uint16_t count, int32_t byte_timeout_ms, void* arg) {
	(void)arg;
	(void)byte_timeout_ms;

	uint16_t i = 0;
	while (i < count && rx_tail != rx_head) {
		buf[i++] = rx_buf[rx_tail];
		rx_tail = (rx_tail + 1) % MODBUS_RX_BUF_SIZE;
	}
	return (int32_t)i;
}

int32_t modbus_port_write(const uint8_t* buf, uint16_t count, int32_t byte_timeout_ms, void* arg) {
	(void)byte_timeout_ms;
	(void)arg;
    
	// Activar driver RS-485
	DE_PORT |= (1 << DE_BIT);
	_delay_us(50);   // t_setup del MAX485

	for (uint16_t i = 0; i < count; i++) {
		while (!(UCSR0A & (1 << UDRE0))) { /* esperar buffer libre */ }
		UDR0 = buf[i];
	}

	// Esperar a que salga el último byte físicamente
	while (!(UCSR0A & (1 << TXC0))) { /* esperar transmisión completa */ }
	UCSR0A |= (1 << TXC0);   // limpiar flag TXC

	_delay_us(50);           // t_hold del MAX485 antes de volver a RX
	DE_PORT &= ~(1 << DE_BIT);

	return (int32_t)count;
}
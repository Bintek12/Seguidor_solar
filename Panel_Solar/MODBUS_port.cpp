#ifndef F_CPU
#define F_CPU 8000000UL
#endif

#include "DebugSerial.h"
#include "MODBUS_port.h"
#include "MODBUS.h"
#include "nanomodbus.h"

#include "Panel.h" 
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

// ---------------- Buffer circular de recepción ----------------
//#define MODBUS_RX_BUF_SIZE  64
// ---------------- Buffer circular ----------------
#define MODBUS_RX_BUF_SIZE  256

static volatile uint8_t  rx_buf[MODBUS_RX_BUF_SIZE];
static volatile uint16_t rx_head     = 0;   // escribe la ISR
static volatile uint16_t rx_tail     = 0;   // lee modbus_port_read
volatile bool            frame_ready = false;
volatile bool            tx_active   = false;

// ---------------- Timer2: timeout de fin de trama ----------------
// Prescaler 128 @ 8 MHz → 4.096 ms
#define TIMER2_CS_BITS  ((1 << CS22) | (1 << CS20))

// Usamos el puntero global definido en MODBUS.cpp
extern Panel* panel_ref;


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
ISR(USART_RX_vect) {
	uint8_t status = UCSR0A;
	uint8_t data   = UDR0;
	(void)status;

	if (tx_active) return;

	uint16_t next = (rx_head + 1) % MODBUS_RX_BUF_SIZE;
	if (next != rx_tail) {
		rx_buf[rx_head] = data;
		rx_head = next;
	}

	modbus_timer_restart();
}

ISR(TIMER2_OVF_vect) {
	modbus_timer_stop();
	if (rx_head != rx_tail) {
		frame_ready = true;
	}
}
extern uint32_t getMillis(void);


// Callback para leer Holding Registers (Función 0x03)
nmbs_error read_holding_registers(uint16_t address, uint16_t quantity, uint16_t* registers, uint8_t unit_id, void* arg) {
	// 'arg' podría ser un puntero a tu objeto Panel. Aquí lo usamos directamente.
	//Panel* panel = (Panel*)arg;
	Panel* p = panel_ref; 
	
	for (uint16_t i = 0; i < quantity; i++) {
		switch (address + i) {
			case 0x0000: // Modo de Operación
				registers[i] = 0;
			break;
			case 0x0001: // Setpoint de Ángulo
				registers[i] = 45; 
			break;
			case 0x0002: // Umbral parada (x100)
				registers[i] = (uint16_t)(p->getStopThreshold() * 100.0f);
			break;
			case 0x0003: // Kp
			  registers[i] = 21; 
			break;
			// ... añade todos los casos para tus Holding Registers ...
			default:
			return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS ;
		}
	}
	return NMBS_ERROR_NONE;
}

// Callback para leer Input Registers (Función 0x04)
nmbs_error read_input_registers(uint16_t address, uint16_t quantity, uint16_t* registers, uint8_t unit_id, void* arg) {
	//Panel* panel = (Panel*)arg;
	Panel* p = panel_ref; 
	for (uint16_t i = 0; i < quantity; i++) {
		switch (address + i) {
			case 0x0010: 
			 registers[i] = (p->getEastFiltered()); 
			break;
			case 0x0011: 
			 registers[i] = (p->getWestFiltered()); 
			break;
			case 0x0012:
			registers[i] = (p->getError());
			break;
			case 0x0013:
			registers[i] = (p->getError());
			break;
			case 0x0014:
			registers[i] = (p->getStopThreshold());
			break;
			case 0x0015:
			registers[i] = (p->getCurrentError());
			break;
			case 0x0016:
			registers[i] = (p->getPIDOutput());
			break;
			case 0x0017:
			registers[i] = p->readTemperature(6)*10; 
			break;
			case 0x0018:
			registers[i] = p->readTemperature(0)*10;
			break;
			case 0x0020: // Temperatura
			 registers[i] = (uint16_t)(p->readTemperature(6) * 10.0f);
			break;
			case 0x0080: // Estado de Límites (empaquetado en bits)
			{
				 uint16_t status = 0;
				 if (p->limiteEste()) status |= (1 << 0);
				 if (p->limiteOeste()) status |= (1 << 1);
				 if (p->limiteHorizontal()) status |= (1 << 2);
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

// ---------------- Lectura circular ----------------
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

// ---------------- Escritura con anti-eco ----------------
int32_t modbus_port_write(const uint8_t* buf, uint16_t count, int32_t byte_timeout_ms, void* arg) {
	(void)byte_timeout_ms;
	(void)arg;

	tx_active = true;
	modbus_timer_stop();

	DE_PORT |= (1 << DE_BIT);
	_delay_us(50);

	for (uint16_t i = 0; i < count; i++) {
		while (!(UCSR0A & (1 << UDRE0))) { }
		UDR0 = buf[i];
	}
	while (!(UCSR0A & (1 << TXC0))) { }
	UCSR0A |= (1 << TXC0);

	_delay_us(50);
	DE_PORT &= ~(1 << DE_BIT);
	_delay_us(100);

	while (UCSR0A & (1 << RXC0)) { (void)UDR0; }
	rx_head = rx_tail = 0;
    frame_ready = false;
	tx_active = false;
	return (int32_t)count;
}
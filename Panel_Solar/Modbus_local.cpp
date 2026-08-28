#include "main.h"
#include "Modbus_local.h"
#include <avr/io.h>
#include <avr/interrupt.h>

// Definición única de las variables globales
volatile uint8_t rx_buffer[MODBUS_BUFFER_SIZE];
volatile uint16_t holding_registers[HOLDING_REG_COUNT];
volatile uint32_t system_ms = 0;


uint16_t Modbus_Read_Register(uint16_t reg) {
	if (reg < HOLDING_REG_COUNT) return holding_registers[reg];
	return 0;
}

// Calcula CRC16 Modbus RTU
uint16_t crc16(const uint8_t *buf, uint8_t len) {
	uint16_t crc = 0xFFFF;
	for (uint8_t pos = 0; pos < len; pos++) {
		crc ^= buf[pos]; // XOR byte con CRC
		for (uint8_t i = 0; i < 8; i++) {
			if (crc & 0x0001) {
				crc >>= 1;
				crc ^= 0xA001; // polinomio Modbus
				} else {
				crc >>= 1;
			}
		}
	}
	return crc;
}


void Modbus_Service(void) {
	if (!flags.datos_listos) return;   // no hay trama completa
	flags.datos_listos = false;        // limpiar flag

	if (usart.rx_index < 7) {
		usart.rx_index = 0;
		return;
	} // trama mínima

	uint8_t addr = usart.rx_buffer[0];
	uint8_t func = usart.rx_buffer[1];
	uint16_t start = (usart.rx_buffer[2] << 8) | usart.rx_buffer[3];
	uint16_t count_or_val = (usart.rx_buffer[4] << 8) | usart.rx_buffer[5];

	uint16_t crc_rx = (usart.rx_buffer[usart.rx_index - 2]) |
	(usart.rx_buffer[usart.rx_index - 1] << 8);
	uint16_t crc_calc = crc16((const uint8_t*)usart.rx_buffer, usart.rx_index - 2);

	if (addr != 0x0A || crc_rx != crc_calc) {
		usart.rx_index = 0;
		return;
	}

	// ---------------------------
	// Función 0x03: Read Holding Registers
	// ---------------------------
	if (func == 0x03) {
		uint8_t resp[64];
		resp[0] = addr;
		resp[1] = func;
		resp[2] = count_or_val * 2;

		for (uint16_t i = 0; i < count_or_val; i++) {
			uint16_t val = Modbus_Read_Register(start + i);
			resp[3 + i*2] = val >> 8;
			resp[4 + i*2] = val & 0xFF;
		}

		uint16_t crc = crc16(resp, 3 + count_or_val*2);
		resp[3 + count_or_val*2] = crc & 0xFF;
		resp[4 + count_or_val*2] = crc >> 8;

		for (uint8_t i = 0; i < 5 + count_or_val*2; i++) {
			while (!(UCSR0A & (1 << UDRE0)));
			UDR0 = resp[i];
		}
	}

	// ---------------------------
	// Función 0x06: Write Single Register
	// ---------------------------
	else if (func == 0x06) {
		uint16_t value = count_or_val; // valor a escribir

		// Actualiza el registro en el micro
		Modbus_Write_Register(start, value);

		// Eco de la petición como respuesta (Modbus estándar)
		uint8_t resp[8];
		for (uint8_t i = 0; i < 6; i++) {
			resp[i] = usart.rx_buffer[i]; // dirección, función, reg, valor
		}

		uint16_t crc = crc16(resp, 6);
		resp[6] = crc & 0xFF;
		resp[7] = crc >> 8;

		for (uint8_t i = 0; i < 8; i++) {
			while (!(UCSR0A & (1 << UDRE0)));
			UDR0 = resp[i];
		}
	}

	usart.rx_index = 0; // limpiar buffer
}

void Modbus_Write_Register(uint16_t reg, uint16_t value) {
	if (reg < HOLDING_REG_COUNT) {
		holding_registers[reg] = value;

		// Aquí puedes disparar acciones según el registro
		// Ejemplo:
		if (reg == 0x0005) {   // registro de alarma
			if (value == 1) {
				// activar alarma
				} else {
				// desactivar alarma
			}
		}
	}
}


void Modbus_Update_Registers(void) {
	// Actualiza con tus sensores
	holding_registers[0] = 10/* temp panel 1 */;
	holding_registers[1] = 20/* temp panel 2 */;
	holding_registers[2] = 30/* LDR Este */;
	holding_registers[3] = 40/* LDR Oeste */;
	holding_registers[4] = 50/* Azimut */;
	holding_registers[5] = 60/* alarma */;
	holding_registers[0x10] = 100/* segundos */;
	holding_registers[0x11] = 200/* minutos */;
	holding_registers[0x12] = 300/* horas */;
	holding_registers[0x13] = 400/* día */;
	holding_registers[0x14] = 0x0826/* mes/año */;
}

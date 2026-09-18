// MODBUS.cpp
#include <avr/io.h>
#include "MODBUS.h"
#include "MODBUS_port.h"
//#include "nanomodbus.h"   // ajusta si tu header se llama distinto



// -------------------- Estado interno --------------------
nmbs_t   nmbs;

static Panel*   panel_ref = nullptr;

// -------------------- Helpers de conversión --------------------
static inline uint16_t to_u16(float v)        { return (uint16_t)(v + 0.5f); }
static inline uint16_t to_i16_x100(float v)   { return (uint16_t)(int16_t)(v * 100.0f); }
static inline uint16_t to_i16_x10 (float v)   { return (uint16_t)(int16_t)(v * 10.0f);  }

nmbs_error nmbs_server_create(nmbs_t* nmbs, uint8_t address_rtu,
const nmbs_platform_conf* platform_conf,
const nmbs_callbacks* callbacks);


// -------------------- Lectura de registros --------------------
static uint16_t leer_registro(uint16_t addr) {
	switch (addr) {
		case REG_ESTE_FILTRADO:
		return to_u16(panel_ref->getEastFiltered());

		case REG_OESTE_FILTRADO:
		return to_u16(panel_ref->getWestFiltered());

		case REG_ERROR_PID_X100:
		return to_i16_x100(panel_ref->getCurrentError());

		case REG_SALIDA_PID_X100:
		return to_i16_x100(panel_ref->getPIDOutput());

		case REG_TEMPERATURA_X10:
		return to_i16_x10(panel_ref->readTemperature(6));  // ADC6

		case REG_ESTADO_LIMITES: {
			uint16_t b = 0;
			if (panel_ref->limiteEste())       b |= (1 << 0);
			if (panel_ref->limiteOeste())      b |= (1 << 1);
			if (panel_ref->limiteHorizontal()) b |= (1 << 2);
			return b;
		}

		case REG_ALARMA:
		return panel_ref->isAlarm() ? 1 : 0;

		case REG_STATUS_MSG:
		return 0;   // reservado; puedes mapear getStatusMessage() a un código

		default:
		return 0;
	}
}

// -------------------- Callback de nanoMODBUS --------------------


// -------------------- API pública --------------------
void modbus_init(Panel* panel, uint32_t baudios) {
	panel_ref = panel;

	modbus_port_init(baudios);

	// 1- Configurar transporte RTU con nuestros callbacks de bajo nivel
	nmbs_platform_conf platform_conf;
	nmbs_platform_conf_create(&platform_conf);
	platform_conf.transport = NMBS_TRANSPORT_RTU;
	platform_conf.read      = modbus_port_read;
	platform_conf.write     = modbus_port_write;
	platform_conf.arg       = nullptr;

	// 2- Configurar callbacks de datos
	nmbs_callbacks callbacks;
	nmbs_callbacks_create(&callbacks);
	
	// 3. Asignar SOLO las que has implementado
	callbacks.read_holding_registers = read_holding_registers;
	callbacks.read_input_registers   = read_input_registers;
	// callbacks.write_single_register = write_single_register; // cuando la implementes
	// callbacks.write_multiple_registers = write_multiple_registers;
	// (los demás quedan nullptr → nanoMODBUS devuelve excepción si los piden)
	
    // 4. Crear el servidor con las callbacks
    nmbs_error err = nmbs_server_create(&nmbs, MODBUS_SLAVE_ID, &platform_conf, &callbacks);
if (err != NMBS_ERROR_NONE) {
    // LED fijo si falla, visible a simple vista
    PORTD |= (1 << PD3);
    DDRD |= (1 << PD3);
   }
}


/*
void modbus_poll(void) {
	nmbs_server_poll(&nmbs);
}
*/
#include "MODBUS.h"
#include "MODBUS_port.h"
#include "nanoMODBUS.h"   // ajusta si tu header se llama distinto

// -------------------- Estado interno --------------------
static nmbs_t   nmbs;
static Panel*   panel_ref = nullptr;

// -------------------- Helpers de conversión --------------------
static inline uint16_t to_u16(float v)        { return (uint16_t)(v + 0.5f); }
static inline uint16_t to_i16_x100(float v)   { return (uint16_t)(int16_t)(v * 100.0f); }
static inline uint16_t to_i16_x10 (float v)   { return (uint16_t)(int16_t)(v * 10.0f);  }

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
static nmbs_error on_read_holding_registers(uint16_t address, uint16_t quantity,
uint16_t* regs_out, uint8_t unit_id,
void* arg) {
	(void)unit_id;   // no lo usamos: solo hay un esclavo
	(void)arg;
	if (address + quantity > REG_COUNT) {
		return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
	}
	for (uint16_t i = 0; i < quantity; i++) {
		regs_out[i] = leer_registro(address + i);
	}
	return NMBS_ERROR_NONE;
}

// -------------------- API pública --------------------
void modbus_init(Panel* panel, uint32_t baudios) {
	panel_ref = panel;

	modbus_port_init(baudios);

	// Configurar transporte RTU con nuestros callbacks de bajo nivel
	nmbs_platform_conf platform_conf;
	nmbs_platform_conf_create(&platform_conf);
	platform_conf.transport = NMBS_TRANSPORT_RTU;
	platform_conf.read      = modbus_port_read;
	platform_conf.write     = modbus_port_write;
	platform_conf.arg       = nullptr;

	// Configurar callbacks de datos
	nmbs_callbacks callbacks;
	nmbs_callbacks_create(&callbacks);
	callbacks.read_holding_registers = on_read_holding_registers;
	// (los demás quedan nullptr → nanoMODBUS devuelve excepción si los piden)

	// Crear el servidor (esclavo)
	nmbs_server_create(&nmbs, MODBUS_SLAVE_ID, &platform_conf, &callbacks);
}

void modbus_poll(void) {
	nmbs_server_poll(&nmbs);
}
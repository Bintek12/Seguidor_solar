// MODBUS.cpp
#include <avr/io.h>
#include "MODBUS.h"
#include "MODBUS_port.h"



// -------------------- Estado interno --------------------
// Declaración global del puntero al objeto Panel
nmbs_t   nmbs;
Panel* panel_ref = nullptr;   


//static Panel*   panel_ref = nullptr;

nmbs_error nmbs_server_create(nmbs_t* nmbs, uint8_t address_rtu,
const nmbs_platform_conf* platform_conf,
const nmbs_callbacks* callbacks);

// -------------------- API pública --------------------
void modbus_init(Panel* panel, uint32_t baudios) {
	panel_ref = panel;   // guardar puntero real

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
	callbacks.write_single_register = write_single_register; // cuando la implementes
	callbacks.write_multiple_registers = write_multiple_registers;
	// (los demás quedan nullptr → nanoMODBUS devuelve excepción si los piden)
	
    // 4. Crear el servidor con las callbacks
    nmbs_error err = nmbs_server_create(&nmbs, MODBUS_SLAVE_ID, &platform_conf, &callbacks);
if (err != NMBS_ERROR_NONE) {
    // LED fijo si falla, visible a simple vista
    PORTD |= (1 << PD3);
    DDRD |= (1 << PD3);
   }
}

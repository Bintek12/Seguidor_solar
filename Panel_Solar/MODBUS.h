#ifndef PANEL_SOLAR_MODBUS_H
#define PANEL_SOLAR_MODBUS_H

#include <stdint.h>
#include <stdbool.h>
#include "Panel.h"
#include "nanomodbus.h"   // ajusta si tu header se llama distinto

// ID de esclavo MODBUS (0x47 = 71 decimal)
#define MODBUS_SLAVE_ID     0x1
#define MODBUS_BAUDIOS     9600UL
#define MODBUS_BYTE_TIMEOUT 5   // ms de silencio por byte


extern nmbs_t nmbs;


// Inicializa la capa MODBUS. Debe llamarse una vez tras panel.init().
void modbus_init(Panel* panel, uint32_t baudios);




#endif
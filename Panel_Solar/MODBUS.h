#ifndef PANEL_SOLAR_MODBUS_H
#define PANEL_SOLAR_MODBUS_H

#include <stdint.h>
#include <stdbool.h>
#include "Panel.h"

// ID de esclavo MODBUS (0x47 = 71 decimal)
#define MODBUS_SLAVE_ID     0x47
#define MODBUS_BAUDIOS      38400UL
#define MODBUS_BYTE_TIMEOUT 5   // ms de silencio por byte

// Mapa de registros (holding registers, FC 0x03)
enum RegsModbus : uint16_t {
	REG_ESTE_FILTRADO   = 0,  // uint16, 0..1023  - lectura ADC Este filtrada
	REG_OESTE_FILTRADO  = 1,  // uint16, 0..1023  - lectura ADC Oeste filtrada
	REG_ERROR_PID_X100  = 2,  // int16, error * 100  (con signo)
	REG_SALIDA_PID_X100 = 3,  // int16, pidOutput * 100 (con signo)
	REG_TEMPERATURA_X10 = 4,  // int16, °C * 10 (con signo)
	REG_ESTADO_LIMITES  = 5,  // bitfield: b0=Este, b1=Oeste, b2=Horizontal
	REG_ALARMA          = 6,  // 0 = OK, 1 = alarma activa
	REG_STATUS_MSG      = 7,  // código de estado (entero)
	REG_COUNT           = 8   // total de registros
};

// Inicializa la capa MODBUS. Debe llamarse una vez tras panel.init().
void modbus_init(Panel* panel, uint32_t baudios);

// Procesa peticiones pendientes. Llamar en cada iteración del loop().
void modbus_poll(void);

#endif
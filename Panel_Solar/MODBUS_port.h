#ifndef PANEL_SOLAR_MODBUS_PORT_H
#define PANEL_SOLAR_MODBUS_PORT_H

#include "nanomodbus.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


extern volatile bool     frame_ready ;
void     modbus_port_init(uint32_t baudios);
int32_t  modbus_port_read (uint8_t* buf, uint16_t count, int32_t byte_timeout_ms, void* arg);
int32_t  modbus_port_write(const uint8_t* buf, uint16_t count, int32_t byte_timeout_ms, void* arg);
void modbus_timer_init(void);
uint16_t leerLDR(uint8_t canal);
//static 
nmbs_error read_holding_registers(uint16_t address, uint16_t quantity, uint16_t* registers, uint8_t unit_id, void* arg);
//static
nmbs_error read_input_registers(uint16_t address, uint16_t quantity, uint16_t* registers, uint8_t unit_id, void* arg);

#endif
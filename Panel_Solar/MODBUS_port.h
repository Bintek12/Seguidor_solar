#ifndef PANEL_SOLAR_MODBUS_PORT_H
#define PANEL_SOLAR_MODBUS_PORT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

void     modbus_port_init(uint32_t baudios);
int32_t  modbus_port_read (uint8_t* buf, uint16_t count, int32_t byte_timeout_ms, void* arg);
int32_t  modbus_port_write(const uint8_t* buf, uint16_t count, int32_t byte_timeout_ms, void* arg);

#endif
#ifndef MODBUS_LOCAL_H
#define MODBUS_LOCAL_H

#include <stdint.h>

#define SLAVE_ID            0x0A
#define MODBUS_BUFFER_SIZE  64
#define HOLDING_REG_COUNT   32

extern volatile uint16_t holding_registers[HOLDING_REG_COUNT];

void Modbus_Init(void);
void Modbus_Service(void);
uint16_t Modbus_Read_Register(uint16_t reg);
void Modbus_Update_Registers(void);

#endif


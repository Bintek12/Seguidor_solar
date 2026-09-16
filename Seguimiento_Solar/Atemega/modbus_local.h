// modbus_local.h
#ifndef MODBUS_LOCAL_H
#define MODBUS_LOCAL_H

#include <stdint.h>
#include "tracker.h"   // aquí ya conoce TrackerMode

extern uint16_t regOperation[16];
extern uint16_t regHistory[120];
extern uint16_t regStatus[16];
extern uint16_t regElevation[16];
extern uint16_t regCalibration[16];
extern uint16_t regDiagnostics[16];
extern uint16_t holdingRegisters[128];

void modbusInit();
void modbusUpdate(int ldrLeft, int ldrRight, TrackerMode mode);
void modbusWriteRegister(uint16_t reg, uint16_t value);

#endif

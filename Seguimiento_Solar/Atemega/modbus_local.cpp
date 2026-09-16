

#include "modbus_local.h"
#include "logger.h"
#include "eeprom.h"
#include "tracker.h"
#include "motor.h"


// Bloques de registros Modbus
// --- Operación básica ---
uint16_t regOperation[16];   // 0x0000–0x000F
// --- Histórico de lecturas ---
uint16_t regHistory[120];     // 0x0010–0x004F
// --- Estado del sistema ---
uint16_t regStatus[16];      // 0x0100–0x010F
// --- Elevación futura ---
uint16_t regElevation[16];   // 0x0200–0x020F
// --- Calibración ---
uint16_t regCalibration[16]; // 0x0300–0x030F
// --- Diagnóstico ---
uint16_t regDiagnostics[16]; // 0x0400–0x040F


// Definición de registros Modbus
uint16_t holdingRegisters[128];
uint16_t inputRegisters[16];

void modbusInit() {
	// Inicializar registros de operación
	regOperation[0] = 0; // LDR Izq
	regOperation[1] = 0; // LDR Der
	regOperation[2] = loadMode(); // Modo desde EEPROM
	regOperation[3] = 0; // Retorno al origen (0=Normal, 1=Forzar)
}
void modbusUpdate(int ldrLeft, int ldrRight, TrackerMode mode) {
	// Actualizar bloque de operación
	regOperation[0] = ldrLeft;
	regOperation[1] = ldrRight;
	regOperation[2] = (uint16_t)mode;

	// Histórico: pares de lecturas LDR
	for (int i = 0; i < 60; i++) {
		regHistory[i*2]     = logBuffer[i].ldrLeft;
		regHistory[i*2 + 1] = logBuffer[i].ldrRight;
	}

	// Estado del sistema
	regStatus[0] = getMotorState();   // 0=STOP, 1=LEFT, 2=RIGHT
	regStatus[1] = getLimitStatus();  // Bit0=Left, Bit1=Right

	// Reservados para elevación, calibración y diagnóstico
	// regElevation[...] → futuro control de inclinación
	// regCalibration[...] → offsets, deadzone
	// regDiagnostics[...] → errores, temperatura, voltaje

	// Servicio Modbus
	//ModBusService();  BGR que es esto
}

// Función principal de servicio Modbus
void modbusRespond() {
	//ModBusService(); // llamada a la librería avrModBus
}

void modbusWriteRegister(uint16_t reg, uint16_t value) {
	if (reg == 2) { // Modo de operación
		regOperation[2] = value;
		saveMode((uint8_t)value);
	}
	else if (reg == 3) { // Retorno al origen
		regOperation[3] = value;
		if (value == 1) {
			trackerReturnHome();
			regOperation[3] = 0; // reset automático
		}
	}
}


/*


#include "modbus.h"
#include "logger.h"
//#include "eeprom.h"
#include "tracker.h"

// Arrays de registros Modbus
uint16_t holdingRegisters[128];
uint16_t inputRegisters[16];

void modbusInit() {
	holdingRegisters[0] = 0; // LDR Izq
	holdingRegisters[1] = 0; // LDR Der
	holdingRegisters[2] = loadMode(); // Modo desde EEPROM
}

// Actualización de registros
void modbusUpdate(int ldrLeft, int ldrRight, TrackerMode mode) {
	holdingRegisters[0] = ldrLeft;
	holdingRegisters[1] = ldrRight;
	holdingRegisters[2] = (uint16_t)mode;

	for (int i = 0; i < 60; i++) {
		holdingRegisters[16 + i*2]     = logBuffer[i].ldrLeft;
		holdingRegisters[16 + i*2 + 1] = logBuffer[i].ldrRight;
	}

	inputRegisters[0] = getMotorState();
	inputRegisters[1] = getLimitStatus();

	ModBusService();
}

// Callback de escritura en registros Modbus
// Esta función se llama cuando el maestro escribe en un Holding Register
void modbusWriteRegister(uint16_t reg, uint16_t value) {
	if (reg == 2) { // Registro de modo de operación
		holdingRegisters[2] = value;
		saveMode((uint8_t)value); // Guardar en EEPROM
	}
}
*/
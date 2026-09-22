#ifndef MODBUS_REGS_H
#define MODBUS_REGS_H
#include <stdint.h> 
/* ===== Mapa de registros Modbus del seguidor solar ===== */

/* Búfer historico de muestras LDR (100..219) */
#define MODBUS_LOG_BASE_ADDR   100
#define MODBUS_LOG_COUNT       120

/* Registros de configuración */
#define MODBUS_INTERVAL_REG    300   /* intervalo en segundos */
#define MODBUS_TRIGGER_REG     302   /* escribir 1 ? forzar muestra */

/* Contador total de muestras (32 bits) */
#define MODBUS_TOTAL_LOW_REG    301
#define MODBUS_TOTAL_HIGH_REG   303

/* Estado del búfer */
#define MODBUS_NEXT_SLOT_REG    304
#define MODBUS_BUFFER_CAP_REG   305
#define MODBUS_STATUS_REG       306

/* Diagnóstico rápido */
#define MODBUS_LAST_SAMPLE_REG  307
/* Añade aqui otros registros que ya tuvieras:
   #define MODBUS_...  ... */
extern volatile uint16_t sample_interval_s;   // ? declara, no define

#endif /* MODBUS_REGS_H */
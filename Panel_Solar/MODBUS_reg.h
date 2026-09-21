#ifndef MODBUS_REGS_H
#define MODBUS_REGS_H

/* ===== Mapa de registros Modbus del seguidor solar ===== */

/* Búfer historico de muestras LDR (100..219) */
#define MODBUS_LOG_BASE_ADDR   100
#define MODBUS_LOG_COUNT       120

/* Registros de configuración */
#define MODBUS_INTERVAL_REG    300   /* intervalo en segundos */
#define MODBUS_COUNT_REG       301   /* nº de muestras válidas */
#define MODBUS_TRIGGER_REG     302   /* escribir 1 ? forzar muestra */

/* Añade aqui otros registros que ya tuvieras:
   #define MODBUS_...  ... */

#endif /* MODBUS_REGS_H */
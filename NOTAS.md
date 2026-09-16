# Estado del proyecto

## 2026-09-16 — Rama MODBUS
- nanoMODBUS integrado como esclavo RTU
- Baudios: 38400, ID: 0x47, 8N1
- Mapa: 8 holding registers (ver MODBUS.h)
- Compila sin errores ni warnings
- **PENDIENTE**: probar con maestro MODBUS (QModMaster / pymodbus)
- Pin DE: PD3
- UART: USART0 (PD0/PD1), sin ISR (nanoMODBUS hace polling)
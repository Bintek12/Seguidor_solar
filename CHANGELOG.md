# Changelog

Todos los cambios relevantes de este proyecto se documentan en este archivo.
El formato sigue las convenciones de [Keep a Changelog](https://keepachangelog.com/es-ES/1.1.0/)
y el proyecto usa [Versionado Semántico](https://semver.org/lang/es/).

---

## [1.1] — 2026-09-22

### Añadido

- **Registro 301** `TOTAL_LOW`: 16 bits bajos del contador total de muestras.
- **Registro 303** `TOTAL_HIGH`: 16 bits altos del contador total.
- **Registro 304** `NEXT_SLOT`: índice del próximo slot a escribir en el búfer.
- **Registro 305** `BUFFER_CAP`: capacidad del búfer (constante 120).
- **Registro 306** `STATUS_FLAGS`: bits de estado (lleno, overflow).
- **Registro 307** `LAST_SAMPLE`: último valor muestreado.
- Persistencia del intervalo de muestreo en EEPROM (registro 300).
- Función `eeprom_log_status_flags()` para diagnóstico del búfer.
- Función `eeprom_log_last_sample()` para acceso rápido al último valor.
- Documentación `PROTOCOLO.md` con el mapa completo de registros, tramas
  Modbus de ejemplo y protocolo de poll incremental para el maestro.

### Cambiado

- `read_holding_registers` refactorizado en **bloques contiguos** que
  soportan lecturas de cualquier `quantity` dentro del rango (antes
  cada registro exigía `quantity == 1`).
- `panel_ref` protegido contra `nullptr` en todos los callbacks.

### Eliminado

- **Módulo `DebugSerial`** eliminado del build. Evitaba conflictos con
  nanoMODBUS por compartir el mismo UART0 y activar la interrupción de
  recepción, lo que interrumpía el polling de Modbus.
- Registros temporales `350..353` usados durante el desarrollo del
  contador total (reemplazados por 301 y 303).
- Tag `v1.0-buffer-eeprom-funcional` (redundante con `v1.0`).

### Corregido

- **Bug crítico**: `sample_interval_s` estaba duplicada como `static` en
  `main.h` y `MODBUS_port.cpp`. Ahora hay una única definición en
  `main.cpp` y un `extern` en `MODBUS_reg.h`.
- **Bug de lectura del registro 301**: `MODBUS_COUNT_REG` y
  `MODBUS_TOTAL_LOW_REG` apuntaban a la misma dirección, y el primero
  devolvía siempre 120 en lugar del contador real. Eliminada la macro
  duplicada.
- **Bug del registro 302**: no era legible, lo que hacía fallar la
  lectura contigua de `301..307`. Ahora devuelve 0.
- `MODBUS_reg.h` ahora incluye `<stdint.h>` antes de cualquier uso de
  tipos `uintN_t`.

### Verificado en QModbus

- Lectura contigua de 1, 7 y 120 registros sin excepciones.
- Contador total persistente tras apagado y encendido.
- Rotación del búfer correcta (registro 219 = muestra más reciente).
- Intervalo configurado persiste en EEPROM.
- Registro 307 coincide con el registro 219.

---

## [1.0] — 2026-09-21

### Añadido

- **Búfer circular de 120 muestras** en EEPROM interna del ATmega168A.
- **Wear leveling** con algoritmo basado en AVR101 (Microchip) usando
  búfer de estado de 120 bytes.
- **CRC-8 por muestra** (3 bytes por registro: `[low, high, crc]`).
- **Registro 300** `INTERVAL_REG`: intervalo de muestreo configurable.
- **Registro 302** `TRIGGER_REG`: fuerza una muestra manual.
- **Registros 100..219**: búfer histórico de 120 muestras.
- Función `eeprom_log_init()` con firma `0xB8` para formateo inicial.
- Función `eeprom_log_write()` con escritura atómica (protección `cli/sei`).
- Función `eeprom_log_read()` con traducción de índice (más antiguo primero).
- Función `eeprom_log_next_slot()` con detección de overflow de byte.
- Callbacks Modbus: `read_holding_registers`, `read_input_registers`,
  `write_single_register`, `write_multiple_registers`.
- Muestreo autónomo configurable por el maestro.

### Cambiado

- Modo de operación del búfer: paso de escritura lineal con detención a
  búfer circular con wear leveling.
- Estructura de registro: de 4 bytes (slot_index + valor + checksum) a
  3 bytes (valor + CRC), permitiendo 120 slots en 512 bytes.

### Corregido

- Algoritmo de búsqueda del último slot: de "primer break en la secuencia"
  a "min/max sobre el búfer de estado", robusto frente al desbordamiento
  del contador de 8 bits.

---

## [0.3] — 2026-09-20

### Añadido

- Ajustes de la lectura real de los registros Modbus.
- Actualización de las variables expuestas en Modbus.
- Tag `V0.3-registrosMODBUS`.

### Corregido

- Comunicación estable con buffer circular y mecanismo anti-eco.

---

## [0.2] — 2026-09-19

### Añadido

- Módulo Modbus funcional (compilación y lectura).
- Comunicación estable con nanoMODBUS sobre RS485.
- Tags `v0.2-modbus-compila`, `v0.2-modbus-lectura`.

---

## [0.1] — 2026-09-18

### Añadido

- Estructura inicial del proyecto.
- Optimización de uso de RAM.
- Control básico de motor y lectura de LDR.
- Tag `v0.1-optimizado-ram`.

---

## Tipos de cambios

- `Añadido` — nuevas funcionalidades.
- `Cambiado` — cambios en funcionalidades existentes.
- `Obsoleto` — funcionalidades que se eliminarán pronto.
- `Eliminado` — funcionalidades eliminadas.
- `Corregido` — corrección de bugs.
- `Seguridad` — vulnerabilidades corregidas.

---

## Enlaces

- [`README.md`](README.md) — descripción general del proyecto
- [`PROTOCOLO.md`](PROTOCOLO.md) — documentación del protocolo Modbus
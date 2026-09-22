# Protocolo Modbus RTU — Panel Solar ATmega168A

**Versión del firmware:** v1.1
**Última actualización:** 22/Sep/2026
**Autor:** [tu nombre]
**Hardware:** ATmega168A + nanoMODBUS + transceptor RS485 half-duplex

---

## 1. Resumen

El Panel Solar actúa como **esclavo Modbus RTU** en un bus RS485 half-duplex. Un
master externo (PC, PLC, SCADA) interroga los registros para leer el histórico
de muestras solares y configurar el comportamiento del seguidor.

| Parámetro | Valor |
|---|---|
| Modo | Modbus RTU |
| Interfaz | RS485 half-duplex |
| Baud rate | 9600 bps (configurable en firmware) |
| Formato | 8N1 (8 data bits, no parity, 1 stop bit) |
| Slave ID | `1` (por defecto) |
| Funciones soportadas | `0x03` (Read Holding Registers), `0x06` (Write Single Register), `0x10` (Write Multiple Registers) |
| Endianness | Big-endian en el cable (estándar Modbus) |

---

## 2. Mapa de registros

### Convención de direcciones

Todas las direcciones son **0-based** (las que van en la trama Modbus).
Si tu cliente Modbus usa direccionamiento 1-based, resta 1 al valor mostrado.

### 2.1. Búfer histórico de muestras — `100..219`

Búfer circular de 120 muestras. Cada slot contiene el valor filtrado del LDR
este del panel, muestreado a intervalos configurables.

| Rango | Nombre | R/W | Formato | Descripción |
|---|---|---|---|---|
| `100` | `LOG[0]` | R | uint16 | Muestra más antigua del búfer |
| `101` | `LOG[1]` | R | uint16 | Segunda más antigua |
| … | … | … | … | … |
| `218` | `LOG[118]` | R | uint16 | Penúltima muestra |
| `219` | `LOG[119]` | R | uint16 | **Muestra más reciente** |

**Valores válidos:**
- `0` – `1023`: valor crudo o filtrado del ADC de 10 bits.
- `0xFFFF`: **muestra corrupta** (CRC inválido). El master debe descartarla.

**Lecturas típicas:**
- **Completa:** `Read 100, qty=120` ? 120 muestras en una sola trama.
- **Ligera:** `Read <base>, qty=N` ? las N más recientes.

---

### 2.2. Configuración y estado — `300..307`

| Reg | Nombre | R/W | Rango | Descripción |
|---|---|---|---|---|
| `300` | `INTERVAL_REG` | R/W | 1–65535 | Intervalo de muestreo en **segundos**. Persiste en EEPROM. Escribir `0` ? excepción `0x03` |
| `301` | `TOTAL_LOW` | R | 0–65535 | 16 bits **bajos** del contador total de muestras |
| `302` | `TRIGGER_REG` | R/W | 0/1 | **Leer** ? siempre `0`. **Escribir `1`** ? fuerza una muestra inmediata |
| `303` | `TOTAL_HIGH` | R | 0–65535 | 16 bits **altos** del contador total |
| `304` | `NEXT_SLOT` | R | 0–119 | Próximo slot a escribir (posición interna del búfer) |
| `305` | `BUFFER_CAP` | R | 120 (fijo) | Capacidad total del búfer |
| `306` | `STATUS_FLAGS` | R | 0–3 | `bit0` = búfer lleno; `bit1` = hubo overflow |
| `307` | `LAST_SAMPLE` | R | 0–1023 / 0xFFFF | Último valor escrito (equivale a `LOG[119]`) |

**Persistencia:** solo `INTERVAL_REG` y los contadores internos (`passes`) sobreviven
a un reinicio. El resto se recalcula.

---

### 2.3. Registros legacy de configuración — `0..3` (opcional)

| Reg | Nombre | R/W | Descripción |
|---|---|---|---|
| `0` | `OP_MODE` | R | Modo de operación (actualmente 0) |
| `1` | `ANGLE_SETPOINT` | R | Setpoint de ángulo (actualmente 45) |
| `2` | `STOP_THRESHOLD` | R | Umbral de parada ×100 |
| `3` | `KP` | R | Ganancia proporcional ×1000 (actualmente 21) |

> Estos registros existen por compatibilidad. Pueden eliminarse en versiones futuras.

---

## 3. Cálculos derivados

### 3.1. Contador total de muestras (32 bits)

El contador se compone de dos registros contiguos:

```
total_samples = (TOTAL_HIGH << 16) | TOTAL_LOW
              = (reg[303] << 16) | reg[301]
```

Rango efectivo: 0 a 4 294 967 295 muestras (˜ 136 años a 1 muestra/minuto).

### 3.2. Otros derivados

| Concepto | Fórmula | Ejemplo (total=434, cap=120) |
|---|---|---|
| `passes` (vueltas completas) | `total_samples / BUFFER_CAP` | 3 |
| `next_slot` | `total_samples % BUFFER_CAP` | 74 |
| Muestras disponibles | `min(total_samples, BUFFER_CAP)` | 120 |
| Búfer lleno | `total_samples >= BUFFER_CAP` | true |
| Overflow ocurrió | `total_samples > BUFFER_CAP` | true |
| Muestra más antigua válida | `total_samples - (BUFFER_CAP - 1)` | 315 |

### 3.3. Verificación de coherencia

El master debe verificar estas igualdades tras cada lectura:

```
(reg[303] << 16 | reg[301]) % 120     == reg[304]      # next_slot correcto
(reg[303] << 16 | reg[301]) >= 120    == bit0 de reg[306]
(reg[303] << 16 | reg[301]) > 120     == bit1 de reg[306]
reg[307] == reg[219]                                    # last_sample correcto
```

Si alguna falla, marcar alerta en el log.

---

## 4. Protocolo recomendado para el master

### 4.1. Arranque inicial del master (una sola vez)

```
1. Read 300, qty=8        ?  leer todo el bloque de configuración
2. Read 100, qty=120      ?  leer el búfer completo
3. Guardar total_anterior = (reg[303] << 16) | reg[301]
4. Guardar interval_s = reg[300]
5. Asignar timestamps retrocediendo desde el RTC actual
```

### 4.2. Poll periódico (cada 30 minutos con interval=60 s)

```
1. Read 301, qty=3        ?  (TOTAL_LOW, _, TOTAL_HIGH)
2. total_actual = (HIGH << 16) | LOW
3. nuevas = total_actual - total_anterior

   Si nuevas == 0:
       ? nada nuevo, salir

   Si 1 <= nuevas <= 120:
       ? Read del búfer: los `nuevas` registros más recientes
       ? Asignar timestamps con RTC retrocediendo `interval_s` por muestra
       ? Guardar en BD

   Si nuevas > 120:
       ? ALERTA: overflow, se perdieron (nuevas - 120) muestras
       ? Read 100, qty=120   (búfer completo)
       ? Marcar hueco en la gráfica
       ? Guardar en BD con flag de pérdida

4. total_anterior = total_actual
```

### 4.3. Frecuencia de poll recomendada

Regla: el poll debe ser **al menos 2× más frecuente** que el tiempo que
tarda el búfer en llenarse.

| Intervalo muestreo | Búfer se llena en | Poll recomendado | Poll máximo |
|---|---|---|---|
| 60 s | 2 h | cada 30 min | cada 90 min |
| 30 s | 1 h | cada 15 min | cada 45 min |
| 5 min | 10 h | cada 2 h | cada 5 h |
| 15 min | 30 h | cada 6 h | cada 15 h |

---

## 5. Ejemplos de tramas Modbus RTU

Todas las tramas usan Slave ID `0x01`. El CRC se calcula sobre todos los
bytes previos y se envía **little-endian** (byte bajo primero).

### 5.1. Leer búfer completo (100..219)

**Petición (master ? slave):**
```
01 03 00 64 00 78 <CRC>
¦  ¦  +----+ +----+
¦  ¦     ¦      +-- 120 registros
¦  ¦     +-- dirección 100
¦  +-- función 0x03
+-- slave ID
```

**Respuesta (slave ? master):**
```
01 03 F0 <240 bytes de datos> <CRC>
¦  ¦  +-- 240 bytes = 120 registros × 2
¦  +-- función
+-- slave ID
```

### 5.2. Leer bloque de configuración (300..307)

**Petición:**
```
01 03 01 2C 00 08 <CRC>
         ¦     +-- 8 registros
         +-- dirección 300
```

**Respuesta:**
```
01 03 10 <16 bytes de datos> <CRC>
```

Ejemplo de interpretación:

| Offset | Reg | Valor | Significado |
|---|---|---|---|
| 0–1 | 300 | `00 3C` | Intervalo = 60 s |
| 2–3 | 301 | `01 B2` | Total_LOW = 434 |
| 4–5 | 302 | `00 00` | Trigger dummy = 0 |
| 6–7 | 303 | `00 00` | Total_HIGH = 0 |
| 8–9 | 304 | `00 4A` | Next_slot = 74 |
| 10–11 | 305 | `00 78` | Buffer_cap = 120 |
| 12–13 | 306 | `00 03` | Status = lleno + overflow |
| 14–15 | 307 | `02 00` | Last_sample = 512 |

### 5.3. Cambiar intervalo a 30 s

**Petición:**
```
01 06 01 2C 00 1E <CRC>
         ¦     +-- valor 30
         +-- dirección 300
```

**Respuesta:** eco de la petición (mismos bytes).

### 5.4. Forzar muestra manual

**Petición:**
```
01 06 01 2E 00 01 <CRC>
         ¦     +-- escribir 1
         +-- dirección 302
```

---

## 6. Códigos de excepción Modbus

| Código | Nombre | Cuándo se devuelve |
|---|---|---|
| `0x02` | `ILLEGAL_DATA_ADDRESS` | Dirección fuera de cualquier bloque válido |
| `0x03` | `ILLEGAL_DATA_VALUE` | Valor fuera de rango (ej. intervalo = 0) |
| `0x04` | `SERVER_DEVICE_FAILURE` | `panel_ref` nulo o fallo interno |

---

## 7. Estados de error y recuperación

### 7.1. Muestra corrupta (`0xFFFF`)

Si `LOG[i] == 0xFFFF`, la muestra tenía CRC inválido en EEPROM.
Acciones:
- Descartar la muestra.
- Registrar alerta si ocurre en > 5 % de las muestras.
- Si es persistente, formatear EEPROM con un pin de reset (futuro).

### 7.2. Overflow del búfer

Cuando `nuevas > 120` en un poll:
- Registrar evento con `timestamp_master`, `total_actual`, `total_anterior`.
- Marcar hueco en la gráfica entre `total_anterior + 120` y `total_actual`.
- Considerar aumentar la frecuencia de poll.

### 7.3. Timeout de comunicación

Si no llega respuesta en 1 s:
- Reintentar 2 veces.
- Tras 3 fallos, marcar el slave como no disponible.
- Continuar con el próximo poll programado.

---

## 8. Persistencia en EEPROM (referencia interna)

Distribución del ATmega168A (512 bytes):

| Rango | Tamaño | Contenido |
|---|---|---|
| `0x000` | 1 B | Magic (`0xB8`) |
| `0x001–0x003` | 3 B | Reservado |
| `0x004–0x167` | 360 B | Datos (120 slots × 3 B) |
| `0x168–0x1DF` | 120 B | Estado (wear leveling) |
| `0x1E0–0x1E1` | 2 B | Contador `passes` |
| `0x1E2–0x1E3` | 2 B | Intervalo persistido |
| `0x1E4–0x1FF` | 28 B | Libre |

Cada slot de datos: `[low, high, crc8]`.
Vida útil teórica: > 200 años con muestreo cada 60 s.

---

## 9. Compatibilidad y versionado

| Versión firmware | Cambios |
|---|---|
| v1.0 | Búfer circular EEPROM básico, registros 100–219, 300, 302 |
| v1.1 | Añadidos 301, 303, 304, 305, 306, 307. Intervalo persistido. DebugSerial eliminado |

**Regla de compatibilidad:** los registros existentes **nunca cambian de dirección**
en versiones menores. Solo se añaden nuevos en rangos libres (ej. 308+, 220–299).

---

## 10. Recursos

- **nanoMODBUS:** https://github.com/debevv/nanoMODBUS
- **Especificación Modbus:** MODBUS Application Protocol V1.1b3 (modbus.org)
- **QModbus:** cliente de pruebas multiplataforma
- **AVR101:** algoritmo de wear leveling para EEPROM (Microchip)

---

*Fin del documento.*
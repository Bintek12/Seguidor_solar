
# Seguidor_solar
Proyecto de gestión y supervision de energía Renovable con paneles fotoVoltaicos y seguimiento solar para máximo aprovechamiento de la producción de energía limpia.
Utiliza sensores LDR para determinar la máxima intensidad solar y posicionarse
Controlado por micro Atmega168A como sistema embebido en placa de desarrollo suministrado por Ledoelectronics.com
y compañía Española EQUILAB.
el sistema actúa como esclavo con protocolo MODBUS RS485 para comunicarse con sistemas de control SCADA o similar

# Panel Solar — Seguidor Solar de 1 Eje con Modbus RTU

Firmware para un seguidor solar autónomo de un solo eje, basado en
**ATmega168A**, con comunicación **Modbus RTU sobre RS485** y registro
histórico de muestras solares en EEPROM interna.

El panel orienta automáticamente su superficie hacia el sol durante el
día, basándose en la lectura de dos sensores LDR (este/oeste), y almacena
un histórico de lecturas que un maestro externo puede descargar vía
Modbus para análisis de eficiencia y generación.

---

## ✨ Características

- **Seguimiento solar automático** con control PID y motor DC
- **2 sensores LDR** (este/oeste) con filtrado digital
- **Modbus RTU esclavo** sobre RS485 half-duplex
- **Búfer circular de 120 muestras** en EEPROM interna
- **Wear leveling** en EEPROM → vida útil > 200 años
- **CRC-8 por muestra** para detección de corrupción
- **Intervalo de muestreo configurable** por el maestro, persistido
- **Contador total de muestras** de 32 bits (no se reinicia al apagar)
- **Detección de overflow** del búfer
- **Memoria RAM mínima**: ~530 bytes de 1024 (51 %)
- **Memoria Flash**: ~13 KB de 16 KB (80 %)

---

## 🔧 Hardware

| Componente | Modelo / especificación |
|---|---|
| Microcontrolador | ATmega168A @ 16 MHz |
| Transceptor RS485 | MAX485 o similar (half-duplex) |
| Sensores solares | 2 × LDR con divisor resistivo |
| Actuador | Motor DC con puente H o relé de 2 canales |
| Programador | AVRISP mkII, Atmel-ICE o USBtinyISP |
| Alimentación | 5 V regulados |

### Conexiones principales

| Pin ATmega168A | Uso |
|---|---|
| `PD0` / `PD1` | UART0 RX / TX hacia transceptor RS485 |
| `PD2` | Pin DE/RE del MAX485 (dirección del bus) |
| `PC0`–`PC1` | Entradas analógicas de los LDR (ADC) |
| `PB1`, `PB2` | Salidas PWM del motor |
| `PC2`, `PC3` | Control de sentido del motor (puente H) |

---

## 🚀 Compilación y programación

### Requisitos

- **Microchip Studio 7.0** o superior
- **AVR-GCC** 5.4.0 o superior (incluido en Microchip Studio)
- **AVRDUDE** o el programador integrado de Microchip Studio

### Compilar

1. Abre `Panel_Solar.cppproj` en Microchip Studio.
2. Selecciona la configuración **Debug AVR** (o **Release** para producción).
3. **Build → Rebuild Solution** (o `F7`).
4. Verifica que no haya errores ni warnings.

### Programar

1. Conecta el programador al ATmega168A.
2. **Tools → Device Programming**.
3. Selecciona la herramienta, el dispositivo (`ATmega168A`), la interfaz (ISP).
4. Pestaña **Memories** → **Flash** → **Program**.

### Consumo de recursos (v1.1)

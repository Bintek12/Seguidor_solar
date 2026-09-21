
/*
/#include "MODBUS_reg.h"
#include <util/crc16.h>
#include "eeprom_log.h"
#include <avr/interrupt.h>
#include <avr/eeprom.h>
*/

#include "eeprom_log.h"
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <util/crc16.h>

/* -----------------------------------------------------------------
   Devuelve el índice del siguiente slot donde se debe escribir.
   Usa min/max sobre el búfer de estado para ser robusto frente
   al desbordamiento del contador de 8 bits tras 256 ciclos.
   ----------------------------------------------------------------- */
uint8_t eeprom_log_next_slot(void) {
    uint8_t min_val = 0xFF, min_idx = 0;
    uint8_t max_val = 0x00, max_idx = 0;

    for (uint8_t i = 0; i < NUM_SLOTS; i++) {
        uint8_t v = eeprom_read_byte((uint8_t*)(STATUS_BUFFER_START + i));
        if (v <= min_val) { min_val = v; min_idx = i; }   /* <= para quedarnos con el mayor índice */
        if (v >= max_val) { max_val = v; max_idx = i; }   /* >= para quedarnos con el mayor índice */
    }

    if (min_val == max_val) return 0;                     /* búfer virgen o recién dado una vuelta */

    if ((uint8_t)(max_val - min_val) < 128)
        return (uint8_t)((max_idx + 1) % NUM_SLOTS);      /* sin overflow */
    else
        return (uint8_t)((min_idx + 1) % NUM_SLOTS);      /* con overflow de byte */
}

/* -----------------------------------------------------------------
   Inicialización: estado a 0 en TODOS los slots (no 0..119).
   ----------------------------------------------------------------- */
void eeprom_log_init(void) {
    if (eeprom_read_byte((uint8_t*)0) == EEPROM_MAGIC) return;

    for (uint8_t i = 0; i < NUM_SLOTS; i++)
        eeprom_write_byte((uint8_t*)(STATUS_BUFFER_START + i), 0);

    for (uint16_t i = DATA_BUFFER_START; i < STATUS_BUFFER_START; i++)
        eeprom_write_byte((uint8_t*)i, 0);

    eeprom_write_byte((uint8_t*)0, EEPROM_MAGIC);
}

/* -----------------------------------------------------------------
   Escritura de una muestra nueva.
   ----------------------------------------------------------------- */
void eeprom_log_write(uint16_t ldr_value) {
    uint8_t next = eeprom_log_next_slot();
    uint16_t addr = DATA_BUFFER_START + (uint16_t)next * RECORD_SIZE;

    uint8_t low  = (uint8_t)(ldr_value & 0xFF);
    uint8_t high = (uint8_t)((ldr_value >> 8) & 0xFF);
    uint8_t crc  = 0;
    crc = _crc8_ccitt_update(crc, low);
    crc = _crc8_ccitt_update(crc, high);

    uint8_t old_sreg = SREG; cli();
    eeprom_write_byte((uint8_t*)addr,        low);
    eeprom_write_byte((uint8_t*)(addr + 1),  high);
    eeprom_write_byte((uint8_t*)(addr + 2),  crc);
    SREG = old_sreg;

    /* Incrementar el estado del slot recién escrito */
    uint16_t s_addr = STATUS_BUFFER_START + (uint16_t)next;
    uint8_t  s_val  = eeprom_read_byte((uint8_t*)s_addr);
    eeprom_write_byte((uint8_t*)s_addr, (uint8_t)(s_val + 1));
}

/* -----------------------------------------------------------------
   Lectura ordenada: slot 0 = más antiguo, slot 119 = más reciente.
   ----------------------------------------------------------------- */
uint16_t eeprom_log_read(uint8_t slot) {
    if (slot >= NUM_SLOTS) return 0xFFFF;

    uint8_t next = eeprom_log_next_slot();          /* siguiente a escribir = más antiguo actual */
    uint8_t real = (uint8_t)((next + slot) % NUM_SLOTS);

    uint16_t addr = DATA_BUFFER_START + (uint16_t)real * RECORD_SIZE;
    uint8_t low   = eeprom_read_byte((uint8_t*)addr);
    uint8_t high  = eeprom_read_byte((uint8_t*)(addr + 1));
    uint8_t crc_s = eeprom_read_byte((uint8_t*)(addr + 2));

    uint8_t crc_c = 0;
    crc_c = _crc8_ccitt_update(crc_c, low);
    crc_c = _crc8_ccitt_update(crc_c, high);

    return (crc_c == crc_s) ? (uint16_t)(low | (high << 8)) : 0xFFFF;
}
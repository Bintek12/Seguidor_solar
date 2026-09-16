#ifndef EEPROM_H
#define EEPROM_H

#include <stdint.h>
#include <avr/eeprom.h>

// Variable en EEPROM
extern uint8_t EEMEM storedMode;

// Prototipos
uint8_t loadMode();
void saveMode(uint8_t mode);

#endif

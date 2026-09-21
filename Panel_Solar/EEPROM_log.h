#ifndef EEPROM_LOG_H
#define EEPROM_LOG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
	#endif

	#define NUM_SLOTS           120
	#define RECORD_SIZE         3
	#define DATA_BUFFER_START   4
	#define STATUS_BUFFER_START (DATA_BUFFER_START + NUM_SLOTS * RECORD_SIZE)
	#define EEPROM_MAGIC        0xC4

	void     eeprom_log_init(void);
	void     eeprom_log_write(uint16_t ldr_value);
	uint16_t eeprom_log_read(uint8_t slot);
	uint8_t  eeprom_log_next_slot(void);      /* ? nuevo */

	#ifdef __cplusplus
}
#endif

#endif
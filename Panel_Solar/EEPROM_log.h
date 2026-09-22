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
	#define PASSES_ADDR         (STATUS_BUFFER_START + NUM_SLOTS)   /* 484 */
	#define INTERVAL_ADDR       (PASSES_ADDR + 2)                    /* 486 */
	#define EEPROM_MAGIC        0xB8

	
	
	void     eeprom_log_init(void);
	void     eeprom_log_write(uint16_t ldr_value);
	uint16_t eeprom_log_read(uint8_t slot);
	uint8_t  eeprom_log_next_slot(void);
	

	uint16_t eeprom_log_passes_read(void);
	void     eeprom_log_passes_write(uint16_t value);
	uint32_t eeprom_log_total_samples(void);
	uint16_t eeprom_log_interval_read(void);
	void     eeprom_log_interval_write(uint16_t value);
	/* NUEVOS */
	uint16_t eeprom_log_status_flags(void);
	uint16_t eeprom_log_last_sample(void);
	

	#ifdef __cplusplus
}
#endif

#endif
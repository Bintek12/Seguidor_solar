// rtc.h
#ifndef ADC_H
#define ADC_H

#include <stdint.h>

// Inicializa el ADC del ATmega168A
void initADC();

// Lee un canal específico del ADC
uint16_t readADC(uint8_t channel);

#endif

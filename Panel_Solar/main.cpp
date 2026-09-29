/* #define NMBS_DEBUG  */  
#include "MODBUS.h"
#include "Config.h"
#include <avr/io.h>
#include <avr/wdt.h>
#include <stdint.h>
#include "nanoModBus.h"
#include "MODBUS_reg.h"
#include "MODBUS_port.h"

#define F_CPU (8000000UL)
#include <util/delay.h>
#include "Motor.h"
#include "Panel.h"
#include "DebugSerial.h"
#include "EEPROM_log.h"
#include <avr/interrupt.h>
#include "main.h"

//struct USART usart;
//struct FLAGS flags; 

// Variables globales de tiempo
volatile uint16_t sample_interval_s = 60;   //por defecto 60 s única definición, con valor inicial
uint32_t getMillis();

volatile uint32_t system_ms = 0;
 // instancia global como ya tienes
Motor motor;
Panel panel;  



// ISR que incrementa el contador
ISR(TIMER1_COMPA_vect) {
	system_ms++;
}

	
int main() {
	//Motor motor;
	panel.setOperationMode(panel.OperationMode::AUTOMATIC); // /*AUTOMATIC*/); 
	initTimerMillis();
	setupWatchdog();
	panel.init();
	//panel.initTimerMillis();
	//panel.initPID(2.5f, 0.1f, 0.5f, 255.0f, 5.0f);
	panel.initPID(1.5, 0.3, 0.05, 100.0, 2.0);
    
	//Timer1_Init();
    uint32_t lastMotor = 0;   
	eeprom_log_init();
	modbus_init(&panel, MODBUS_BAUDIOS); 
	modbus_timer_init();
	/* NUEVO: recuperar el intervalo guardado en EEPROM */
	sample_interval_s = eeprom_log_interval_read();
	if (sample_interval_s == 0 || sample_interval_s == 0xFFFF) {
		sample_interval_s = 60;   /* valor de seguridad si la EEPROM está corrupta */
	}

	/* NUEVO: evitar muestra inmediata al arrancar */
	last_sample_ms = getMillis();
	// Habilitar la interrupción de RX del USART (¡importante!)
	UCSR0B |= (1 << RXCIE0);
	sei(); 	
	
	while (1) {
		// 1. refresca el estado de los limites 
		panel.update();
		// 3. Lee los LDRs y actualiza eastFiltered / westFiltered
		panel.leerSensores();
		
		//panel.aplicarControlMotor(); 
	
		if (frame_ready) {
			 frame_ready = false;
			nmbs_server_poll(&nmbs);
		}
      
		// Verificar alarma (prioridad máxima)
		if (panel.isAlarm()) {
			motor.stop();
			//debug.println("ALARMA ACTIVA - Motor detenido");
			_delay_ms(500);
			continue;
		}
		// Verificar límite de movimiento
		switch (panel.limiteActivo()) {
			case Limite::Este:       motor.stop(); break; /* detener motor hacia el este */
			case Limite::Oeste:      motor.stop(); break; /* detener motor hacia el oeste */
			case Limite::Horizontal:  break;
			case Limite::Ninguno:     break;
		}  
		
		// 3. Cada 200ms (por ejemplo), llama a la decisión del PID
		static unsigned long lastPID = 0;
		if (getMillis() - lastPID >= 100) { // 100ms = dt=0.1
			lastPID = getMillis();
			//Direccion dir = panel.decidirDireccion();
			//panel.decidirDireccion();
			// Nota: 'dir' solo lo usas para mostrarlo o lógica extra,
			// la acción real la hace 'aplicarControlMotor'.
		}

		// 3. Actualizar motor cada 10 ms (PWM) suave
		if (getMillis() - lastMotor >= 10) { 
			lastMotor = getMillis();
			if (panel.getOperationMode()==panel.OperationMode::AUTOMATIC) {
				panel.decidirDireccion();
				panel.aplicarControlMotor();
			}
		}
		// 3.b. Muestreo autónomo en EEPROM (independiente del master)
		if ((getMillis() - last_sample_ms) >= (uint32_t)sample_interval_s * 1000UL) {
			last_sample_ms = getMillis();
			//uint16_t v = (panel.eastFiltered + panel.westFiltered) / 2;
			uint16_t v = panel.getEastFiltered();
			eeprom_log_write(v);
		}
		// Debe ejecutarse regularmente, al menos una vez cada 2 segundos (en este ejemplo).
		wdt_reset(); // <--- Punto clave para evitar el reinicio[reference:7]
	}	    // end While 
}           //  End main

// Función para obtener milisegundos (accesible desde panel.cpp)
uint32_t getMillis() {
	uint32_t val;
	cli();
	val = system_ms;
	sei();
	return val;
}

// Configurar Timer1 para interrupción cada 1 ms con prescaler 64
void initTimerMillis() {
	TCCR1B = 0;
	TCNT1 = 0;
	OCR1A = 124;   // (8e6/64/1000) - 1 = 124
	TCCR1A = 0;
	TCCR1B = (1 << WGM12) | (1 << CS10) | (1 << CS11); // CTC, prescaler 64
	TIMSK1 = (1 << OCIE1A);
	sei();
}


void setupWatchdog() {
	// 1. Limpiar el flag de reinicio por Watchdog (WDRF) en MCUSR
	// Esto es OBLIGATORIO para evitar reinicios inesperados.
	MCUSR &= ~(1 << WDRF);

	// 2. Deshabilitar el WDT durante la configuración inicial.
	// Es una buena práctica para asegurar un inicio limpio.
	wdt_disable();

	// 3. Habilitar el WDT con el tiempo de espera deseado.
	// Ejemplo: 2 segundos (WDTO_2S).
	// Otros valores comunes: WDTO_15MS, WDTO_30MS, WDTO_60MS, WDTO_120MS,
	// WDTO_250MS, WDTO_500MS, WDTO_1S, WDTO_2S, WDTO_4S, WDTO_8S[reference:4].
	wdt_enable(WDTO_2S);
}

void Timer1_Init(void) {
	TCCR1B = (1<<WGM12)|(1<<CS11); // CTC, prescaler 8
	OCR1A = 2000;                  // ~2 ms a 16 MHz
	TIMSK1 = (1<<OCIE1A);          // habilita interrupción
}

uint8_t find_last_slot() {
	uint8_t prev = eeprom_read_byte((uint8_t*)(STATUS_BUFFER_START + NUM_SLOTS - 1));
	for (uint8_t i = 0; i < NUM_SLOTS; i++) {
		uint8_t curr = eeprom_read_byte((uint8_t*)(STATUS_BUFFER_START + i));
		if ((uint8_t)(prev + 1) != curr) {
			return (i == 0) ? (NUM_SLOTS - 1) : (i - 1);
		}
		prev = curr;
	}
	return NUM_SLOTS - 1; // Búfer lleno, último slot es el 119
}

void write_ldr_sample(uint16_t ldr_value) {
	uint8_t last_slot = find_last_slot();
	uint8_t next_slot = (last_slot + 1) % NUM_SLOTS;

	uint16_t addr = DATA_BUFFER_START + next_slot * RECORD_SIZE;
	uint8_t crc = 0;

	uint8_t low  = ldr_value & 0xFF;
	uint8_t high = (ldr_value >> 8) & 0xFF;
	crc = _crc8_ccitt_update(crc, low);
	crc = _crc8_ccitt_update(crc, high);

	eeprom_write_byte((uint8_t*)addr, low);
	eeprom_write_byte((uint8_t*)(addr + 1), high);
	eeprom_write_byte((uint8_t*)(addr + 2), crc);

	// Actualizar contador de estado del slot
	uint16_t status_addr = STATUS_BUFFER_START + (uint16_t)next_slot; 
	uint8_t  status_val  = eeprom_read_byte((uint8_t*)status_addr);
	eeprom_write_byte((uint8_t*)status_addr, (uint8_t)(status_val + 1));
}
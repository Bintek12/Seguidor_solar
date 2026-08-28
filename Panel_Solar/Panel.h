#ifndef PANEL_H
#define PANEL_H

#include "config.h"
#include <stdint.h>
#include <stdbool.h>

// Enumeración de dirección (válida en C y C++)
enum Direccion {
	DIR_STOP = 0,
	DIR_ESTE = 1,
	DIR_OESTE = -1
};

class Panel {
	public:
	Panel();
	void init();
	void initTimerMillis();
	void leerSensores();
	Direccion decidirDireccion();
    void aplicarControlMotor(); 
	// Getters
	float getEastFiltered() const;
	float getWestFiltered() const;
	float getError() const;

	// PID y umbral
	void initPID(float kp, float ki, float kd, float maxOut, float stopThr);
	float getPIDOutput() const { return pidOutput; }
	float getCurrentError() const { return currentError; }
	
	void setPIDGains(float kp, float ki, float kd);
	void setStopThreshold(float threshold);
	float getStopThreshold() const;

	// Temperatura (NTC en ADC6)
	float readTemperature();

	// Alarmas y límites
	bool isAlarm() const;
	bool isLimit() const;
	const char* getStatusMessage() const;

	private:
	// Valores filtrados
	float eastFiltered;
	float westFiltered;
	float error;
    float stopThreshold;
	// Variables del PID
	float Kp, Ki, Kd;

	float integral;
	float prevError;
	float maxOutput;
	float pidOutput;      // Salida con signo (+ = ESTE, - = OESTE)
	float currentError;

	// Control de dirección (para proteger relé PB6)
	int lastDirectionSign; // 1=ESTE, -1=OESTE, 0=STOP

	// Constantes de tiempo
	static const uint32_t PWM_PERIOD_MS = 200; // Período de 200ms para PWM


	// ---------- Filtros con buffers estáticos ----------
	static const uint8_t MA_WINDOW = 10;
	static const uint8_t MED_WINDOW = 5;

	// Media móvil (Este y Oeste)
	float maBufferEast[MA_WINDOW];
	float maBufferWest[MA_WINDOW];
	uint8_t maIndexEast, maIndexWest;
	float maSumEast, maSumWest;
	bool maFilledEast, maFilledWest;

	// Filtro mediano (Este y Oeste)
	uint16_t medBufferEast[MED_WINDOW];
	uint16_t medBufferWest[MED_WINDOW];
	uint8_t medIndexEast, medIndexWest;
	bool medFilledEast, medFilledWest;

	// Funciones auxiliares de filtrado
	float movingAverage(float newValue, float* buffer, uint8_t& index, float& sum, bool& filled);
	uint16_t medianFilter(uint16_t newValue, uint16_t* buffer, uint8_t& index, bool& filled);

	// ADC y temperatura
	uint16_t leerADC(uint8_t canal);
	float calcularTemperatura(uint16_t adcValue);

	// Constantes NTC
	static const float SERIES_RESISTOR;
	static const float NTC_BETA;
	static const float NTC_R25;
};

#endif
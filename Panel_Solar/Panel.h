#ifndef PANEL_H
#define PANEL_H

#include "config.h"
#include <stdint.h>
#include <stdbool.h>




// Enumeración de dirección (válida en C y C++)
enum class Direccion : int8_t {
	Stop  =  0,
	Este  =  1,
	Oeste = -1
};
enum class Limite {
	Ninguno,
	Este,
	Oeste,
	Horizontal
};

class Panel {
	public:
	Panel();
	void update();
	// Consultas rápidas sobre el estado ya leído
	bool limiteEste()       const { return _este; }
	bool limiteOeste()      const { return _oeste; }
	bool limiteHorizontal() const { return _horizontal; }
	bool algunLimite()      const { return _este || _oeste || _horizontal; }

	// Devuelve el límite "activo" con la prioridad que definas
	Limite limiteActivo() const;
	
	void init();
	//void initTimerMillis();
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
	float readTemperature(int ch);

	// Alarmas y límites
	bool isAlarm() const;
	//bool isLimit() const;
	const char* getStatusMessage() const;

	private:
		
	//limites
	uint8_t _este        : 1;
	uint8_t _oeste       : 1;
	uint8_t _horizontal  : 1;
	uint8_t maFilledEast : 1;
	uint8_t maFilledWest : 1;
	uint8_t medFilledEast: 1;
	uint8_t medFilledWest: 1;
	
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
	Direccion lastDirection = Direccion::Stop;
	// Constantes de tiempo
	static const uint32_t PWM_PERIOD_MS = 200; // Período de 200ms para PWM


	// ---------- Filtros con buffers estáticos ----------
	static const uint8_t MA_WINDOW = 10;
	static const uint8_t MED_WINDOW = 5;

	// Media móvil (Este y Oeste)
	uint16_t  maBufferEast[MA_WINDOW];
	uint16_t  maBufferWest[MA_WINDOW];
	uint8_t maIndexEast, maIndexWest;
	uint16_t  maSumEast, maSumWest;

	// Filtro mediano (Este y Oeste)
	uint16_t medBufferEast[MED_WINDOW];
	uint16_t medBufferWest[MED_WINDOW];
	uint8_t medIndexEast, medIndexWest;

	// Funciones auxiliares de filtrado
	//uint16_t movingAverage(uint16_t newValue, uint16_t* buffer,	uint8_t& index, uint16_t& sum, bool& filled);
	float movingAverageEast(uint16_t newValue);
	float movingAverageWest(uint16_t newValue);
	//uint16_t medianFilter(uint16_t newValue, uint16_t* buffer, uint8_t& index, bool& filled);
    uint16_t medianFilterEast(uint16_t newValue);
    uint16_t medianFilterWest(uint16_t newValue);
	// ADC y temperatura
	uint16_t leerADC(uint8_t canal);
	float calcularTemperatura(uint16_t adcValue);

	// Constantes NTC
	static const float SERIES_RESISTOR;
	static const float NTC_BETA;
	static const float NTC_R25;
};

#endif
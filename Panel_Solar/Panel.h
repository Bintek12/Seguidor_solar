#ifndef PANEL_H
#define PANEL_H

#include "config.h"
#include <stdint.h>
#include <stdbool.h>


uint32_t getMillis(); 

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
	enum class OperationMode : uint8_t {
		MANUAL     = 0,
		AUTOMATIC  = 1
	};
	// Modo de operación
	void setOperationMode(OperationMode mode);   // NO const
	OperationMode getOperationMode() const;
	bool isAutomatic() const { return mode_ == OperationMode::AUTOMATIC; }
	bool isManual()    const { return mode_ == OperationMode::MANUAL; }

    uint16_t getOperationModeRegister() const {
	    return static_cast<uint16_t>(mode_);
    }
	void update();
	// Consultas rápidas sobre el estado ya leído
	bool limiteEste()       const { return _este; }
	bool limiteOeste()      const { return _oeste; }
	bool limiteHorizontal() const { return _horizontal; }
	bool algunLimite()      const { return _este || _oeste || _horizontal; }

	// Devuelve el límite "activo" con la prioridad que definas
	Limite limiteActivo() const;
	
	void init();
	void leerSensores();
	void actualizarMotor();  
	
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
	int8_t readTemperature(int ch);   // -40..125 °C, 1 °C de resolución
   
	// Alarmas y límites
	bool isAlarm() const;
	//bool isLimit() const;
	const char* getStatusMessage() const;
    float Kp, Ki, Kd;
	private:
		
	//limites
	uint8_t _mode        : 1;
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
    float stopThreshold = 5.0;
	//const float stopThreshold;
	// Variables del PID
	//float Kp, Ki, Kd;
    float    k_pwm   = 0.0f;   // = PWM_PERIOD_MS / maxOutput  (precalculado en initPID)
    uint32_t onTimeMin = 30;   // = 15% de 200 ms
	float integral;
	float prevError;
	float maxOutput;
	float pidOutput;      // Salida con signo (+ = ESTE, - = OESTE)
	float currentError;

	// Control de dirección (para proteger relé PB6)
	Direccion lastDirection = Direccion::Stop;
	// Constantes de tiempo
	static const uint32_t PWM_PERIOD_MS = 200; // Período de 200ms para PWM
	//const unsigned long PWM_PERIOD_MS = 200;
    OperationMode mode_ = OperationMode::MANUAL;  // valor por defecto

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

	float movingAverageEast(uint16_t newValue);
	float movingAverageWest(uint16_t newValue);

    uint16_t medianFilterEast(uint16_t newValue);
    uint16_t medianFilterWest(uint16_t newValue);
	// ADC y temperatura
	uint16_t leerADC(uint8_t canal);
};

#endif
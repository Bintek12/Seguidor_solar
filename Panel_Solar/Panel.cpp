
// panel.cpp
#include "Panel.h"
#include <avr/io.h>
//#include <math.h>        // Para fabsf
//#include <stdint.h>
//#include <stdbool.h>
#include "ntc_table.h"
#define F_CPU (8000000UL)
#include <util/delay.h>
#include <stdlib.h>   // para abs() en enteros
inline int8_t signoDe(Direccion d) {
	return static_cast<int8_t>(d);
}

Panel::Panel()
: Kp(2.0f), Ki(0.5f), Kd(0.1f),_mode(false), _este(false), _oeste(false), _horizontal(false), maFilledEast(false), maFilledWest(false),
medFilledEast(false), medFilledWest(false),eastFiltered(0), westFiltered(0), error(0), stopThreshold(5.0f),  
integral(0), prevError(0), maxOutput(100.0f),
maIndexEast(0), maIndexWest(0), maSumEast(0), maSumWest(0),medIndexEast(0), medIndexWest(0)


{
	init();
}

void Panel::init() {
	// Configurar ADC
	ADMUX = (1 << REFS0);
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1); // prescaler 64

	// Configurar pines de alarma y límite como entradas con pull-up
	DDRB &= ~(1 << ALARMA_PIN) ;
	PORTB |= (1 << ALARMA_PIN) ;
	/*
	DDRC &= ~((1 << LIMITE_PIN_E)| (1 << LIMITE_PIN_H)| (1 << LIMITE_PIN_W));
	PORTC |= (1 << LIMITE_PIN_E)| (1 << LIMITE_PIN_H)| (1 << LIMITE_PIN_W);
	*/
	// Configurar como entradas
	DDRD &= ~((1 << LIMITE_PIN_E) | (1 << LIMITE_PIN_W));
	// Activar resistencias pull-up internas
	PORTD |= (1 << LIMITE_PIN_E) | (1 << LIMITE_PIN_W);
	// Habilitar interrupciones por cambio de estado en PD6 y PD7
	PCMSK2 |= (1 << PCINT22) | (1 << PCINT23); // Máscara de interrupción
	PCICR  |= (1 << PCIE2);                    // Habilitar grupo de interrupciones PCINT[23:16]


	// Inicializar buffers de filtros
	for (uint8_t i = 0; i < MA_WINDOW; i++) {
		maBufferEast[i] = 0;
		maBufferWest[i] = 0;
	}
	for (uint8_t i = 0; i < MED_WINDOW; i++) {
		medBufferEast[i] = 0;
		medBufferWest[i] = 0;
	}
	maIndexEast = maIndexWest = 0;
	maSumEast = maSumWest = 0;
	maFilledEast = maFilledWest = false;
	medIndexEast = medIndexWest = 0;
	medFilledEast = medFilledWest = false;

	// Reset PID
	integral = 0;
	prevError = 0;
}

uint16_t Panel::leerADC(uint8_t canal) {
	ADMUX = (1 << REFS0) | (canal & 0x0F);
	//ADMUX = (ADMUX & 0xF0) | (canal & 0x0F);
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC)) {}
	return ADC;
}

// ---------- Filtro de media móvil ----------
float Panel::movingAverageEast(uint16_t newValue) {
	maSumEast -= maBufferEast[maIndexEast];
	maBufferEast[maIndexEast] = newValue;
	maSumEast += newValue;
	maIndexEast = (maIndexEast + 1) % MA_WINDOW;
	if (!maFilledEast && maIndexEast == 0) maFilledEast = true;
	
	uint8_t n = maFilledEast ? MA_WINDOW : (maIndexEast == 0 ? MA_WINDOW : maIndexEast);
	return (float)maSumEast / n;
}
float Panel::movingAverageWest(uint16_t newValue) {
	maSumWest -= maBufferWest[maIndexWest];
	maBufferWest[maIndexWest] = newValue;
	maSumWest += newValue;
	maIndexWest = (maIndexWest + 1) % MA_WINDOW;
	if (!maFilledWest && maIndexWest == 0) maFilledWest = true;
	
	uint8_t n = maFilledWest ? MA_WINDOW : (maIndexWest == 0 ? MA_WINDOW : maIndexWest);
	return (float)maSumWest / n;
}
uint16_t Panel::medianFilterEast(uint16_t newValue) {
	medBufferEast[medIndexEast] = newValue;
	medIndexEast = (medIndexEast + 1) % MED_WINDOW;
	if (!medFilledEast && medIndexEast == 0) medFilledEast = true;
	if (!medFilledEast) return newValue;   // aún no hay suficientes muestras
	
	// Copia para ordenar y calcular la mediana
	uint16_t temp[MED_WINDOW];
	for (uint8_t i = 0; i < MED_WINDOW; i++) temp[i] = medBufferEast[i];
	// Copiar y ordenar
	// Ordenamiento simple (burbuja)
	for (uint8_t i = 0; i < MED_WINDOW - 1; i++) {
		for (uint8_t j = 0; j < MED_WINDOW - i - 1; j++) {
			if (temp[j] > temp[j+1]) {
				uint16_t t = temp[j];
				temp[j] = temp[j+1];
				temp[j+1] = t;
			}
		}
	}
	return temp[MED_WINDOW / 2];
}
uint16_t Panel::medianFilterWest(uint16_t newValue) {
	medBufferWest[medIndexWest] = newValue;
	medIndexWest = (medIndexWest + 1) % MED_WINDOW;
	if (!medFilledWest && medIndexWest == 0) medFilledWest = true;
	if (!medFilledWest) return newValue;   // aún no hay suficientes muestras
	
	// Copia para ordenar y calcular la mediana
	uint16_t temp[MED_WINDOW];
	for (uint8_t i = 0; i < MED_WINDOW; i++) temp[i] = medBufferWest[i];
	// Copiar y ordenar
	// Ordenamiento simple (burbuja)
	for (uint8_t i = 0; i < MED_WINDOW - 1; i++) {
		for (uint8_t j = 0; j < MED_WINDOW - i - 1; j++) {
			if (temp[j] > temp[j+1]) {
				uint16_t t = temp[j];
				temp[j] = temp[j+1];
				temp[j+1] = t;
			}
		}
	}
	return temp[MED_WINDOW / 2];
}
void Panel::leerSensores() {
	uint16_t rawEast = leerADC(LDR_ESTE);
	uint16_t rawWest = leerADC(LDR_OESTE);

	// Filtro mediano
	uint16_t medEast = medianFilterEast(rawEast);
	uint16_t medWest = medianFilterWest(rawWest);

	// Filtro media móvil
	eastFiltered = movingAverageEast(medEast);
	westFiltered = movingAverageWest(medWest);
	error = eastFiltered - westFiltered;;
}

//float Panel::getEastFiltered() const { return eastFiltered;}
//float Panel::getWestFiltered() const { return westFiltered; }
float Panel::getError() const { return error; }

void Panel::setPIDGains(float kp, float ki, float kd) {
	Kp = kp; Ki = ki; Kd = kd;
}

void Panel::setStopThreshold(float threshold) {
	stopThreshold = threshold;
}

float Panel::getStopThreshold() const { return stopThreshold; }

// ==============================================
// VARIABLES DE CONTROL PID Y PWM
// ==============================================
// Constantes del PID (AJUSTA ESTOS VALORES SEGÚN TU PRUEBA)
// Parámetros PID en enteros (escalados)
int16_t Kp = 20;   // Ganancia proporcional
int16_t Ki = 5;    // Ganancia integral
int16_t Kd = 10;   // Ganancia derivativa
int16_t stopThreshold = 3;   // Umbral de error muerto
int16_t maxOutput = 100;     // Saturación máxima
int16_t k_pwm = 2;          // Escalado PWM
const uint16_t PWM_PERIOD_MS = 50; // Periodo PWM
const uint16_t onTimeMin = 8;      // Duty mínimo (~15 %)

/*
const float Kp = 1.5;
const float Ki = 0.3;
const float Kd = 0.05;
*/
uint16_t  maSumEast=0, maSumWest=0;
// Límites de salida (de 0 a 100, representa el % de ancho de pulso)
//const float maxOutput = 100.0;
//const float minOutput = 0.0;

// Variables internas del PID
//float integral = 0;
//float prevError = 0;
//float pidOutput = 0;      // <--- SALIDA DEL PID (VALOR CON SIGNO).
// Positivo = Gira ESTE, Negativo = Gira OESTE.
//float currentError = 0;   // Para depuración

// Umbral de parada (banda muerta para evitar ruido)
//const float stopThreshold = 5.0; // Ajusta según tus LDRs
//float stopThreshold = 5.0; // Ajusta según tus LDRs

// Control de dirección (para no quemar el relé PB6)
//int lastDirectionSign = 0; // 1 = ESTE, -1 = OESTE, 0 = STOP
//int8_t lastDirectionSign = 0; // 1 = ESTE, -1 = OESTE, 0 = STOP

// Variables para el PWM por software en PB7 (Período de 200ms = 5Hz, ideal para relé sólido)
//const unsigned long PWM_PERIOD_MS = 200;
unsigned long pwmTimerStart = 0;


// Variables internas
int16_t currentError = 0;
int16_t prevError = 0;
int32_t integral = 0;
int16_t pidOutput = 0;
//static Direccion lastDirection = Direccion::Stop;


// Inicialización de parámetros PID (llamar desde el constructor o setup)
void Panel::initPID(float kp, float ki, float kd, float maxOut, float stopThr)
{
	Kp = kp;  Ki = ki;  Kd = kd;
	maxOutput     = maxOut;
	stopThreshold = stopThr;
	integral      = 0.0f;
	prevError     = 0.0f;
	pidOutput     = 0.0f;
	currentError  = 0.0f;

	// Precalculados para el PWM
	k_pwm     = (float)PWM_PERIOD_MS / maxOutput;
	onTimeMin = (PWM_PERIOD_MS * 15u) / 100u;   // 15 %
}
/*
// Constantes escaladas x64
#define KP_Q     (int16_t)(Kp * 64)
#define KI_Q     (int16_t)(Ki * 64)
#define KD_Q     (int16_t)(Kd * 64)
#define MAXOUT_Q (int16_t)(maxOutput * 64)


void Panel::actualizarMotor(){
	const uint32_t now = getMillis();
	static uint32_t lastPidTime = 0;
	// ================= PID cada 100 ms =================
	if ((uint32_t)(now - lastPidTime) >= 100) {
		lastPidTime = now;
		currentError = eastFiltered - westFiltered;
		// Banda muerta
		if (fabsf(currentError) < stopThreshold) {
			integral  = 0.0f;
			prevError = 0.0f;
			pidOutput = 0.0f;
			} else {
			const float dt = 0.1f;
			float output = Kp * currentError
			+ Ki * integral * dt
			+ Kd * (currentError - prevError) / dt;

			// Anti-windup
			if (fabsf(output) < maxOutput)
			integral += currentError * dt;

			prevError = currentError;

			// Saturación
			if      (output >  maxOutput) output =  maxOutput;
			else if (output < -maxOutput) output = -maxOutput;

			pidOutput = output;
		}
		// Dirección (relé PB6): sólo conmuta si cambia y salimos de ±2
		if (pidOutput > 2.0f) {
			if (lastDirection != Direccion::Este) {
				PORTB &= ~(1 << PB6);
				lastDirection = Direccion::Este;
			}
			} else if (pidOutput < -2.0f) {
			if (lastDirection != Direccion::Oeste) {
				PORTB |=  (1 << PB6);
				lastDirection = Direccion::Oeste;
			}
		}
	}
	// ================= PWM cada llamada =================
	if (pidOutput == 0.0f || fabsf(currentError) < stopThreshold) {
		PORTB &= ~(1 << PB7);
		return;
	}
	// duty = |pidOutput| / maxOutput  -> evitamos la división usando k_pwm
	uint32_t onTime = (uint32_t)(fabsf(pidOutput) * k_pwm);

	if      (onTime < onTimeMin)    onTime = onTimeMin;    // mínimo 15 %
	else if (onTime > PWM_PERIOD_MS) onTime = PWM_PERIOD_MS;

	if ((now % PWM_PERIOD_MS) < onTime)
	PORTB |=  (1 << PB7);
	else
	PORTB &= ~(1 << PB7);
}
*/

void Panel::actualizarMotor() {
	const uint32_t now = getMillis();
	static uint32_t lastPidTime = 0;

	// ================= PID cada 100 ms =================
	if ((uint32_t)(now - lastPidTime) >= 100) {
		lastPidTime = now;

		// Error = diferencia Este - Oeste (ya filtrados en enteros)
		currentError = eastFiltered - westFiltered;

		// Banda muerta
		if (abs(currentError) < stopThreshold) {
			integral  = 0;
			prevError = 0;
			pidOutput = 0;
			} else {
			const int16_t dt = 1; // equivalente a 0.1s escalado
			int32_t output = (int32_t)Kp * currentError
			+ (int32_t)Ki * integral * dt
			+ (int32_t)Kd * (currentError - prevError) / dt;

			// Anti-windup
			if (abs(output) < maxOutput)
			integral += currentError * dt;

			prevError = currentError;

			// Saturación
			if      (output >  maxOutput) output =  maxOutput;
			else if (output < -maxOutput) output = -maxOutput;

			pidOutput = (int16_t)output;
		}

		// Dirección (relé PB6): sólo conmuta si cambia y salimos de ±2
		if (pidOutput > 2) {
			if (lastDirection != Direccion::Este) {
				MOTOR_PORT &= ~(1 << MOTOR_DIR); // Este
				lastDirection = Direccion::Este;
			}
			} else if (pidOutput < -2) {
			if (lastDirection != Direccion::Oeste) {
				MOTOR_PORT |=  (1 << MOTOR_DIR); // Oeste
				lastDirection = Direccion::Oeste;
			}
		}
	}

	// ================= PWM cada llamada =================
	if (pidOutput == 0 || abs(currentError) < stopThreshold) {
		MOTOR_PORT &= ~(1 << MOTOR_POWER); // Apagar motor
		return;
	}

	//uint16_t onTime = (uint16_t)(std::abs((int16_t)pidOutput)) * k_pwm;
	uint16_t onTime = (uint16_t)( (uint16_t)abs(pidOutput) * k_pwm );

	if (onTime < onTimeMin) {
		onTime = onTimeMin;     // mínimo duty
		} else if (onTime > (uint16_t)PWM_PERIOD_MS) {
		onTime = PWM_PERIOD_MS; // máximo duty
	}


	if ((now % PWM_PERIOD_MS) < onTime)
	MOTOR_PORT |=  (1 << MOTOR_POWER); // Encender
	else
	MOTOR_PORT &= ~(1 << MOTOR_POWER); // Apagar
}

int8_t Panel::readTemperature(int ch){
	uint16_t adc = leerADC(ch);
	if (adc > 1023) adc = 1023;
	return (int8_t)pgm_read_byte(&ntcTable[adc]);
}

bool Panel::isAlarm() const {
	return (PINB & (1 << ALARMA_PIN)) != 0;
}

void Panel::setOperationMode(OperationMode mode) {
	mode_ = mode;
}

Panel::OperationMode Panel::getOperationMode() const {
	return mode_;
}

void Panel::update() {
	// Lectura activa en bajo: 0 = límite alcanzado
	_este       = (PINC & (1 << LIMITE_PIN_E)) == 0;
	_horizontal = (PINC & (1 << LIMITE_PIN_H)) == 0;
	_oeste      = (PINC & (1 << LIMITE_PIN_W)) == 0; 
}

Limite Panel::limiteActivo() const {
	if (_este)       return Limite::Este;
	if (_oeste)      return Limite::Oeste;
	if (_horizontal) return Limite::Horizontal;
	return Limite::Ninguno;
}

const char* Panel::getStatusMessage() const {
	if (isAlarm()) return "ALARMA";
	//if (limiteActivo()) return "LIMITE";
	return "OK";
}

void Panel::este() {
	accion(Direccion::Este);
}

void Panel::oeste() {
	accion(Direccion::Oeste);
}

void Panel::stop() {
	accion(Direccion::Stop);
}

void Panel::accion(Direccion dir) {
	switch (dir) {
		case Direccion::Stop:
		MOTOR_PORT &= ~(1 << MOTOR_POWER); // Apagar motor
		lastDirection = Direccion::Stop;
		break;

		case Direccion::Este:
	    MOTOR_PORT &= ~(1 << MOTOR_POWER); // Reset
		_delay_ms(20);
		MOTOR_PORT &= ~(1 << MOTOR_DIR);   // Dirección Este
		MOTOR_PORT |=  (1 << MOTOR_POWER); // Encender
		lastDirection = Direccion::Este;
		break;

		case Direccion::Oeste:
		MOTOR_PORT &= ~(1 << MOTOR_POWER); // Reset
		_delay_ms(20);
		MOTOR_PORT |=  (1 << MOTOR_DIR);   // Dirección Oeste
		MOTOR_PORT |=  (1 << MOTOR_POWER); // Encender
		lastDirection = Direccion::Oeste;
		break;
	}
}
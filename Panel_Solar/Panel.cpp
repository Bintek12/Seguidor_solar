
// panel.cpp
#include "Panel.h"
#include <avr/io.h>
#include <math.h>        // Para fabsf
#include "ntc_table.h"


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
	
	DDRC &= ~((1 << LIMITE_PIN_E)| (1 << LIMITE_PIN_H)| (1 << LIMITE_PIN_W));
	PORTC |= (1 << LIMITE_PIN_E)| (1 << LIMITE_PIN_H)| (1 << LIMITE_PIN_W);

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

float Panel::getEastFiltered() const { return eastFiltered;}
float Panel::getWestFiltered() const { return westFiltered; }
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
const float Kp = 1.5;
const float Ki = 0.3;
const float Kd = 0.05;

uint16_t  maSumEast=0, maSumWest=0;
// Límites de salida (de 0 a 100, representa el % de ancho de pulso)
const float maxOutput = 100.0;
const float minOutput = 0.0;

// Variables internas del PID
float integral = 0;
float prevError = 0;
float pidOutput = 0;      // <--- SALIDA DEL PID (VALOR CON SIGNO).
// Positivo = Gira ESTE, Negativo = Gira OESTE.
float currentError = 0;   // Para depuración

// Umbral de parada (banda muerta para evitar ruido)
//const float stopThreshold = 5.0; // Ajusta según tus LDRs
float stopThreshold = 5.0; // Ajusta según tus LDRs

// Control de dirección (para no quemar el relé PB6)
//int lastDirectionSign = 0; // 1 = ESTE, -1 = OESTE, 0 = STOP
int8_t lastDirectionSign = 0; // 1 = ESTE, -1 = OESTE, 0 = STOP

// Variables para el PWM por software en PB7 (Período de 200ms = 5Hz, ideal para relé sólido)
//const unsigned long PWM_PERIOD_MS = 200;
unsigned long pwmTimerStart = 0;

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

/*

void Panel::actualizarMotor() {
	extern uint32_t getMillis();
	static uint32_t lastPidTime = 0;
	static int32_t  integralQ  = 0;
	static int16_t  prevErr    = 0;
	static int16_t  pidOutQ    = 0;

	uint32_t now = getMillis();

	if ((uint32_t)(now - lastPidTime) >= 100) {
		lastPidTime = now;

		int16_t err = (int16_t)(eastFiltered - westFiltered);   // ya en unidades de sensor

		if (err > -stopThreshold && err < stopThreshold) {
			integralQ = 0;
			prevErr   = 0;
			pidOutQ   = 0;
			} else {
			// dt*10 = 1 (trabajamos en décimas para evitar fracciones)
			int32_t out = (int32_t)KP_Q * err
			+ (int32_t)KI_Q * integralQ / 10      // * dt(0.1)
			+ (int32_t)KD_Q * (err - prevErr) * 10;

			// Anti-windup
			if (out > -MAXOUT_Q && out < MAXOUT_Q)
			integralQ += err * 10;                        // acumula en décimas

			prevErr = err;

			// Saturación
			if      (out >  MAXOUT_Q) out =  MAXOUT_Q;
			else if (out < -MAXOUT_Q) out = -MAXOUT_Q;

			pidOutQ = (int16_t)out;
		}

		// Dirección
		if (pidOutQ > 128) {              // 128/64 = 2.0
			if (lastDirection != Direccion::Este) {
				PORTB &= ~(1 << PB6);
				lastDirection = Direccion::Este;
			}
			} else if (pidOutQ < -128) {
			if (lastDirection != Direccion::Oeste) {
				PORTB |= (1 << PB6);
				lastDirection = Direccion::Oeste;
			}
		}
	}

	// PWM
	if (pidOutQ == 0) {
		PORTB &= ~(1 << PB7);
		return;
	}

	int16_t dutyQ = (int16_t)(((int32_t)pidOutQ * 64) / MAXOUT_Q);  // 0..64 aprox
	if (dutyQ < 0) dutyQ = -dutyQ;
	if (dutyQ < 10) dutyQ = 10;        // mínimo 15% ~ 10/64
	if (dutyQ > 64) dutyQ = 64;

	uint32_t onTime = ((uint32_t)dutyQ * PWM_PERIOD_MS) >> 6;

	if ((now % PWM_PERIOD_MS) < onTime)
	PORTB |=  (1 << PB7);
	else
	PORTB &= ~(1 << PB7);
}
// Función que calcula el PID y retorna dirección (se llama cada 100 ms)
Direccion Panel::decidirDireccion() {
	// 1. Obtener error
	currentError = eastFiltered - westFiltered;

	// 2. Banda muerta
	if (fabsf(currentError) < stopThreshold) {
		integral = 0.0;
		prevError = 0.0;
		pidOutput = 0.0;
		return Direccion::Stop;
	}

	// 3. Cálculo PID con dt = 0.1 s (porque llamamos cada 100 ms)
	const float dt = 0.1;
	float output = Kp * currentError
	+ Ki * integral * dt
	+ Kd * (currentError - prevError) / dt;

	// 4. Anti-Windup
	if (fabsf(output) < maxOutput) {
		integral += currentError * dt;
	}
	prevError = currentError;

	// 5. Saturación
	if (output > maxOutput) output = maxOutput;
	if (output < -maxOutput) output = -maxOutput;

	pidOutput = output;
   	
	// 6. Retornar dirección
	if (output > 0) return Direccion::Este;
	else if (output < 0) return Direccion::Oeste;
	else return Direccion::Stop;
}

// NUEVA FUNCIÓN: Aplica el control a los pines (llamar cada 1 ms)
void Panel::aplicarControlMotor() {
	// A. GESTIÓN DE DIRECCIÓN (PB6) - Relé electromecánico
	Direccion currentDir = Direccion::Stop;
	if (pidOutput > 2.0)       currentDir = Direccion::Este;   // ESTE
	else if (pidOutput < -2.0) currentDir = Direccion::Oeste;  // OESTE

	// Solo cambiamos si salimos de la zona muerta y la dirección cambió
	if (currentDir != Direccion::Stop && currentDir != lastDirection) {
		if (currentDir == Direccion::Este) {
			PORTB &= ~(1 << PB6);  // ESTE  -> PB6 = 0
			} else {
			PORTB |=  (1 << PB6);  // OESTE -> PB6 = 1
		}
		lastDirection = currentDir;

		// Debug: cambio de dirección (usando tu DebugSerial)
		//debug.print("\r\n[CAMBIO DIR] -> ");
		//debug.println(currentDir == Direccion::Este ? "ESTE" : "OESTE");
	}

	// B. GESTIÓN DE VELOCIDAD (PB7) - PWM por software (Relé sólido)
	// Calcular ciclo de trabajo
	float absOut = fabsf(pidOutput);
	float duty = absOut / maxOutput;
	if (duty > 0 && duty < 0.15) duty = 0.15;  // Mínimo 15%
	if (duty > 1.0) duty = 1.0;

	uint32_t onTime = (uint32_t)(duty * PWM_PERIOD_MS); // ms de encendido

	// Obtener tiempo actual (debe estar en milisegundos desde inicio)
	extern uint32_t getMillis();  // Declarada en main.cpp
	uint32_t currentTime = getMillis() % PWM_PERIOD_MS;

	if (fabsf(currentError) < stopThreshold || pidOutput == 0) {
		PORTB &= ~(1 << PB7);  // Apagar motor
		} else {
		if (currentTime < onTime) {
			PORTB |=  (1 << PB7);  // Encender
			} else {
			PORTB &= ~(1 << PB7);  // Apagar
		}
	}
}

*/

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
/*
void Panel::setOperationMode(bool automatic) {
	mode_ = automatic ? OperationMode::AUTOMATIC : OperationMode::MANUAL;
}
*/
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
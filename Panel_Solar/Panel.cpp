
// panel.cpp
#include "Panel.h"
#include <avr/io.h>
#include <math.h>        // Para fabsf
#include "DebugSerial.h" // Asumo que tienes esta clase


// Definir constantes estáticas
const float Panel::SERIES_RESISTOR = 10000.0f;
const float Panel::NTC_BETA = 3950.0f;
const float Panel::NTC_R25 = 10000.0f;

inline int8_t signoDe(Direccion d) {
	return static_cast<int8_t>(d);
}

Panel::Panel()
: eastFiltered(0), westFiltered(0), error(0), stopThreshold(5.0f),
Kp(2.0f), Ki(0.5f), Kd(0.1f),
integral(0), prevError(0), maxOutput(100.0f),// minOutput(-100.0f),
maIndexEast(0), maIndexWest(0), maSumEast(0), maSumWest(0),
maFilledEast(false), maFilledWest(false),
medIndexEast(0), medIndexWest(0),
medFilledEast(false), medFilledWest(false) , _este(false), _oeste(false), _horizontal(false)
{
	init();
}

void Panel::init() {
	// Configurar ADC
	ADMUX = (1 << REFS0);
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1); // prescaler 64

	// Configurar pines de alarma y límite como entradas con pull-up
	DDRB &= ~((1 << ALARMA_PIN) | (1 << LIMITE_PIN_E)| (1 << LIMITE_PIN_H)| (1 << LIMITE_PIN_W));
	PORTB |= (1 << ALARMA_PIN) | (1 << LIMITE_PIN_E)| (1 << LIMITE_PIN_H)| (1 << LIMITE_PIN_W);

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
	ADMUX = (ADMUX & 0xF0) | (canal & 0x0F);
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC)) {}
	return ADC;
}

// ---------- Filtro de media móvil ----------
float Panel::movingAverage(float newValue, float* buffer, uint8_t& index, float& sum, bool& filled) {
	if (!filled) {
		buffer[index] = newValue;
		sum += newValue;
		index++;
		if (index >= MA_WINDOW) {
			filled = true;
			index = 0;
		}
		return newValue; // mientras no esté lleno, devolvemos el valor sin filtrar
	}
	sum = sum - buffer[index] + newValue;
	buffer[index] = newValue;
	index = (index + 1) % MA_WINDOW;
	return sum / (float)MA_WINDOW;
}

// ---------- Filtro mediano ----------
uint16_t Panel::medianFilter(uint16_t newValue, uint16_t* buffer, uint8_t& index, bool& filled) {
	buffer[index] = newValue;
	index = (index + 1) % MED_WINDOW;
	if (!filled && index == 0) {
		filled = true;
	}
	if (!filled) {
		return newValue;
	}
	// Copiar y ordenar
	uint16_t temp[MED_WINDOW];
	for (uint8_t i = 0; i < MED_WINDOW; i++) temp[i] = buffer[i];
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
	uint16_t medEast = medianFilter(rawEast, medBufferEast, medIndexEast, medFilledEast);
	uint16_t medWest = medianFilter(rawWest, medBufferWest, medIndexWest, medFilledWest);

	// Filtro media móvil
	eastFiltered = movingAverage((float)medEast, maBufferEast, maIndexEast, maSumEast, maFilledEast);
	westFiltered = movingAverage((float)medWest, maBufferWest, maIndexWest, maSumWest, maFilledWest);
}

float Panel::getEastFiltered() const { return eastFiltered; }
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
const float stopThreshold = 2.0; // Ajusta según tus LDRs

// Control de dirección (para no quemar el relé PB6)
int lastDirectionSign = 0; // 1 = ESTE, -1 = OESTE, 0 = STOP

// Variables para el PWM por software en PB7 (Período de 200ms = 5Hz, ideal para relé sólido)
const unsigned long PWM_PERIOD_MS = 200;
unsigned long pwmTimerStart = 0;

// Inicialización de parámetros PID (llamar desde el constructor o setup)
void Panel::initPID(float kp, float ki, float kd, float maxOut, float stopThr) {
	Kp = kp;
	Ki = ki;
	Kd = kd;
	maxOutput = maxOut;
	stopThreshold = stopThr;
	integral = 0.0;
	prevError = 0.0;
	pidOutput = 0.0;
	currentError = 0.0;
	lastDirectionSign = 0;
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
float Panel::readTemperature(int ch) {
	uint16_t adc = leerADC(ch); // ADC6
	return calcularTemperatura(adc);
}

float Panel::calcularTemperatura(uint16_t adcValue) {
	float resistance = SERIES_RESISTOR / ((1023.0f / (float)adcValue) - 1.0f);
	float tempK = 1.0f / (1.0f/298.15f + (1.0f/NTC_BETA) * logf(resistance / NTC_R25));
	return tempK - 273.15f;
}

bool Panel::isAlarm() const {
	return (PINB & (1 << ALARMA_PIN)) != 0;
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
#include "tracker.h"
#include "motor.h"
#include <math.h>
#include <avr/io.h>
#include <stdlib.h>

// Coordenadas geográficas
#define LATITUDE   20.89   // Holguín, Cuba
#define LONGITUDE -76.26

// Deadzone para evitar movimientos innecesarios
#define DEADZONE 50

// Pines de finales de carrera
#define LIMIT_LEFT   PINB0
#define LIMIT_RIGHT  PINB1

// --- Función principal ---
void trackerUpdate(int ldrLeft, int ldrRight, DateTime dt, TrackerMode mode) {
	// Ejemplo: al final del día (hora > 18:00), volver al origen
	if (dt.hour >= 18) {
		trackerReturnHome();
		return;
	}

	switch(mode) {
		case MODE_LDR:
		trackerLDR(ldrLeft, ldrRight);
		break;
		case MODE_ASTRONOMICAL:
		trackerAstronomical(dt);
		break;
		case MODE_HYBRID:
		trackerHybrid(ldrLeft, ldrRight, dt);
		break;
	}
}

// --- Seguimiento por sensores ---
void trackerLDR(int ldrLeft, int ldrRight) {
	int diff = ldrLeft - ldrRight;
	if (abs(diff) > DEADZONE) {
		if (diff > 0 && !limitLeftActive()) motorLeft();
		else if (diff < 0 && !limitRightActive()) motorRight();
		} else {
		motorStop();
	}
}

// --- Seguimiento por cálculo astronómico ---
void trackerAstronomical(DateTime dt) {
	float azimuth = calculateAzimuth(dt);

	// Ejemplo simplificado: si azimut > 180 → girar derecha
	if (azimuth > 180 && !limitRightActive()) motorRight();
	else if (azimuth <= 180 && !limitLeftActive()) motorLeft();
	else motorStop();
}

// --- Seguimiento híbrido ---
void trackerHybrid(int ldrLeft, int ldrRight, DateTime dt) {
	trackerLDR(ldrLeft, ldrRight);

	if (abs(ldrLeft - ldrRight) <= DEADZONE) {
		trackerAstronomical(dt);
	}
}

// --- Retorno automático al origen ---
void trackerReturnHome() {
	// Mover hacia la izquierda hasta que se active el final de carrera
	while (!limitLeftActive()) {
		motorLeft();
	}
	motorStop();
}
/*

// --- Funciones de finales de carrera ---
bool limitLeftActive() {
	return (PINB & (1<<LIMIT_LEFT)) == 0; // activo en bajo
}
bool limitRightActive() {
	return (PINB & (1<<LIMIT_RIGHT)) == 0;
}

*/
// --- Cálculo astronómico simplificado ---
float calculateAzimuth(DateTime dt) {
	int N = dt.day; // día del año simplificado
	float decl = -23.44 * cos((360.0/365.0)*(N+10) * M_PI/180.0);
	float hourAngle = (dt.hour + dt.minute/60.0 - 12) * 15.0;

	float azimuth = atan2(sin(hourAngle * M_PI/180.0),
	cos(hourAngle * M_PI/180.0)*sin(LATITUDE*M_PI/180.0) -
	tan(decl*M_PI/180.0)*cos(LATITUDE*M_PI/180.0));
	return azimuth * 180.0 / M_PI; // en grados
}

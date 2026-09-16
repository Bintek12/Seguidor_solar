#ifndef TRACKER_H
#define TRACKER_H

#include "rtc.h"

// Modos de funcionamiento
enum TrackerMode {
	MODE_LDR,
	MODE_ASTRONOMICAL,
	MODE_HYBRID
};

// Funciones principales
void trackerUpdate(int ldrLeft, int ldrRight, DateTime dt, TrackerMode mode);

// Funciones auxiliares
void trackerLDR(int ldrLeft, int ldrRight);
void trackerAstronomical(DateTime dt);
void trackerHybrid(int ldrLeft, int ldrRight, DateTime dt);

// Función de retorno al origen
void trackerReturnHome();

float calculateAzimuth(DateTime dt);


#endif

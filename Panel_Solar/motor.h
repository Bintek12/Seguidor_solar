#ifndef MOTOR_H
#define MOTOR_H

#include "config.h"

class Motor {
	public:
	Motor();                    // Constructor (llama a init)
	void init();
	void girarEste();
	void girarOeste();
	void stop();
	bool isMoving() const;

	private:
	bool moving;
	enum Estado { STOP, ESTE, OESTE } estado;
};

#endif
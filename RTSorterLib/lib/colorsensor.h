#ifndef _COLORSENS_H_
#define _COLORSENS_H_

#include "kernel.h"
#include "kernel_id.h"
#include "ecrobot_interface.h"
#include "math.h"
#include "motor.h"
#include "bounded-buffer.h"

/* Representation of a feeder */
struct ColorSensor {
	U32 port;
	int colorSensorBackgroundLight;
	int last_time_candy_detected;
};

void color_sensor_initialize(struct ColorSensor* colorSensor, U32 port);
int color_sensor_detect(struct ColorSensor* colorSensor, int *colorValue);
#endif

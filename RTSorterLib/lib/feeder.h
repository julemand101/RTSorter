#ifndef _FEEDER_H_
#define _FEEDER_H_

#include "kernel.h"
#include "kernel_id.h"
#include "ecrobot_interface.h"
#include "math.h"
#include "motor.h"
#include "bounded-buffer.h"

/* NXT PORT CONFIGURATIONS */
U32 black_feeder_motor_port			= NXT_PORT_A;
U32 red_feeder_motor_port 			= NXT_PORT_B;
U32 black_feeder_light_sensor_port 	= NXT_PORT_S1;
U32 red_feeder_light_sensor_port	= NXT_PORT_S2;

/* MISC */
#define MAX_LIGHT_SENSOR_VALUE 		1023		/* Max value according to spec. */

/* VARIABLES used in PROGRAM LOGIC */
int recentCandyPushTime;		/* The time when a candy was recently pushed onto the masterbelt */

/* ID's for available feeders */
typedef enum {
	BLACK,
	RED
} feederId;

/* Representation of a feeder */
struct Feeder {
	U32 motor;
	U32 sensor;
	feederId id;
	int light_sensor_value;
	int min_light_sensor_value;
	int speed;
	int isRunning;
	int idleLightSensorValue;
	char* prettyname;
};

void feeder_react_on_candy(struct Candy* candy, struct BoundedBuffer* candyBuffer, int pushFrequency);
int feeder_has_candy(struct Feeder* feeder);
void feeder_initalize(struct Feeder* feeder, feederId id, U32 sensor, U32 motor, int feeder_speed, char* prettyName);
void feeder_stop(struct Feeder* feeder);
void feeder_start(struct Feeder* feeder);
#endif

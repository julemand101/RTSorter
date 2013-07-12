#ifndef __ERR_DETECT__
#define __ERR_DETECT__

#include "kernel.h"
#include "kernel_id.h"
#include "ecrobot_interface.h"
#include "feeder.h"

struct ErrorCounter {
	int lastValue;
	int counter;
};

void initialize_error_counter(struct ErrorCounter* errCounter);
void error_detect_motor_disconnected(int deviceReading, struct ErrorCounter* errCounter, char* deviceName);
void error_detect_sensor_disconnected(int deviceReading, struct ErrorCounter* errCounter, char* deviceName);
void error_detect_dogkick_received();
void error_detect_is_dog_alive();
void emergency_stop(char* errMsgLine1, char* errMsgLine2);
void error_detect_initialize();

#endif

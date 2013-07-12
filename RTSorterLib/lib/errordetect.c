#include "errordetect.h"
#include "feeder.h"
#include "candy.h"
#include "bounded-buffer.h"
#include "motor.h"
#include "display.h"
#include "RS485_comm.h"

void initialize_error_counter(struct ErrorCounter* errCounter) {
	errCounter->counter = 0;
	errCounter->lastValue = 0;
}

void error_detect_motor_disconnected(int deviceReading, struct ErrorCounter* errCounter, char* deviceName) {
	//Check if the last 6 counts has been the same (= something is wrong)

	if (deviceReading == errCounter->lastValue) {
		errCounter->counter++;
		errCounter->lastValue = deviceReading;
	} else {
		errCounter->lastValue = deviceReading;
		errCounter->counter = 0;
	}

	if (errCounter->counter == 6) {
		emergency_stop(deviceName,"Disconnected");
	}
}

void error_detect_sensor_disconnected(int deviceReading, struct ErrorCounter* errCounter, char* deviceName) {
	//Check if the last 6 counts has been the same (= something is wrong)
	if (deviceReading == 1023) {
		errCounter->counter++;
		//errCounter->lastValue = deviceReading;
	} else {
		//errCounter->lastValue = deviceReading;
		errCounter->counter = 0;
	}

	if (errCounter->counter == 6) {
		emergency_stop(deviceName,"Disconnected");
	}
}

int lastDogKickReceived;
int lastDogKickSent;

void error_detect_dogkick_received() {
	lastDogKickReceived = systick_get_ms();
	//Check if "the dog is dead" (we havn't received anything for too long)
}

void error_detect_is_dog_alive() {
	if(systick_get_ms() - lastDogKickReceived > 200) { // > X, where X is the time in ms. that we allow the NXTs to NOT communicate.
		emergency_stop("RS485 Error","Watchdog Timeout");
	}
}

void emergency_stop(char* errMsgLine1, char* errMsgLine2) {
	motor_set_speed(NXT_PORT_A,0,1);
	motor_set_speed(NXT_PORT_B,0,1);
	motor_set_speed(NXT_PORT_C,0,1);
	print_clear_display();
	print_str(0,1,errMsgLine1);
	print_str(0,2,errMsgLine2);

	int i;
	for(i = 0; i < 3; i++) {
		print_str(6,4,"ERROR");
		print_update();
		ecrobot_set_light_sensor_active(NXT_PORT_S1);
		ecrobot_set_light_sensor_active(NXT_PORT_S2);
		ecrobot_set_light_sensor_active(NXT_PORT_S3);
		ecrobot_sound_tone(1000,500,15);
		systick_wait_ms(750);
		print_clear_line(4);
		print_update();
		ecrobot_set_light_sensor_inactive(NXT_PORT_S1);
		ecrobot_set_light_sensor_inactive(NXT_PORT_S2);
		ecrobot_set_light_sensor_inactive(NXT_PORT_S3);
		ecrobot_sound_tone(500,500,15);
		systick_wait_ms(750);
	}

	motor_set_speed(NXT_PORT_A,0,0);
	motor_set_speed(NXT_PORT_B,0,0);
	motor_set_speed(NXT_PORT_C,0,0);

	while(true) {
		print_str(6,4,"ERROR");
		print_update();
		ecrobot_set_light_sensor_active(NXT_PORT_S1);
		ecrobot_set_light_sensor_active(NXT_PORT_S2);
		ecrobot_set_light_sensor_active(NXT_PORT_S3);
		systick_wait_ms(750);
		print_clear_line(4);
		print_update();
		ecrobot_set_light_sensor_inactive(NXT_PORT_S1);
		ecrobot_set_light_sensor_inactive(NXT_PORT_S2);
		ecrobot_set_light_sensor_inactive(NXT_PORT_S3);
		systick_wait_ms(750);
	}
}

void error_detect_initialize() {
	lastDogKickReceived = systick_get_ms() + 1000; //Gives us a second to start the other NXT
}

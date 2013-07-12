/*
 ============================================================================
 Name        : feeder.C
 Author      : S502A
 Version     : 3.1
 Description : Provides handling for feeding the masterbelt.
 Usage		 : By using a candy buffer, the feeders are able
 	 	 	   to feed the masterbelt by one candy pr. some
 	 	 	   time period.
 ============================================================================
 */

#include "kernel.h"
#include "kernel_id.h"
#include "ecrobot_interface.h"
#include "feeder.h"
#include "display.h"
#include "wcet.h"

/* Checks if provided feeder has detected candy */
int feeder_has_candy(struct Feeder* feeder) {

	/* When a candy is first detected by the light sensor, the value returned from the sensor
	 * gets lower, and lower, and lower - until it reaches a minimum. When we get the minimum, we
	 * know for sure, that we have detected _ONE PIECE_ of candy, and that it has passed the sensor.
	 * This if-clause helps determining the minimum value. */
	if(feeder->light_sensor_value < feeder->min_light_sensor_value && feeder->light_sensor_value < feeder->idleLightSensorValue) {
		feeder->min_light_sensor_value = feeder->light_sensor_value;
	}

	/* If the feeder light sensor value is greater than the the minimum light value
	 * detected, and the feeder light sensor value is greater than the idle light
	 * sensor value, this means that a candy has passed the sensor, as the reported
	 * value from the sensor is on its way up to normal again. */
	if(feeder->light_sensor_value > feeder->min_light_sensor_value && feeder->light_sensor_value > feeder->idleLightSensorValue){
		/* The mininmum value reported from the light sensor is reset to get ready
		 * for the next candy. */
		feeder->min_light_sensor_value = 1023;
		return 1;
	}
	else {
		return 0;
	}
}

/* Define how the feeders should react on the candy enqueued. */
void feeder_react_on_candy(struct Candy* candy, struct BoundedBuffer* candyBuffer, int pushFrequency) {

	/* Peek at the second oldest candy to be dequeued */
	struct Candy* nextCandy = buffer_peek(1, candyBuffer);

	/* If it exists, and its related feeder is not related to the
	 * current candy's, stop the motor related to the next candy
	 * and raise the flag */
	if (nextCandy != null && nextCandy->feeder->id != candy->feeder->id) {
		feeder_stop(nextCandy->feeder);
		#ifdef WCET_ANAL
		assertionTrue(0);
		#endif
	}

	/* If the current timestamp minus the timestamp for when the recent
	 * piece of candy was feeded to the masterbelt is lower than the allowed
	 * feeding frequency, we have to wait until the time limit has passed */
	if ((systick_get_ms() - recentCandyPushTime) < pushFrequency) {
		/* Ensure the feeder is stopped while waiting */
		feeder_stop(candy->feeder);
		#ifdef WCET_ANAL
		assertionTrue(1);
		#endif
	} else {
		/* We're now allowed to feed the first element of the queue,
		 * to the master belt. It must therefore be dequeued from
		 * the candy queue. */
		buffer_dequeue(candyBuffer);

		/* Ensure the feeder related to the current candy is running.
		 * Otherwise, we will not be able to feed the candy to the
		 * master belt. */
		feeder_start(candy->feeder);

		/* Update the timestamp, saying when the recent piece of candy
		 * was feeded to the master belt. */
		recentCandyPushTime = systick_get_ms();
		#ifdef WCET_ANAL
		assertionTrue(2);
		#endif
	}
}

void feeder_initalize(struct Feeder* feeder, feederId id, U32 sensor, U32 motor, int feeder_speed, char* prettyName) {
	feeder->id 						= id;
	feeder->sensor 					= sensor;
	feeder->motor 					= motor;
	feeder->min_light_sensor_value 	= MAX_LIGHT_SENSOR_VALUE;
	feeder->speed 					= feeder_speed;
	feeder->prettyname				= prettyName;

	/* Defines the typical idle light sensor value, when no candy is detected.
	 * This value is set lower than the approx. 750 to 780 that is normally reported
	 * on idle. The value could therefore have been closer to these values, however,
	 * by doing this (using 740), we make sure that we are idling if the light sensor
	 * value is greater than this. Going to close to 750 to 780 will make it unnessecary
	 * complex to derive if we are idling or not. */
	feeder->idleLightSensorValue	= ecrobot_get_light_sensor(sensor) - 30; //We dont believe the light sensors to have a idle range of 30 :D
}

void feeder_stop(struct Feeder* feeder) {
	motor_set_speed(feeder->motor, 0 ,1);
	feeder->isRunning = false;
}

void feeder_start(struct Feeder* feeder) {
	motor_set_speed(feeder->motor, feeder->speed, 1);
	feeder->isRunning = true;
}


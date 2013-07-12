/*
 ============================================================================
 Name        : colorsensor.c
 Author      : S502A
 Version     :
 Description :
 ============================================================================
 */

#include "colorsensor.h"
#include "display.h"
#include "math.h"
#include "wcet.h"

#define MAX_LIGHT_VALUE 1023
#define NOISE_VALUE 3 //Jacob says that 3 is better than 5, therefore, this is 2. Any questions?

static int last_light_value = MAX_LIGHT_VALUE;
static int we_got_a_candy = false;

int color_sensor_detect(struct ColorSensor* colorSensor, int *colorValue) {
	int new_light_value = ecrobot_get_light_sensor(colorSensor->port);
	int delta = new_light_value - last_light_value;

	if (new_light_value < (colorSensor->colorSensorBackgroundLight - 20) && abs(delta) > NOISE_VALUE)
    {
        if (delta < 0) {
        	we_got_a_candy = true;
        }

        if (we_got_a_candy && delta >= 0) {
        	*colorValue = last_light_value;
        	we_got_a_candy = false;
			#ifdef WCET_ANAL
			assertionTrue(3);
			#endif
        	return 1;
        }

        last_light_value = new_light_value;
    }

	return 0;
}

void color_sensor_initialize(struct ColorSensor* colorSensor, U32 port) {

    colorSensor->port = port;
    colorSensor->last_time_candy_detected = 0;
    colorSensor->colorSensorBackgroundLight = ecrobot_get_light_sensor(port);
}

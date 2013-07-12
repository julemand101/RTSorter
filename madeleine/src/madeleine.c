#include "kernel.h"
#include "kernel_id.h"
#include "ecrobot_interface.h"
#include "lib/feeder.h"
#include "lib/RS485_comm.h"
#include "lib/bounded-buffer.h"
#include "lib/errordetect.h"
#include "lib/colorsensor.h"
#include "lib/display.h"

/* NXT PORT CONFIGURATIONS */
#define BLACK_FEEDER_MOTOR_PORT			NXT_PORT_A
#define RED_FEEDER_MOTOR_PORT 			NXT_PORT_B
#define CONVEYOR_MOTOR_PORT 			NXT_PORT_C
#define BLACK_FEEDER_LIGHT_SENSOR_PORT 	NXT_PORT_S1
#define RED_FEEDER_LIGHT_SENSOR_PORT 	NXT_PORT_S2
#define CONVEYOR_COLOR_SENSOR_PORT 		NXT_PORT_S3

/* SETTINGS */
#define VERSION 		"MADELEINE 2.1"
#define VERBOSE_ENABLED
//#define WCET_ANAL
#define FEEDER_SPEED	-100		//Conveyor speed of both feeders
#define PUSH_FREQUENCY 	1000 		//Defines the minimum required period (in ms) between candy feeded to the master belt

#ifdef WCET_ANAL
	#include "lib/wcet.h"
#endif

/* Counter and task declarations */
DeclareCounter(SysTimerCount);
DeclareTask(LIGHT_SENSOR);
DeclareTask(DEQUEUER);
DeclareTask(ENQUEUER_BLACK);
DeclareTask(ENQUEUER_RED);
DeclareTask(ERROR_DETECT);
DeclareTask(NETWORK_SEND);
DeclareTask(NETWORK_RECEIVE);

/* Red feeder declarations */
struct Feeder redFeeder;

/* Black feeder declarations */
struct Feeder blackFeeder;

/* Common feeder variables */
static struct BoundedBuffer candyBuffer;	//Contains latest sensor readings to detect if they are all 1023 (=error)
static struct Candy candyBufferArray[32];	//The actual array that the above bounded buffer stores values in

/* Color sensor declarations */
struct ColorSensor colorSensor;

/* Network variables */
int token;											//Used to make token passing work

/* Error checking variables */
int start; 											//0 until a start-package has been received from Stephie.
int inErrorMode;									//1 if an error-package is received from Stephie (when candy lies too close
int dogkick; 										//1 = "keep-alive" dog-kick received from Stephie. 0 = no dog kick received.

static struct ErrorCounter errCntSensorBlack;		//Contains latest sensor readings to detect if they are all 1023 (=error)
static struct ErrorCounter errCntSensorRed;			//Contains latest sensor readings to detect if they are all 1023 (=error)
static struct ErrorCounter errCntSensorColor;		//Contains latest sensor readings to detect if they are all 1023 (=error)
static struct ErrorCounter errCntMotorBlack;		//Contains succesive motor readings - used for error checking
static struct ErrorCounter errCntMotorRed;			//Contains succesive motor readings - used for error checking

/*****************************************************************************************************************************/
/* Initialization hook: start sensors, initialize feeders
 * and assign values to global variables */
void ecrobot_device_initialize(void)
{
	/* Initialize feeders */
	feeder_initalize(
			&blackFeeder,
			BLACK,
			BLACK_FEEDER_LIGHT_SENSOR_PORT,
			BLACK_FEEDER_MOTOR_PORT,
			FEEDER_SPEED,
			"Feeder Two");
	feeder_initalize(
			&redFeeder,
			RED,
			RED_FEEDER_LIGHT_SENSOR_PORT,
			RED_FEEDER_MOTOR_PORT,
			FEEDER_SPEED,
			"Feeder One");

	/* Initialize the light sensor recognizing color */
	color_sensor_initialize(&colorSensor,CONVEYOR_COLOR_SENSOR_PORT);

	/* Enable the light sensors */
	ecrobot_set_light_sensor_active(blackFeeder.sensor);
	ecrobot_set_light_sensor_active(redFeeder.sensor);
	ecrobot_set_light_sensor_active(CONVEYOR_COLOR_SENSOR_PORT);

	/* Initialize the candy buffer containing candy enqueded by
	 * the light sensors at the feeders */
	buffer_initialize(&candyBuffer, candyBufferArray, 32, "candyBuffer");

	/* Initializations used for error detection */
	initialize_error_counter(&errCntMotorBlack);
	initialize_error_counter(&errCntMotorRed);
	initialize_error_counter(&errCntSensorBlack);
	initialize_error_counter(&errCntSensorRed);
	initialize_error_counter(&errCntSensorColor);

	/* Initialize rs485 network connection */
	initialize_rs485();

	/* Assign initial values to other global variables */
	dogkick 						= -1;	//Used for watchdog
	start 							= -1; 	//Waiting for start signal from Stephanie
	inErrorMode 						= 0; 	//Default: not in error mode
	token 							= 0; 	//Stephanie has the token at start

	/* GLOBAL VARIABLE from feeder.h */
	recentCandyPushTime 			= 0;	//Timestamp indicating when feeders last detected candy
}

/* To be run when pressing run (right arrow) on the NXT */
void StartupHook(void)
{
#ifdef VERBOSE_ENABLED
	display_goto_xy(0,0);
	display_string(VERSION);
	display_update();
#endif
}

/* Device termination hook: break motors */
void ecrobot_device_terminate(void)
{
	motor_set_speed(blackFeeder.motor, 		0, 1);
	motor_set_speed(redFeeder.motor, 		0, 1);
	motor_set_speed(CONVEYOR_MOTOR_PORT, 	0, 1);
}

/* nxtOSEK hook to be invoked from an interrupt service routine (ISR) in category 2 */
void user_1ms_isr_type2(void)
{
  StatusType ercd;

  ercd = SignalCounter(SysTimerCount); /* Increment OSEK Alarm Counter */
  if (ercd != E_OK)
  {
    ShutdownOS(ercd);
  }
}

/*****************************************************************************************************************************/
/* ENQUEUER TASK: responsible for enqueueing candy registered by the
 * RED feeder. The candy is enqueued to the global candy buffer. */
TASK(ENQUEUER_RED)
{
	#ifdef WCET_ANAL
		displayResultsForMadeleine();
		print_update();
		startTimer(Mtask_ENQUEUER_RED);
	#endif

	/* If madeleine is not allowed to "start" proper functionality by
	 * steph., or if we run in error mode, this task should terminate
	 * when started. */
	if (start != 1 || inErrorMode != 0) {
		#ifdef WCET_ANAL
			stopTimer(Mtask_ENQUEUER_RED);
		#endif
		TerminateTask();
	}

	/* Get light sensor value from red feeder */
	if(redFeeder.isRunning) {
		redFeeder.light_sensor_value = ecrobot_get_light_sensor(redFeeder.sensor);
	}

	#ifdef VERBOSE_ENABLED
		display_goto_xy(0,3);
		display_string("ONE RAW    = ");
		display_int(redFeeder.light_sensor_value, 0);
		print_update();
	#endif

	/* If candy is detected, enqueue it */
	if (feeder_has_candy(&redFeeder)) {
		ecrobot_sound_tone(1000, 200, 100);
		struct Candy newCandy;
		newCandy.feeder = &redFeeder;
		buffer_enqueue(&candyBuffer, newCandy);
	}
	#ifdef WCET_ANAL
		stopTimer(Mtask_ENQUEUER_RED);
	#endif

	TerminateTask();
}

/*****************************************************************************************************************************/
/* ENQUEUER TASK: responsible for enqueueing candy registered by the
 * BLACK feeder. The candy is enqueued to the global candy buffer. */
TASK(ENQUEUER_BLACK)
{
	#ifdef WCET_ANAL
		startTimer(Mtask_ENQUEUER_BLACK);
	#endif

	/* If madeleine is not allowed to "start" proper functionality by
	 * steph., or if we run in error mode, this task should terminate
	 * when started. */
	if (start != 1 || inErrorMode != 0) {
		#ifdef WCET_ANAL
			stopTimer(Mtask_ENQUEUER_BLACK);
		#endif
		TerminateTask();
	}

	/* Get light sensor value from black feeder */
	if(blackFeeder.isRunning) {
		blackFeeder.light_sensor_value = ecrobot_get_light_sensor(blackFeeder.sensor);
	}

	#ifdef VERBOSE_ENABLED
		display_goto_xy(0,4);
		display_string("TWO RAW    = ");
		display_int(blackFeeder.light_sensor_value, 0);
		print_update();
	#endif

	/* If candy is detected, enqueue it */
	if (feeder_has_candy(&blackFeeder)) {
		ecrobot_sound_tone(1000, 200, 100);
		struct Candy newCandy;
		newCandy.feeder = &blackFeeder;
		buffer_enqueue(&candyBuffer, newCandy);
	}
	#ifdef WCET_ANAL
		stopTimer(Mtask_ENQUEUER_BLACK);
	#endif
	TerminateTask();
}

/*****************************************************************************************************************************/
/* DEQUEUER TASK: responsible for dequeueing candy
 * from the global candy buffer. */
TASK(DEQUEUER) {
	#ifdef WCET_ANAL
		startTimer(Mtask_DEQUEUER);
	#endif

	/* If madeleine is not allowed to "start" proper functionality by
	 * steph., or if we run in error mode, this task should terminate
	 * when started. */
	if (start != 1 || inErrorMode != 0) {
		#ifdef WCET_ANAL
			stopTimer(Mtask_DEQUEUER);
		#endif
		TerminateTask();
	}
	#ifdef VERBOSE_ENABLED
		print_clear_line(5);
		display_goto_xy(0,5);
		print_str(0,5, "ONE BG     = ");
		display_int(redFeeder.idleLightSensorValue, 0);
		print_clear_line(6);
		display_goto_xy(0,6);
		print_str(0,6, "TWO BG     = ");
		display_int(blackFeeder.idleLightSensorValue, 0);
		display_update();
//		print_clear_line(5);
//		display_goto_xy(0,5);
//		display_string("C.BUF READ = ");
//		display_int(candyBuffer.read, 0);
//		print_clear_line(6);
//		display_goto_xy(0,6);
//		display_string("C.BUF WRITE= ");
//		display_int(candyBuffer.write, 0);
//		display_goto_xy(0,7);
//		display_string("C.BUF LNGT = ");
//		display_int(buffer_count_elements(&candyBuffer), 0);
//		display_update();
	#endif

	/* Read the candy to feed to the master belt */
	struct Candy* candy = buffer_peek(0, &candyBuffer);

	/* If no candy has been detected by the feeders, no
	 * candy has been enqued, hence the buffer is empty
	 * which is why candy == null. If this is the case,
	 * there is nothing to dequeue, and we should terminate
	 * this task. */
	if(candy == null) {
		#ifdef WCET_ANAL
			stopTimer(Mtask_DEQUEUER);
		#endif
		TerminateTask();
	}

	/* Check how to react on the current candy. Controls if the
	 * opposite feeder should be stopped, or if the current
	 * feeder should be stopped due to the allowed push limit. */
	feeder_react_on_candy(candy, &candyBuffer, PUSH_FREQUENCY);

	#ifdef WCET_ANAL
		stopTimer(Mtask_DEQUEUER);
	#endif

	TerminateTask();
}

/*****************************************************************************************************************************/
/* LIGHT_SENSOR TASK: responsible for detecting the color of candy
 * at the main conveyor belt */
TASK(LIGHT_SENSOR)
{
	#ifdef WCET_ANAL
		startTimer(Mtask_LIGHTSENSOR);
	#endif

	/* If we are not allowed to start normal operation by
	 * Steph., this task should terminate when started. */
    if (start != 1) {
		#ifdef WCET_ANAL
			stopTimer(Mtask_LIGHTSENSOR);
		#endif
    	TerminateTask();
    }

    int colorValue 	= 0;
    int returnVal	= 0;

	#ifdef VERBOSE_ENABLED
    	print_str(0,2, "COLOR VAL  = ");
    	print_update();
	#endif

    /* Get the light value from the light sensor, and store
     * it in the colorValue int. */
    returnVal = color_sensor_detect(&colorSensor, &colorValue);

    /* If that operation went well... */
    if (returnVal == 1) {
		ecrobot_sound_tone(colorValue*2, 10, 100);
		colorValue = colorSensor.colorSensorBackgroundLight - colorValue;
		colorSensor.last_time_candy_detected = systick_get_ms();

		#ifdef VERBOSE_ENABLED
			print_int(13, 2, colorValue);
			print_update();
		#endif

		/* The returned colorValue is invalid */
		if (colorValue > 300) {
			/* Error value - no color value is this high! Feed whatever it is to the dragon (do nothing) */
			#ifdef WCET_ANAL
				assertionTrue(4);
			#endif
		} else {
			/* The colorvalue is normal, and we send it to Steph. */
			sendint_rs485(colorValue);
		}
    }

	#ifdef WCET_ANAL
		stopTimer(Mtask_LIGHTSENSOR);
	#endif

    TerminateTask();
}

/*****************************************************************************************************************************/
/* ERROR_DETECT TASK: responsible for detecting if various
 * errors occurs on madeleine */
TASK(ERROR_DETECT)
{
	#ifdef WCET_ANAL
		startTimer(Mtask_ERRORDETECT);
	#endif

	/* Check if the motor on feeder black is running */
	if (blackFeeder.isRunning == 1) {
		error_detect_motor_disconnected(nxt_motor_get_count(blackFeeder.motor), &errCntMotorBlack, "Feeder Two");
	}
	/* Check if the motor on feeder red is running */
	if (redFeeder.isRunning == 1) {
		error_detect_motor_disconnected(nxt_motor_get_count(redFeeder.motor), &errCntMotorRed, "Feeder One");
	}

	/* Chech if the feeders ligt sensors or
	 * the main belt light sensor are disconnected */
	error_detect_sensor_disconnected(ecrobot_get_light_sensor(BLACK_FEEDER_LIGHT_SENSOR_PORT), &errCntSensorBlack, "Sensor Two");
	error_detect_sensor_disconnected(ecrobot_get_light_sensor(RED_FEEDER_LIGHT_SENSOR_PORT), &errCntSensorRed, "Sensor One");
	error_detect_sensor_disconnected(ecrobot_get_light_sensor(CONVEYOR_COLOR_SENSOR_PORT), &errCntSensorColor, "Color Sensor");

	/* If madeleine is allowed to run, check if
	 * the rotorblink should "start" or "stop".
	 * Further, check if the network is alive. */
	if (start == 1) {
		if (dogkick == 1) {
			motor_set_speed(NXT_PORT_C, 100, 0); //Rotorblink GO!
			dogkick = 0;
			/* Network check */
			error_detect_dogkick_received();
			error_detect_is_dog_alive();
		} else {
			motor_set_speed(NXT_PORT_C, 0, 0); //Rotorblink STOP!
			/* Network check */
			error_detect_is_dog_alive();
		}
	} else if (start == -1) {
		/* Keep updating the dogkick received timestamp,
		 * to prevent network failure when madie is not
		 * allowed to run */
		error_detect_dogkick_received();
	}

	#ifdef WCET_ANAL
		stopTimer(Mtask_ERRORDETECT);
	#endif

	TerminateTask();
}

/*****************************************************************************************************************************/
/* NETWORK_SEND TASK: responsible for sending information
 * to stephanie. The task is based on token-passing, hence
 * madeleine is only allowed to send data if it has the
 * token. */
TASK(NETWORK_SEND)
{
	#ifdef WCET_ANAL
		startTimer(Mtask_NETWORK_SEND);
	#endif
	/* If we have the token, and the buffer is empty,
	 * then send 1337 to indicate that we are alive,
	 * and to "free" the token. */
	if (token == 1 && buffer_count_elements(&sendBuffer) == 0) {
		/* Nothing in buffer. Send the token */
		token = 0;
		sendint_rs485(1337);
		send_buffered_ints_rs485();
	}
	else if (token == 1) {
		/* Something is in the send buffer. Send it. */
		send_buffered_ints_rs485();
	}

	#ifdef WCET_ANAL
		stopTimer(Mtask_NETWORK_SEND);
	#endif
	TerminateTask();
}

/*****************************************************************************************************************************/
/* NETWORK_RECEIVE TASK: responsible for receiving
 * and reacting on data sent from steph. */
TASK(NETWORK_RECEIVE)
{
	#ifdef WCET_ANAL
		startTimer(Mtask_NETWORK_RECEIVE);
	#endif

	int receive = 0;

	/* Wait for a value has been received from steph. */
	if (recieveint_rs485(&receive))
	{
		dogkick = 1;

		/* Standard message indicating that steph is alive */
		if (receive == 1337) {
			token = 1; //Made has the token

			/* If madeleine has not yet started, this signal
			 * from steph. allows madie to start normal operation. */
			if (start != 1) {
				start = 1;					//From now on, madeleine should be in "normal operation" mode
				feeder_start(&blackFeeder);	//Start the black feeder
				feeder_start(&redFeeder);	//Start the red feeder
			}
			#ifdef WCET_ANAL
				stopTimer(Mtask_NETWORK_RECEIVE);
			#endif
			TerminateTask();
		}

		/* Steph reports that madeleine may leave error mode,
		 * and continue to operate in normal mode. */
		if (receive == 8000) {
			inErrorMode = 0;			//Leave error-mode
			feeder_start(&blackFeeder);	//Start the black feeder again
			feeder_start(&redFeeder);	//Start the red feeder again

			/* Imagine the following scenarium: if some candy
			 * has been enqueued by one or both feeders, when
			 * stephanie suddenly reported that madeleine has
			 * to go in error mode. When madeleine goes to error
			 * mode, the feeders are stopped and the deqeue
			 * task is not allowed to dequeue candy. After a
			 * while, the error on the main belt has been re-solved
			 * and steph. gives madeleine green light for normal
			 * operation again. When returning to normal mode, it is
			 * VERY IMPORTANT that the timestamp for when candy last
			 * was feeded to the main belt, is updated. If this is not
			 * done, the push-frequency will be somehow violated, and
			 * candy might collide on the main belt. */
			recentCandyPushTime = systick_get_ms();

			#ifdef WCET_ANAL
				assertionTrue(5);
				stopTimer(Mtask_NETWORK_RECEIVE);
			#endif
			TerminateTask();
		}

		/* Steph report that madeleine must enter error mode! */
		if (receive == 37708) { 		//Enter error mode
			feeder_stop(&blackFeeder);	//Stop the black feeder
			feeder_stop(&redFeeder);	//Stop the red feeder
			inErrorMode = 1;			//Indicate error-mode
			#ifdef WCET_ANAL
				assertionTrue(6);
				stopTimer(Mtask_NETWORK_RECEIVE);
			#endif
			TerminateTask();
		}
	}

	#ifdef WCET_ANAL
		stopTimer(Mtask_NETWORK_RECEIVE);
	#endif
	TerminateTask();
}

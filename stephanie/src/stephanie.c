#include "kernel.h"
#include "kernel_id.h"
#include "ecrobot_interface.h"
#include "lib/motor.h"
#include "lib/bounded-buffer.h"
#include "lib/display.h"
#include "lib/RS485_comm.h"
#include "lib/math.h"
#include "lib/errordetect.h"
#include "lib/pid.h"

#define PUSH_1 		NXT_PORT_A
#define PUSH_2 		NXT_PORT_B
#define CONVEYOR 	NXT_PORT_C
#define VERSION 	"STEPHANIE 2.1"

/* Speed of main conveyor belt when in error mode */
#define CONVEYOR_SPEED_ERROR_MODE -20;

#define VERBOSE_ENABLED
//#define WCET_ANAL

#ifdef WCET_ANAL
	#include "lib/wcet.h"
#endif

/* Declare counter and tasks */
DeclareCounter(SysTimerCount);
DeclareTask(PUSH);
DeclareTask(ERROR_DETECT);
DeclareTask(NETWORK_SEND);
DeclareTask(NETWORK_RECEIVE);

/* Used to implement the logic of the pistons. The
 * enum is indicating whether a piston is stopped,
 * if it is pushing or if it is returning. */
typedef enum {
	STOPPED,
	PUSHING,
	RETURNING
} pusherStatus;

/* GLOBAL variables */
pusherStatus push1_status;
pusherStatus push2_status;
struct BoundedBuffer colorBufferWhite;	//Store blue candy
struct BoundedBuffer colorBufferBlue;	//Store white candy
struct Candy newCandy;					//Placeholder for a new candy detected on the main belt
int start;								//Is steph. allowed to start
int dogkick;							//For network monitoring
int conveyor_voltage;						//Speed of the main conveyor belt
int inErrorMode;						//Indication of error mode
int startedTime;						//Timestamp for when steph. was started
int token;								//Used with token-passing
struct PID conveyorPID;					//PID used to regulate conveyor PID
int pidDelayCounter;

/* Used for error detection on pistons and main conveyor belt */
struct ErrorCounter piston1ErrCnt;
struct ErrorCounter piston2ErrCnt;
struct ErrorCounter conveyorErrCnt;

void StartupHook(void)
{
	/* When Steph. was started */
	startedTime = systick_get_ms();

	/* Initialize RS485 connection */
	initialize_rs485();
}

/*****************************************************************************************************************************/
/* Initialization hook: initialize color buffers and error counters */
void ecrobot_device_initialize(void)
{
	static struct Candy colorBufferWhiteArray[24];
	static struct Candy colorBufferBlueArray[24];
	buffer_initialize(&colorBufferWhite, colorBufferWhiteArray, 24, "pist1buffer");
	buffer_initialize(&colorBufferBlue, colorBufferBlueArray, 24, "pist2buffer");
	initialize_error_counter(&piston1ErrCnt);
	initialize_error_counter(&piston2ErrCnt);
	initialize_error_counter(&conveyorErrCnt);

	pidDelayCounter 					= 0;	//Counter to ensure PID update delay.
	dogkick 							= 0;	//Dogkick start value (none received so far)
	start 								= -1;	//Steph. is not allowed normal operation (we need to wait for 5 sec)
	conveyor_voltage 					= -50;	//Default main conveyor speed
	conveyorPID.conveyor_actual_voltage = -52;	//The actual main conveyor speed
	inErrorMode 						= 0;	//Per default: Steph. is not in error mode
	token 								= 1;	//Stephanie has the token from start!
	initialize_motors();						//Initialize motors
}

/* nxtOSEK hook to be invoked from an ISR in category 2 */
void user_1ms_isr_type2(void)
{
	  StatusType ercd;

	  ercd = SignalCounter(SysTimerCount);
	  if (ercd != E_OK)
	  {
	    ShutdownOS(ercd);
	  }
}

/* nxtOSEK hook to be invoked on device terminate */
void ecrobot_device_terminate(void)
{
	/* Break the motors */
    motor_set_speed(PUSH_1, 	0, 1);
    motor_set_speed(PUSH_2, 	0, 1);
    motor_set_speed(CONVEYOR, 	0, 1);
}

/* Compute the tick distance between the color sensor and pusher 1.
 * Documentation available on dropbox. */
int push_1_compute_tick_distance(int conveyorSpeed) {
	return -1*abs(conveyorSpeed)+305;
}

/* Compute the tick distance between the color sensor and pusher 2.
 * The constant is the number of ticks needed to make candy
 * be transported from push1 to push2. */
int push_2_compute_tick_distance(int conveyorSpeed) {
	return push_1_compute_tick_distance(conveyorSpeed) + 330;
}


/*****************************************************************************************************************************/
/* PUSH TASK: responsible for pushing candy into a container. */
TASK(PUSH)
{
	/* If not started normal operation yet, then
	 * terminate this task. */
	if (start != 1)
		TerminateTask();

	#ifdef WCET_ANAL
		displayResultsForStephanie();
		print_update();
		startTimer(Stask_PUSH);
	#endif

	/* Defines if push1 or push2 has just been stopped */
	int push1HasJustBeenStopped = 0;
	int push2HasJustBeenStopped = 0;

	/* Using the async methods for handling motor rotation, we
	 * are able to handle the rotation of multiple motors, like
	 * if they were running in parallel.
	 * The idea is that the motors are started calling motor_rotate_async,
	 * and then we keep checking if the motors have rotated a certain number
	 * of ticks/degress by calling motor_rotate_async_update.
	 * If they have, we know that the motors should be stopped.
	 * Using this idea, we accomplish to handle multiple pistons without making
	 * use of preemption, busy waiting and multiple tasks! */
	motor_rotate_async_update(&push1HasJustBeenStopped, &push2HasJustBeenStopped);

	#ifdef WCET_ANAL
		int pusher1returning = 0;
		int pusher2returning = 0;
		int pusher1pushing = 0;
		int pusher2pushing = 0;
	#endif

	/* If the piston has just been stopped (in some position) */
	if(push1HasJustBeenStopped) {
		/* Check if the piston has been stopped in a pushing positon.
		 * If this is the case, the piston must be instructed to return,
		 * and its status must be updated to be returning. */
		if (push1_status == PUSHING) {
			motor_rotate_async(NXT_PORT_A, 35, 100);
			push1_status = RETURNING;
			#ifdef WCET_ANAL
				pusher1returning = 1;
			#endif
		}
		/* Check if the pusher has been stopped in a returned positon.
		 * If this is the case, update its status to stopped. */
		else if (push1_status == RETURNING) {
			push1_status = STOPPED;
		}
	}
	/* The same logic as above applies here. */
	if(push2HasJustBeenStopped) {
		if (push2_status == PUSHING) {
			motor_rotate_async(NXT_PORT_B, 35, 100);
			push2_status = RETURNING;
			#ifdef WCET_ANAL
				pusher2returning = 1;
			#endif
		}
		else if (push2_status == RETURNING) {
			push2_status = STOPPED;
		}
	}

	/* If the bounded buffer has some items, check if the oldest
	 * one should be pushed into a box and popped from the buffer. */
	if (buffer_count_elements(&colorBufferWhite) > 0) {
		struct Candy* candyToPush = buffer_peek(0, &colorBufferWhite);

		/* If the item to push's tickstamp (the tickstamp it got
		 * when recognized by the sensor) is less than the ticks
		 * turned by the conveyor motor, this means that the candy
		 * is placed outside one of the pistons. The corresponding
		 * piston must react, and push the candy into the box! */
		if (candyToPush->rpmTicksStamp + push_1_compute_tick_distance(conveyor_voltage) <= abs(nxt_motor_get_count(CONVEYOR))) {
			if (candyToPush->pushIt == 1) {
				if (push1_status == STOPPED) {
					push1_status = PUSHING;
					motor_rotate_async(PUSH_1, -60, 100);
					#ifdef WCET_ANAL
						pusher1pushing = 1;
					#endif
				}
			}
			buffer_dequeue(&colorBufferWhite);
		}
	}

	if (buffer_count_elements(&colorBufferBlue) > 0) {
		struct Candy* candyToPush = buffer_peek(0, &colorBufferBlue);

		/* Same logic as above applies here. */
		if (candyToPush->rpmTicksStamp + push_2_compute_tick_distance(conveyor_voltage) <= abs(nxt_motor_get_count(CONVEYOR))) {
			if (candyToPush->pushIt == 1) {
				if (push2_status == STOPPED) {
					push2_status = PUSHING;
					motor_rotate_async(PUSH_2, -60, 100);
					#ifdef WCET_ANAL
						pusher2pushing = 1;
					#endif
				}
			}
			buffer_dequeue(&colorBufferBlue);
		}
	}

	#ifdef WCET_ANAL
		if(pusher1returning == 1 && pusher2returning == 1)
			assertionTrue(7);

		if(pusher1pushing == 1 && pusher2pushing == 1)
			assertionTrue(8);

		stopTimer(Stask_PUSH);
	#endif
	TerminateTask();
}

/*****************************************************************************************************************************/
/* ERROR_DETECT TASK: responsible for detecting abnormal function in the system. */

int conveyorDegreesLastTime = 0;
int conveyorTimerLastTime = 0;

TASK(ERROR_DETECT)
{
	/* If Steph. is allowed to start normal operation */
	if (start == 1) {
		#ifdef WCET_ANAL
			startTimer(Stask_ERRORDETECT);
		#endif

		/* Regulate conveyor-speed */
		pidDelayCounter++;

		/* When pid_reset has been called elsewhere (when changing to and from error mode), the servo
		 * has been given an initial voltage to change to (a "best guess", to reduce the time the pid
		 * uses on stabilizing the velocity).
		 *
		 * The time it takes to reach the velocity "dictated" by the voltage, when changing to and from
		 * error mode is larger than when the PID is doing its small corrections as it normally does. To
		 * allow the servo to settle, before the PID starts correcting, pid_reset sets settleTime to
		 * systick now. The following if-statement will keep "resetting" the variables as to when the PID
		 * has been checked last, so that when the 200 ms. has gone, the else-if (the actual PID) knows
		 * the correct "last degrees value" of the servo, allowing it to adjust correctly.
		 */
		if (conveyorPID.settle_time + 200 > systick_get_ms()) {
			conveyorDegreesLastTime = motor_get_count(CONVEYOR);
			conveyorTimerLastTime = systick_get_ms();
			pidDelayCounter = 0;
		}
		else if (pidDelayCounter == 5) {

			pidDelayCounter = 0;

			/* Compute rotated degrees since last reading */
			int degrees = motor_get_count(CONVEYOR);
			int output  = degrees - conveyorDegreesLastTime;
			conveyorDegreesLastTime = degrees;

			/* Compute time-interval for this reading */
			int now = systick_get_ms();
			int scantime = now - conveyorTimerLastTime;
			conveyorTimerLastTime = now;

			/* Compute new error value */
			int pid = pid_update(&conveyorPID,scantime,output,0.4,1000,100,true,50);

			/* Compute new actual speed of main conveyor belt */
			conveyorPID.conveyor_actual_voltage = conveyorPID.conveyor_actual_voltage + pid;

			/* Set the speed of the main conveyor belt to the new actual speed */
			motor_set_speed(CONVEYOR,conveyorPID.conveyor_actual_voltage,1);

			#ifdef VERBOSE_ENABLED
				print_clear_line(5);
				print_clear_line(6);
				print_clear_line(7);
				print_str(0,5,"P");
				print_int(0,6,(int)conveyorPID.P);
				print_str(5,5,"I");
				print_int(5,6,(int)conveyorPID.I);
				print_str(10,5,"D");
				print_int(10,6,(int)conveyorPID.D);
				print_str(0,7,"V=");
				print_int(2,7,abs(conveyorPID.conveyor_actual_voltage));
				print_str(5,7,"P=");
				print_int(7,7,abs(output));
				print_str(10,7,"S=");
				print_int(12,7,abs(conveyorPID.sp));
				print_update();
			#endif
		}

		/* Is conveyor motor running with too much force? */
		if (conveyorPID.conveyor_actual_voltage < -100) {
			emergency_stop("Too much candy","on conveyor");
		}

		/* Detect if the pistons are working correct */
		int reading;
		reading = nxt_motor_get_count(PUSH_1);
		if (abs(reading) > 20 || push1_status == 1) { /* piston 1 is out, or instructed to run */
			error_detect_motor_disconnected(reading, &piston1ErrCnt, "Piston White");
		}

		reading = nxt_motor_get_count(PUSH_2);
		if (abs(reading) > 20 || push2_status == 1) { /* piston 2 is out, or instructed to run */
			error_detect_motor_disconnected(reading, &piston2ErrCnt, "Piston Blue");
		}

		/* Detect if the main conveyor belt is running */
		error_detect_motor_disconnected(nxt_motor_get_count(CONVEYOR), &conveyorErrCnt, "Main Belt");

		if (dogkick == 1) {
			dogkick = 0;
			/* Network check */
			error_detect_dogkick_received();
		} else {
			error_detect_is_dog_alive();
		}

		/* If we run in error mode, check if we are good to go back in normal mode again */
		if (inErrorMode) {
			if (buffer_count_elements(&colorBufferBlue) == 0 && buffer_count_elements(&colorBufferWhite) == 0) {
				/* There's nothing more in buffer - speed Steph. up again */
				conveyor_voltage 	= -50;

				pid_reset(&conveyorPID, conveyor_voltage, -52);

				inErrorMode 	= 0;
				motor_set_speed(CONVEYOR,conveyor_voltage,1);
				sendint_rs485(8000);

				#ifdef VERBOSE_ENABLED
					print_clear_line(2);
					print_str(0,2,"MODE = NORMAL");
					print_update();
				#endif

				#ifdef WCET_ANAL
					assertionTrue(9);
				#endif
			}
		}
		#ifdef WCET_ANAL
			stopTimer(Stask_ERRORDETECT);
		#endif
	} else if (start == -1) {
		/* Check if it is 5 seconds since Steph. was started,
		 * and keep counting down until the 5 sec has passed.
		 * When this happens, set start to 1 to allow steph.
		 * to operate in normal mode. When in normal mode, steph.
		 * will ping madeleine and tell her to begin operate
		 * in normal mode too. */
		error_detect_dogkick_received();
		if ((systick_get_ms() - startedTime) > 5000) {
			#ifdef VERBOSE_ENABLED
				print_clear_display();
				print_str(0,0,VERSION);
				print_update();
			#endif
			start = 1;
			motor_set_speed(CONVEYOR,conveyor_voltage,1);
		}
		else {
			print_clear_display();
			print_str(0,0,VERSION);
			print_str(2,2,"STARTING IN:");
			print_int(5,4,5000-(systick_get_ms() - startedTime));
			print_str(9,4,"MS");
			print_update();
			conveyorTimerLastTime = systick_get_ms();
			pid_reset(&conveyorPID, conveyor_voltage, -52);
		}
	}
	TerminateTask();
}

/*****************************************************************************************************************************/
/* NETWORK_SEND TASK: responsible for sending data to madeleine */
TASK(NETWORK_SEND)
{
	if (start != 1)
		TerminateTask();

	#ifdef WCET_ANAL
		startTimer(Stask_NETWORK_SEND);
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
		stopTimer(Stask_NETWORK_SEND);
	#endif
	TerminateTask();
}

/*****************************************************************************************************************************/
/* NETWORK_RECEIVE TASK: responsible for receiving and acting upon data from madeleine.
 * This task defines what to do with the reveived color values. */

int global_blue_push = 0;
int global_white_push = 0;

TASK(NETWORK_RECEIVE)
{
	if (start != 1)
		TerminateTask();

	#ifdef WCET_ANAL
		startTimer(Stask_NETWORK_RECEIVE);
	#endif

	int receive = 0;

	/* Receiver */
	if (token == 0 && recieveint_rs485(&receive))
	{
		/* If we got 1337, steph has the token */
		if (receive == 1337) {
			token 	= 1;
			dogkick = 1;
			#ifdef WCET_ANAL
				stopTimer(Stask_NETWORK_RECEIVE);
			#endif
			TerminateTask();
		}

		/* Dequeue the last candy we received, as we can't safely push it
		 * off the belt because candy is lying too close. */
		if (receive > 8000) {
			#ifdef WCET_ANAL
				stopTimer(Stask_NETWORK_RECEIVE);
			#endif
			TerminateTask();
		}

		/* The following initialization are used to define if
		 * steph. has to report error or dragon mode. */
		struct Candy* recentWhiteCandy 	= buffer_peek(buffer_count_elements(&colorBufferWhite)-1, &colorBufferWhite);
		struct Candy* recentBlueCandy 	= buffer_peek(buffer_count_elements(&colorBufferBlue)-1, &colorBufferBlue);
		int rotationsNow = abs(motor_get_count(CONVEYOR));
		int ticksSinceRecentWhiteCandy 	= rotationsNow - recentWhiteCandy->rpmTicksStamp;
		int ticksSinceRecentBlueCandy 	= rotationsNow - recentBlueCandy->rpmTicksStamp;

		/* We have a candy, define that it is "pushable" */
		newCandy.pushIt = 1;

		/* If their exist either a newly enqueued white or blue candy,
		 * we know that steph. has to look for, if the recent candy and
		 * the current new candy are lying too close on the main belt.  */
		if(recentWhiteCandy != null || recentBlueCandy != null) {

			if (ticksSinceRecentWhiteCandy < 85) {
				/*There's a white candy less than 85 ticks ahead!!
				This is so close, that we have to feed them both to the dragon */

				/* Don't push the pieces of candy */
				recentWhiteCandy->pushIt = 0;
				newCandy.pushIt = 0;
				#ifdef VERBOSE_ENABLED
					print_clear_line(2);
					print_str(0,2,"MODE = DRAGON");
					print_update();
				#endif
			} else if (ticksSinceRecentBlueCandy < 85) {
				/*There's a white candy less than 85 ticks ahead!!
				This is so close, that we have to feed them both to the dragon */

				/* Don't push the pieces of candy */
				recentBlueCandy->pushIt = 0;
				newCandy.pushIt = 0;
				#ifdef VERBOSE_ENABLED
					print_clear_line(2);
					print_str(0,2,"MODE = DRAGON");
					print_update();
				#endif
			} else if (ticksSinceRecentWhiteCandy < 133) {
				/* There's a white candy less than 133 ticks ahead!!
				This is so close, that we have to slow down the main belt */

				conveyor_voltage 	= CONVEYOR_SPEED_ERROR_MODE;

				pid_reset(&conveyorPID,conveyor_voltage, -17);

				inErrorMode 	= 1;
				motor_set_speed(CONVEYOR, conveyor_voltage, 1);
				/* Send signal to madeleine to make that NXT
				 * stop it feeders! */
				sendint_rs485(37708);
				#ifdef VERBOSE_ENABLED
					print_clear_line(2);
					print_str(0,2,"MODE = RESCUE");
					print_update();
				#endif
			} else if (ticksSinceRecentBlueCandy < 133) {
				/* There's a white candy less than 133 ticks ahead!!
				This is so close, that we have to slow down the main belt */

				conveyor_voltage 	= CONVEYOR_SPEED_ERROR_MODE;

				pid_reset(&conveyorPID,conveyor_voltage, -17);

				inErrorMode 	= 1;
				motor_set_speed(CONVEYOR, conveyor_voltage, 1);
				/* Send signal to madeleine to make that NXT
				 * stop it feeders! */
				sendint_rs485(37708); //Stop Feeders
				#ifdef VERBOSE_ENABLED
					print_clear_line(2);
					print_str(0,2,"MODE = RESCUE");
					print_update();
				#endif
			}
			else {
				#ifdef VERBOSE_ENABLED
					print_clear_line(2);
					print_str(0,2,"MODE = NORMAL");
					print_update();
				#endif
			}
		}

		/* Detect if the current item is white, and enqueue it */
		if (receive > 150) {
			global_white_push++;
			dogkick = 1;
			newCandy.rpmTicksStamp 	= abs(nxt_motor_get_count(CONVEYOR));
			newCandy.color 			= WHITE;
			buffer_enqueue(&colorBufferWhite, newCandy);
		}
		/* Detect if the current item is blue, and enqueue it */
		if (receive <= 150) {
			global_blue_push++;
			dogkick = 1;
			newCandy.rpmTicksStamp 	= abs(nxt_motor_get_count(CONVEYOR));
			newCandy.color 			= BLUE;
			buffer_enqueue(&colorBufferBlue, newCandy);
		}

		print_clear_line(4);
		print_clear_line(5);
		print_str(0,3,"WHITE= ");
		display_int(global_white_push,0);
		print_str(0,4,"BLUE = ");
		display_int(global_blue_push,0);
		print_update();
	}
	#ifdef WCET_ANAL
		stopTimer(Stask_NETWORK_RECEIVE);
	#endif
	TerminateTask();
}


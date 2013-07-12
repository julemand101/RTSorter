#ifndef _WCET_H_
#define _WCET_H_

#include "kernel.h"
#include "kernel_id.h"
#include "ecrobot_interface.h"
#include "display.h"

#define WCET_ANAL

/*
 * 11 tasks,
 * first entry the timer start time.
 * second entry is the all time wcet of the task.
 * third is the total number of times task has been run
 */

int assertions[10] =
{
		0, //0: feeder.c:53 		: Feeder stops the opposite feeder
		0, //1: feeder.c:62 		: Stops own feeder (pause due to time limit)
		0, //2: feeder.c:77 		: Go-ness (normal feeder operation)
		0, //3: colorsensor.c:33	: Candy detected by color sensor
		0, //4: madeleine.c:309		: Candy with invalid color detected
		0, //5: madeleine.c:423		: Madeleine has leaved error mode
		0, //6: madeleine.c:433		: Madeleine has entered error mode
		0, //7: stephanie.c:239		: Pistons parallel returning
		0, //8: stephanie.c:242		: Pistons parallel pushing
		0  //9: stephanie.c:289		: Rescue mode entered (slow conveyor)
};

U32 taskWCET[11][3] =
	{
	 //MADELEINE
	 {0,0,0}, //0LightSensor
	 {0,0,0}, //1dequeuer
	 {0,0,0}, //2enqueuer_black
	 {0,0,0}, //3enqueuer_red
	 {0,0,0}, //4ErrorDetect
	 {0,0,0}, //5NETWORK_SEND,
	 {0,0,0}, //6NETWORK_RECEIVE,
	 //STEPHANIE
	 {0,0,0}, //7push
	 {0,0,0}, //8ErrorDetect
	 {0,0,0}, //9NETWORK_SEND,
	 {0,0,0}  //10NETWORK_RECEIVE
	};

typedef enum {
	Mtask_LIGHTSENSOR,
	Mtask_DEQUEUER,
	Mtask_ENQUEUER_BLACK,
	Mtask_ENQUEUER_RED,
	Mtask_ERRORDETECT,
	Mtask_NETWORK_SEND,
	Mtask_NETWORK_RECEIVE,
	Stask_PUSH,
	Stask_ERRORDETECT,
	Stask_NETWORK_SEND,
	Stask_NETWORK_RECEIVE
} taskName;

void startTimer(int taskNumber);
void stopTimer(int taskNumber);
void displayResultsForMadeleine();
void displayResultsForStephanie();
int areAllAssertionsTrue();
void assertionTrue(int id);
#endif

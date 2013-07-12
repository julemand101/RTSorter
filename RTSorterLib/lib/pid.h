#ifndef _PID_H_
#define _PID_H_

#include "kernel.h"
#include "kernel_id.h"
#include "ecrobot_interface.h"

struct PID {
	int 	e1;			//Error
	int 	e2;			//Error
	float 	P;			//Proportional term
	float 	I;			//Integral term
	float 	I_last;
	float 	D;			//Derivative term
	float 	D_last;
	float 	pv_last;	//Process value (the measured value)
	int		sp; 		//Setpoint (the desired value)
	int 	settle_time;
	int 	conveyor_actual_voltage;
};

int pid_update(struct PID* pid, int scanTime, int pv, float kp, float ti, float td, boolean inverse, int max);
void pid_initialize(struct PID* pid);
void pid_reset(struct PID* pid, int conveyor_actual_speed, int setpoint);

#endif

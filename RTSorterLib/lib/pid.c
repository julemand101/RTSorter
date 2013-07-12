/*
 ============================================================================
 Name        : pid.c
 Author      : S502A
 Version     :
 Description : Heavily customized version of PID-implementation by JKG's
 private contact, Lasse
 ============================================================================
 */

#include "pid.h"
#include "kernel.h"
#include "kernel_id.h"
#include "ecrobot_interface.h"
#include "math.h"

int pid_update(struct PID* pid, int scan_time, int pv, float kp, float ki, float kd, boolean inverse, int max) {
	pid->e1 = pid->e2;
	pid->I_last = pid->I;
	pid->D_last = pid->D;

	/* Compute error as difference between setpoint and process value */
	if(inverse){
		//Inverse
		pid->e2 = pid->sp - pv;
	} else {
		//Direct
		pid->e2 = pv - pid->sp;
	}

	/* Compute P */
	pid->P = pid->e2 * kp;
	if(pid->P > max){
		pid->P = max;
	} else if(pid->P < -max){
		pid->P = -max;
	}

	/* Compute I */
	pid->I = pid->I_last + ((pid->e2*scan_time) / (ki));
	if(pid->I > (max-(pid->P * kp))){
		pid->I =(max-(pid->P*kp));
	} else if(pid->I < -(max-(pid->P*kp))){
		pid->I = -(max-(pid->P*kp));
	}

	/* Compute D
	 * Use noise filter to disable the derivate element when error
	 * is less than some static value */
	if(abs(pv-pid->sp) > 3) {
		pid->D = (kd * (((float)pid->e2 - (float)pid->e1) / (float)scan_time));
		if(pid->D > max){
			pid->D = max;
		} else if(pid->D < -max){
			pid->D = -max;
		}
	}
	else {
		pid->D = 0;
	}

	pid->pv_last = pv;

	return pid->P + pid->I + pid->D;
}

void pid_initialize(struct PID* pid) {
	pid->e1 	 = 0;
	pid->e2 	 = 0;
	pid->P 		 = 0;
	pid->I 		 = 0;
	pid->I_last  = 0;
	pid->D 		 = 0;
	pid->D_last  = 0;
	pid->pv_last = 0;
	pid->sp		 = 0;
}

void pid_reset(struct PID* pid, int conveyor_actual_speed, int setpoint) {
	pid_initialize(pid);
	pid->sp = setpoint;
	pid->pv_last = setpoint;
	pid->settle_time = systick_get_ms();
	pid->conveyor_actual_voltage = conveyor_actual_speed;
}

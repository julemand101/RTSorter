#ifndef _MOTOR_H_
#define _MOTOR_H_

#include "kernel.h"
#include "kernel_id.h"
#include "ecrobot_interface.h"

void motor_rotate(U32 port, int degrees, int speed, int brake);
void motor_run_ms(U32 port, int ms, int speed, int brake);
void motor_set_speed(U32 n, int speed_percent, int brake);
void motor_set_count(U32 n, int count);
int  motor_get_count(U32 n);
void motor_rotate_async_update();
void motor_rotate_async(U32 port, int degrees, int speed);
void initialize_motors();

typedef enum {
	MOTOR_RIGHT,
	MOTOR_LEFT
} Direction;

typedef enum {
	MOTOR_DEACTIVATED,
	MOTOR_ACTIVATED,
	MOTOR_SAFE_MODE
} Status;

#endif

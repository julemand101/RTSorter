#include "motor.h"

int motor_A_degrees = 0;
int motor_A_running = 0;
int moto_A_running_left = 0;

int motor_B_degrees = 0;
int motor_B_running = 0;
int moto_B_running_left = 0;

int motor_C_degrees = 0;
int motor_C_running = 0;
int motor_C_running_left = 0;

int prevSafeModeTick = 0;

struct AsyncMotorInstruction
{
	U32 port;
	int status;
	int degrees;
	int direction;
};

struct AsyncMotorInstruction instructions[3];

void initialize_motors()
{
	struct AsyncMotorInstruction inst1;
	inst1.status = MOTOR_DEACTIVATED;

	struct AsyncMotorInstruction inst2;
	inst2.status = MOTOR_DEACTIVATED;

	struct AsyncMotorInstruction inst3;
	inst3.status = MOTOR_DEACTIVATED;

	instructions[0] = inst1;
	instructions[1] = inst2;
	instructions[2] = inst3;
}

void motor_rotate(U32 port, int degrees, int speed, int brake) {

	int _degrees = (nxt_motor_get_count(port) + degrees);

	if (_degrees > nxt_motor_get_count(port))
	{
		nxt_motor_set_speed(port,speed,brake);
		while (nxt_motor_get_count(port) < _degrees)
		{

			//busywaaaait
		}
	}
	else
	{
		nxt_motor_set_speed(port,-speed,brake);
		while (nxt_motor_get_count(port) > _degrees)
		{
			//busywaaaait
		}
	}
	nxt_motor_set_speed(port,0,brake);
}

void motor_rotate_async(U32 port, int degrees, int speed)
{
	//todo: somehow make a check to see wether the motor is done subtracting the piston.

	// Convert from relative degrees to absolute degrees
	int _degrees = (nxt_motor_get_count(port) + degrees);

	// Saves information aboute the motor instruction. The motor that is to be run (port) and how many degrees it should turn
	struct AsyncMotorInstruction instr;
	instr.port = port;
	instr.degrees = _degrees;

	// Which way should the motor turn?
	if (_degrees > nxt_motor_get_count(port))
	{
		nxt_motor_set_speed(port,speed, 1);
		instr.direction = MOTOR_RIGHT;
	}
	else
	{
		nxt_motor_set_speed(port,-speed, 1);
		instr.direction = MOTOR_LEFT;
	}

	switch (port) {
		case NXT_PORT_A:
		{
			instr.status = MOTOR_ACTIVATED;
			instructions[0] = instr;
		}
			break;
		case NXT_PORT_B:
		{
			instr.status = MOTOR_ACTIVATED;
			instructions[1] = instr;
		}
			break;
		case NXT_PORT_C:
		{
			instr.status = MOTOR_ACTIVATED;
			instructions[2] = instr;
		}
			break;
	}
}

void motor_rotate_async_update(int* A_stopped, int* B_stopped, int* C_stopped)
{
	// Loops through all instructions
	for (int i = 0; i < 3; i++)
	{
		struct AsyncMotorInstruction instr = instructions[i];

		// If the instruction is active (the motor is running)
		if (instr.status == MOTOR_ACTIVATED)
		{
			// If the motor is running right
			if (instr.direction == MOTOR_RIGHT)
			{
				// If the motor has reached its "destination"
				if (instr.degrees < nxt_motor_get_count(instr.port))
				{
					// Stop the motor
					nxt_motor_set_speed(instr.port, 0, 1);
					instructions[i].status = MOTOR_DEACTIVATED;

					// Log that the motor has stopped
					switch (instr.port) {
						case NXT_PORT_A:
							*A_stopped = 1;
							break;
						case NXT_PORT_B:
							*B_stopped = 1;
							break;
						case NXT_PORT_C:
							*C_stopped = 1;
							break;
					}
				}
			}
			// If the motor is running left
			else if (instr.direction == MOTOR_LEFT)
			{
				// If the motor has reached its "destination"
				if (instr.degrees > nxt_motor_get_count(instr.port))
				{
					// Stop the motor
					nxt_motor_set_speed(instr.port, 0, 1);
					instructions[i].status = MOTOR_DEACTIVATED;

					// Log that the motor has stopped
					if (instr.port == NXT_PORT_A)
						*A_stopped = 1;
					if (instr.port == NXT_PORT_B)
						*B_stopped = 1;
					if (instr.port == NXT_PORT_C)
						*C_stopped = 1;
				}
			}
		}
	}
}

void motor_run_ms(U32 port, int ms, int speed, int brake) {
	nxt_motor_set_speed(port,speed,brake);
	systick_wait_ms(ms);
	nxt_motor_set_speed(port,0,brake);
}

void motor_set_speed(U32 n, int speed_percent, int brake) {
	nxt_motor_set_speed(n, speed_percent, brake);
}

void motor_set_count(U32 n, int count) {
	nxt_motor_set_count(n,count);
}

int motor_get_count(U32 n) {
	return nxt_motor_get_count(n);
}

#include "wcet.h"

//Assertions
void assertionTrue(int id) {
	assertions[id] = 1;
}

int areAllAssertionsTrue() {
	int i;
	for (i=0 ; i < 10; i++) {
		if (assertions[i] == 0)
			return 0;
	}
	return 1;
}

//Place this at the start of every task.
void startTimer(int task)
{
	taskWCET[task][0] = systick_get_ms();
}

//Place this at the start of every task.
void stopTimer(int task)
{
	U32 executionTime = systick_get_ms() - taskWCET[task][0];
	taskWCET[task][2]++;
	if(executionTime > taskWCET[task][1])
	{
		taskWCET[task][1] = executionTime;
	}
}

void displayResultsForMadeleine()
{
	print_clear_display();
	for(int i = 0; i < 7; i++)
	{
		print_int(0,i,taskWCET[i][1]);
		print_int(3,i,taskWCET[i][2]);
	}
	for(int i=0; i < 7; i++) {
		print_int(12,i,assertions[i]);
	}
}

void displayResultsForStephanie()
{
	print_clear_display();
	for(int i = 7; i < 11; i++)
	{
		print_int(0,i-7,taskWCET[i][1]);
		print_int(3,i-7,taskWCET[i][2]);
	}
	for(int i=7; i < 10; i++) {
		print_int(12,i%7,assertions[i]);
	}

}

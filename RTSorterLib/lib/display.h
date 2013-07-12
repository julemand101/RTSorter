#ifndef _DISPALY_H_
#define _DISPLAY_H_

#include "kernel.h"
#include "kernel_id.h"
#include "ecrobot_interface.h"

void print_int(int x, int y, int value);
void print_str(int x, int y, char string[]);
void print_clear(int line, int start, int length);
void print_clear_line(int line);
void print_clear_display();
void print_update();

#endif

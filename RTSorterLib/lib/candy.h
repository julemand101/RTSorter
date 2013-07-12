#ifndef _CANDY_H_
#define _CANDY_H_

#include "kernel.h"
#include "kernel_id.h"
#include "ecrobot_interface.h"

typedef enum {
	WHITE,
	BLUE
} itemColor;

/* A candy  */
struct Candy {
	struct Feeder* feeder;		// only used by feeder module
	itemColor color;			// only used by color module
	int rpmTicksStamp;			// only used by color module
	int pushIt;					// only used by piston task
};

#endif

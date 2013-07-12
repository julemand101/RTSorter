#ifndef _BOUNDEDBUFFER_H_
#define _BOUNDEDBUFFER_H_

#include "kernel.h"
#include "kernel_id.h"
#include "ecrobot_interface.h"
#include "candy.h"

#define BUFFER_SIZE 8

struct BoundedBuffer {
	int read;
	int write;
	struct Candy *buffer;
	int size;
	char* prettyname;
};

void buffer_initialize(struct BoundedBuffer* bb, struct Candy *candyArray, int arraySize, char* prettyname);
int buffer_enqueue(struct BoundedBuffer*, struct Candy);
int buffer_count_elements(struct BoundedBuffer*);
struct Candy* buffer_dequeue(struct BoundedBuffer*);
struct Candy* buffer_peek(int, struct BoundedBuffer*);

#endif

#include "bounded-buffer.h"
#include "math.h"
#include "errordetect.h"

/* ENQUEUES detected candy to bouded-buffer  */
int buffer_enqueue(struct BoundedBuffer* bb, struct Candy candy) {

	/* If the write pointer, plus one, equals the read pointer,
	 * then the buffer is full. */
	if(((bb->write + 1) % bb->size) == bb->read)
		emergency_stop("Buffer full:",bb->prettyname);//instead of return -1;

	/* Buffer is initialized to contain a finit amount of candy.
	 * Therefore, we alter the value of the current candy pointed
	 * to by the write-head of the buffer. */
	bb->buffer[bb->write] = candy;

	/* Make the buffer circular */
	bb->write= (bb->write + 1) % bb->size;

	return 0;
}

/* DEQUEUES candy from bounded-buffer */
struct Candy* buffer_dequeue(struct BoundedBuffer* bb) {

	/* If both pointers are pointing at the same location,
	 * the buffer is empty. */
	if(bb->read == bb->write)
		return null;

	/* Get the oldest element in the buffer */
	struct Candy* c = &bb->buffer[bb->read];

	/* Increment the read-head, to point the
	 * next element in the buffer. */
	bb->read = (bb->read + 1) % bb->size;

	return c;
}

/* PEEK candy from bounded-buffer */
struct Candy* buffer_peek(int loc, struct BoundedBuffer* bb) {

	/* Compute the position to peek */
	int pos = (((bb->read + loc)) % bb->size);

	/* Check if the peek pos is valid */
	if(pos == bb->write || loc > abs(bb->size-2))
		return null;
	/* If the write head pos is greater than the read head pos,
	 * this means that an element of the buffer only can be
	 * returned if the peek-pos is less than the pos of the
	 * write head. */
	else if(bb->write > bb->read) {
		if(pos < bb->write)
			return &bb->buffer[pos];
		else {
			return null;
		}
	}
	/* If the pos of the write head is less than the pos of the
	 * read head, this means that an element of the buffer only
	 * can be returned if the peek-pos is greater than or equal
	 * to the pos of the read head OR if the peek-pos is less
	 * than the pos of the write head. */
	else if(bb->write < bb->read) {
		if(pos >= bb->read || pos < bb->write)
			return &bb->buffer[pos];
		else
			return null;
	}
	else {
		return null;
	}
}

/* Return no. of elements in bounded buffer */
int buffer_count_elements(struct BoundedBuffer* bb) {
	if(bb->read == bb->write)
		return 0;
	else if(bb->read < bb->write)
		return bb->write - bb->read;
	else
		return bb->size-bb->read + bb->write;
}

/* Initialize bounded buffer */
void buffer_initialize(struct BoundedBuffer* bb, struct Candy *candyArray, int arraySize, char* prettyname) {
	bb->read 	= 0;	// points to the current element to read
	bb->write 	= 0;	// points to the next free slot that can be written
	bb->buffer = candyArray;
	bb->size = arraySize;
	bb->prettyname = prettyname;
}

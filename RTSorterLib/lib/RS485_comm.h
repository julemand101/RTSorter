#ifndef _RS485_H_
#define _RS485_H_

#include "kernel.h"
#include "kernel_id.h"
#include "ecrobot_interface.h"

struct BoundedBuffer sendBuffer; //todo: har fjernet static, for at kunne tilgå sendbufferen fra main. Det er nødvendigt mht. token. Fix det senere :)

void crap_send();
void crap_recieve();
int recieveint_rs485(int *value);
void sendint_rs485(int value);
void send_buffered_ints_rs485();
// Initializes the connection (must be done on both nxt's. Should be called on program start.
void initialize_rs485();
#endif

#include "kernel.h"
#include "kernel_id.h"
#include "ecrobot_interface.h"
#include "RS485_comm.h"
#include "crc.h"
#include "candy.h"
#include "bounded-buffer.h"
#include "display.h"

#define INT_SIZE 4
#define NETWORK_SPEED 9216

void initialize_rs485()
{
	static struct Candy sendBufferArray[64];
	buffer_initialize(&sendBuffer, sendBufferArray, 64, "networkPackets");
	ecrobot_init_rs485(NETWORK_SPEED);
}

U8 raw[INT_SIZE + 1];
int totalBytesRead = 0;
int crc_errors = 0;

void crap_recieve()
{
	U8 crap_buf[5] = {0,0,0,0,0};
	int crapBytes = 0;
	crapBytes = 0;
	while (crapBytes != 5)
	{
		crapBytes += ecrobot_read_rs485(crap_buf, crapBytes, 5 - crapBytes);
	}

	ecrobot_read_rs485(crap_buf, crapBytes, 1024);
	systick_wait_ms(5);
	ecrobot_read_rs485(crap_buf, crapBytes, 1024);
}

void crap_send()
{
	systick_wait_ms(50);
	U8 crap[5] = {0,0,0,0,0};

	ecrobot_send_rs485(crap, 0, 5);
	systick_wait_ms(50);
}

int recieveint_rs485(int *value)
{
	totalBytesRead += ecrobot_read_rs485(raw, totalBytesRead, (INT_SIZE+1) - totalBytesRead);
	if (totalBytesRead != (INT_SIZE+1))
		return 0;

	crc_t crcRecieved = (crc_t)raw[0];

	U8 rawInt[INT_SIZE];
	rawInt[0] = raw[1];
	rawInt[1] = raw[2];
	rawInt[2] = raw[3];
	rawInt[3] = raw[4];

	crc_t crc;
    crc = crc_init();
    crc = crc_update(crc, (unsigned char *)rawInt, INT_SIZE);
    crc = crc_finalize(crc);

    if (crc != crcRecieved)
    {
    	print_str(0,0,"CRC errors = ");
    	display_int(++crc_errors, 0);
    	display_update();

    	//We reboot the network interface
    	ecrobot_term_rs485();
    	ecrobot_init_rs485(NETWORK_SPEED);

    	totalBytesRead = 0;
    	return 0;
    }

    // We convert the raw bytes to an int
    *value = 0;
    for (int i = INT_SIZE-1; i > -1 ; i--)
    {
       int temp = 0;
       temp += rawInt[i];
       temp <<= (8 * i);

       *value += temp;
    }


	totalBytesRead = 0;

//	print_clear_line(5);
//	display_update();
//	display_goto_xy(0,5);
//	display_int(*value, 0);
//	display_update();

	return 1;
}

void sendint_rs485(int value)
{
	struct Candy candy;
	candy.rpmTicksStamp = value;
	buffer_enqueue(&sendBuffer, candy);
}

void send_buffered_ints_rs485() {
	if (buffer_count_elements(&sendBuffer) > 0) {
		struct Candy* candy = buffer_dequeue(&sendBuffer);
		// We convert the int to an array of 4 bytes

		int value = candy->rpmTicksStamp;

		U8 buffer[INT_SIZE];
		buffer[0] = (U8)value;

		value >>= 8;
		buffer[1] = (U8)value;

		value >>= 8;
		buffer[2] = (U8)value;

		value >>= 8;
		buffer[3] = (U8)value;

		// We calculate the checksum for the 4 bytes
		crc_t crc;
		crc = crc_init();
		crc = crc_update(crc, (unsigned char *)buffer, INT_SIZE);
		crc = crc_finalize(crc);

		// We pack all the bytes in one array and sends it
		U8 toSend[INT_SIZE + 1];

		toSend[0] = (U8)crc;
		toSend[1] = buffer[0];
		toSend[2] = buffer[1];
		toSend[3] = buffer[2];
		toSend[4] = buffer[3];

		ecrobot_send_rs485(toSend, 0, INT_SIZE+1);
	}
}

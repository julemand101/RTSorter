/**
 * \file /tmp/crc.c
 * Functions and types for CRC checks.
 *
 * Generated on Wed Oct 27 10:43:05 2010,
 * by pycrc v0.7.6, http://www.tty1.net/pycrc/
 * using the configuration:
 *    Width        = 8
 *    Poly         = 0x07
 *    XorIn        = 0x00
 *    ReflectIn    = False
 *    XorOut       = 0x00
 *    ReflectOut   = False
 *    Algorithm    = table-driven
 *****************************************************************************/
#include "crc.h"

/**
 * Static table used for the table_driven implementation.
 *****************************************************************************/
static const crc_t crc_table[16] = {
    0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15, 0x38, 0x3f, 0x36, 0x31, 0x24, 0x23, 0x2a, 0x2d
};



/**
 * Update the crc value with new data.
 *
 * \param crc      The current crc value.
 * \param data     Pointer to a buffer of \a data_len bytes.
 * \param data_len Number of bytes in the \a data buffer.
 * \return         The updated crc value.
 *****************************************************************************/
crc_t crc_update(crc_t crc, const unsigned char *data, U32 data_len)
{
    unsigned int tbl_idx;

    while (data_len--) {
        tbl_idx = (crc >> 4) ^ (*data >> 4);
        crc = crc_table[tbl_idx & 0x0f] ^ (crc << 4);
        tbl_idx = (crc >> 4) ^ (*data >> 0);
        crc = crc_table[tbl_idx & 0x0f] ^ (crc << 4);

        data++;
    }
    return crc & 0xff;
}




#ifndef _SERIAL_H_
#define _SERIAL_H_

#include <stdbool.h>

#include "avr_mcu.h"

void serial_init();
bool serial_has_next_byte();
unsigned char serial_read_next_byte();
void serial_write(const unsigned char* data, short len);
void serial_write_newline();
void serial_tx_byte(unsigned char data);

#ifdef SERIAL_EXTRA_SUPPORT
void serial_extra_start();
void serial_extra_stop();
bool serial_extra_has_next_byte();
unsigned char serial_extra_read_next_byte();
void serial_extra_tx_byte(unsigned char data);
#endif // SERIAL_EXTRA_SUPPORT

#endif // _SERIAL_H_

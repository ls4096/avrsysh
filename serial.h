#ifndef _SERIAL_H_
#define _SERIAL_H_

#include <stdbool.h>

void serial_init();
bool serial_has_next_byte();
unsigned char serial_read_next_byte();
void serial_write(unsigned char* data, short len);

#endif // _SERIAL_H_

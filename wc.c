#include <stdio.h>
#include <string.h>

#include "wc.h"

#include "serial.h"

void wc_main(void* arg)
{
	unsigned short l = 0;
	unsigned short w = 0;
	unsigned short c = 0;

	bool run = true;
	while (run)
	{
		unsigned char d = serial_read_next_byte();
		switch (d)
		{
		case 0x03:
			return;
		case 0x04:
			run = false;
			break;
		case '\n':
			l++;
		case ' ':
			w++;
		default:
			c++;
		}
	}

	char buf[20];
	sprintf(buf, "%6u%6u%6u", l, w, c);
	serial_write(buf, strlen(buf));
	serial_write_newline();
}

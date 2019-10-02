#include "serial_proxy.h"

#ifdef SERIAL_EXTRA_SUPPORT

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "pm.h"
#include "serial.h"

static const char* START_MSG = "Ctrl+G to stop";

void serialproxy()
{
	serial_write(START_MSG, strlen(START_MSG));
	serial_write_newline();
	serial_write_newline();

	serial_extra_start();

	uint16_t bytes_up = 0;
	uint16_t bytes_down = 0;

	unsigned char c;
	while (1)
	{
		while (!serial_has_next_byte() && !serial_extra_has_next_byte())
		{
			pm_yield();
		}

		while (serial_has_next_byte())
		{
			c = serial_read_next_byte();
			if (c == 0x07)
			{
				goto stop;
			}
			serial_extra_tx_byte(c);
			bytes_up++;
		}

		while (serial_extra_has_next_byte())
		{
			c = serial_extra_read_next_byte();
			serial_tx_byte(c);
			bytes_down++;
		}
	}

stop:
	serial_extra_stop();

	char buf[32];
	sprintf(buf, "bytes: %u up, %u down", bytes_up, bytes_down);
	serial_write(buf, strlen(buf));
	serial_write_newline();
}

#endif // SERIAL_EXTRA_SUPPORT

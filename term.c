#include <stdio.h>
#include <string.h>

#include "term.h"

#include "serial.h"

void term_set_cursor(bool enable)
{
	serial_write("\e[?25", 5);

	if (enable)
	{
		serial_tx_byte('h');
	}
	else
	{
		serial_tx_byte('l');
	}
}

void term_cursor_home()
{
	serial_write("\e[H", 3);
}

void term_move_cursor(short x, short y)
{
	char buf[12];
	sprintf(buf, "\e[%u;%uH", y, x);
	serial_write(buf, strlen(buf));
}

void term_clear_screen()
{
	serial_write("\e[2J\e[H", 7);
}

#include "game.h"

#include "serial.h"
#include "term.h"

void game_draw_vertical(short x, short y, short h, char c)
{
	term_move_cursor(x, y);

	for (short i = 0; i < h; i++)
	{
		if (i != 0)
		{
			serial_write("\e[B\e[D", 6);
		}
		serial_tx_byte(c);
	}
}

void game_draw_horizontal(short x, short y, short l, char c)
{
	term_move_cursor(x, y);

	for (short i = 0; i < l; i++)
	{
		serial_tx_byte(c);
	}
}

void game_draw_border(short w, short h, short c)
{
	char buf[8];
	for (short i = 0; i < h; i++)
	{
		if (i == 0 || i == h - 1)
		{
			for (short j = 0; j < w; j++)
			{
				serial_tx_byte(c);
			}
		}
		else
		{
			serial_tx_byte(c);
			sprintf(buf, "\e[%uC", w - 2);
			serial_write(buf, strlen(buf));
			serial_tx_byte(c);
		}
		serial_write_newline();
	}
}

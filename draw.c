#include <stdio.h>
#include <string.h>

#include "draw.h"

#include "serial.h"
#include "term.h"

#define SPACE_CHAR ' '


static void set_term_bg(draw_bg_setting bg);


void draw_vertical(short x, short y, short h, char c)
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

void draw_horizontal(short x, short y, short l, char c)
{
	term_move_cursor(x, y);

	for (short i = 0; i < l; i++)
	{
		serial_tx_byte(c);
	}
}

void draw_border(short w, short h, short c)
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

void draw_vertical_bg(short x, short y, short h, draw_bg_setting bg)
{
	set_term_bg(bg);
	draw_vertical(x, y, h, SPACE_CHAR);
	set_term_bg(DRAW_BG_RESET);
}

void draw_horizontal_bg(short x, short y, short l, draw_bg_setting bg)
{
	set_term_bg(bg);
	draw_horizontal(x, y, l, SPACE_CHAR);
	set_term_bg(DRAW_BG_RESET);
}


static void set_term_bg(draw_bg_setting bg)
{
	unsigned short bg_code;
	switch (bg)
	{
	case DRAW_BG_RED:
		bg_code = 41;
		break;
	case DRAW_BG_GREEN:
		bg_code = 42;
		break;
	case DRAW_BG_BLUE:
		bg_code = 44;
		break;
	case DRAW_BG_RESET:
	default:
		bg_code = 0;
		break;
	}

	char buf[8];
	sprintf(buf, "\e[%um", bg_code);
	serial_write(buf, strlen(buf));
}

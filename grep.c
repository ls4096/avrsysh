#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "grep.h"
#include "serial.h"

static bool parse_args(const char* str, char* s);
static bool is_match(const char* line, const char* search);

void grep_main(void* arg)
{
	const char* str = (const char*) arg;
	unsigned char search[32];

	if (!parse_args(str, search))
	{
		char buf[9];
		sprintf(buf, "bad args");
		serial_write(buf, strlen(buf));
		serial_write_newline();

		return;
	}

	unsigned char line[64];
	short i = 0;

	while (true)
	{
		unsigned char c = serial_read_next_byte();
		if (c == 0x03 || c == 0x04)
		{
			break;
		}
		else if (c == 0x0a)
		{
			continue;
		}

		line[i++] = c;

		if (c == 0x0d)
		{
			line[i - 1] = 0x00;
			if (is_match(line, search))
			{
				serial_write(line, i);
				serial_write_newline();
			}

			i = 0;
		}
	}
}

static bool parse_args(const char* str, char* s)
{
	str += 4; // Skip the "grep" command at the beginning.

	// Next char must be a space.
	if (*str != ' ')
	{
		return false;
	}

	// Skip all remaining spaces.
	while ((*str) == ' ')
	{
		str++;
	}

	// Next char must not be null terminator.
	if (*str == 0x00)
	{
		return false;
	}

	strcpy(s, str);

	return true;
}

static bool is_match(const char* line, const char* search)
{
	short lp = 0;
	short sp = 0;

	while (line[lp] != 0x00)
	{
		if (search[sp] == 0x00)
		{
			return true;
		}

		if (line[lp] == search[sp])
		{
			sp++;
		}
		else
		{
			sp = 0;
		}

		lp++;
	}

	if (search[sp] == 0x00)
	{
		return true;
	}

	return false;
}

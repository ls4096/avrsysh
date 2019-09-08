#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "seq.h"
#include "serial.h"
#include "util.h"

static bool parse_args(const char* str, unsigned short* a, unsigned short* b);

void seq_main(const char* str)
{
	unsigned short a;
	unsigned short b;

	if (!parse_args(str, &a, &b))
	{
		char buf[9];
		sprintf(buf, "bad args");
		serial_write(buf, strlen(buf));
		serial_write_newline();

		return;
	}

	unsigned short i;
	char buf[8];
	for (i = a; i <= b; i++)
	{
		sprintf(buf, "%u", i);
		serial_write(buf, strlen(buf));
		serial_write_newline();
	}
}

static bool parse_args(const char* str, unsigned short* a, unsigned short* b)
{
	char c;

	*a = 0;
	*b = 0;

	// Keep going until we get a numeric char.
	while (!util_is_numeric(c = *str))
	{
		if (c == 0x00)
		{
			return false;
		}

		str++;
	}

	// Parse "a".
	while (util_is_numeric(c = *str))
	{
		*a *= 10;
		*a += (c - 0x30);

		str++;
	}

	// Expecting space char next.
	if (c != ' ')
	{
		return false;
	}

	// Keep going until we run out of spaces.
	while ((c = *str) == ' ')
	{
		str++;
	}

	// Expecting numeric char next.
	if (!util_is_numeric(c))
	{
		return false;
	}

	// Parse "b".
	while (util_is_numeric(c = *str))
	{
		*b *= 10;
		*b += (c - 0x30);

		str++;
	}

	return true;
}

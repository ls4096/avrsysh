#include <avr/io.h>

#include "pm.h"


#define WAKE_TRACK_VALUE_COUNT 16
static volatile unsigned char _wake_counter[WAKE_TRACK_VALUE_COUNT];
static volatile unsigned char _wake_pos = 0;


void pm_reset()
{
	SMCR = 0;
}

void pm_yield()
{
	SMCR = 1;
	asm volatile ( "sleep" );
	SMCR = 0;
}

void pm_update_wake_counter(unsigned char c)
{
	_wake_counter[_wake_pos++] = c;
	_wake_pos &= 0x0f;
}

void pm_get_wake_count(unsigned short* w)
{
	unsigned short s = 0;

	unsigned char i;
	for (i = 0; i < WAKE_TRACK_VALUE_COUNT; i++)
	{
		s += _wake_counter[i];
	}

	w[0] = s;
	w[1] = 0x1000 - 0x0010;
}

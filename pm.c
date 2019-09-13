#include <avr/io.h>

#include "pm.h"

#include "thread.h"


#define WAKE_TRACK_VALUE_COUNT 16
static volatile unsigned char _wake_counter[WAKE_TRACK_VALUE_COUNT];
static volatile unsigned char _wake_pos = 0;


static void idle_cpu();


void pm_reset()
{
	SMCR = 0;
}

void pm_yield()
{
	if (thread_is_running() && thread_which_is_running() != -2)
	{
		// TODO: Even if the other thread exists, it may not have anything to do either, so we might end up just busy-looping and switching threads when we should be sleeping instead.
		thread_switch();
	}
	else
	{
		idle_cpu();
	}
}

void pm_update_wake_counter(unsigned char c)
{
	_wake_counter[_wake_pos++] = c;
	_wake_pos &= 0x0f;
}

void pm_get_wake_count(unsigned short w[2])
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


static void idle_cpu()
{
	SMCR = 1;
	asm volatile ( "sleep" );
	SMCR = 0;
}

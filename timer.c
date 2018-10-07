#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>

#include "sp_mon.h"
#include "timer.h"
#include "pm.h"


static volatile unsigned short _t[2] = { 0, 0 };
static volatile bool _t_updated = false;

static volatile unsigned char _sleep_counter[2] = { 0, 0 };

#define NOTIFY_COUNT_LIMIT 4
static volatile timer_notify_t* _notify_items[NOTIFY_COUNT_LIMIT];


ISR(TIMER1_OVF_vect)
{
	_t[1]++;
	if (_t[1] == 0)
	{
		_t[0]++;
	}
	_t_updated = true;

	_sleep_counter[0]++;
	_sleep_counter[1] += (SMCR & 0x01);
	if (_sleep_counter[0] == 0xff)
	{
		pm_update_wake_counter(((unsigned char)0xff) - _sleep_counter[1]);
		_sleep_counter[0] = 0;
		_sleep_counter[1] = 0;
	}

	sp_mon_check();

	short i;
	for (i = 0; i < NOTIFY_COUNT_LIMIT; i++)
	{
		if (_notify_items[i] != 0)
		{
			if (timer_compare(_notify_items[i]->t, _t) < 0)
			{
				_notify_items[i]->notify = true;
				_notify_items[i] = 0;
			}
		}
	}
}


void timer_init()
{
	// TODO: Change to 128 ticks per second. Instead of overflow, we should use a compare to a specific value.
	PRR &= ~(1 << PRTIM1);
	TCCR1B |= (1 << CS10);
	TIFR1 |= (1 << TOV1);
	TIMSK1 |= (1 << TOIE1);

	short i;
	for (i = 0; i < NOTIFY_COUNT_LIMIT; i++)
	{
		_notify_items[i] = 0;
	}

	pm_reset();
}

void timer_get_tick_count(unsigned short* t)
{
	while (_t_updated)
	{
		_t_updated = false;

		t[0] = _t[0];
		t[1] = _t[1];
	}
}

short timer_compare(unsigned short* t0, unsigned short* t1)
{
	if (t0[0] > t1[0])
	{
		return 1;
	}

	if (t0[0] < t1[0])
	{
		return -1;
	}

	if (t0[1] > t1[1])
	{
		return 1;
	}

	if (t0[1] < t1[1])
	{
		return -1;
	}

	return 0;
}

void timer_add_seconds(unsigned short* t, unsigned short seconds)
{
	const unsigned short t1 = t[1];

	t[0] += (seconds / TIMER_SECONDS_PER_UPPER_TICK);
	t[1] += ((seconds % TIMER_SECONDS_PER_UPPER_TICK) * TIMER_TICKS_PER_SECOND);

	if (t[1] < t1)
	{
		t[0]++;
	}
}

// Assumes t0 > t1.
unsigned short timer_get_diff_seconds(unsigned short* t0, unsigned short* t1)
{
	unsigned short diff = 0;

	diff += ((t0[0] - t1[0]) * TIMER_SECONDS_PER_UPPER_TICK);
	if (t0[1] >= t1[1])
	{
		diff += ((t0[1] - t1[1]) / TIMER_TICKS_PER_SECOND);
	}
	else
	{
		diff -= ((t1[1] - t0[1]) / TIMER_TICKS_PER_SECOND);
	}

	return diff;
}

bool timer_notify_register(timer_notify_t* tn)
{
	short i;
	for (i = 0; i < NOTIFY_COUNT_LIMIT; i++)
	{
		if (_notify_items[i] == 0)
		{
			tn->notify = false;
			_notify_items[i] = tn;

			return true;
		}
	}

	return false;
}

unsigned short timer_get_notify_registered_count()
{
	unsigned short n = 0;

	short i;
	for (i = 0; i < NOTIFY_COUNT_LIMIT; i++)
	{
		if (_notify_items[i] != 0)
		{
			n++;
		}
	}

	return n;
}

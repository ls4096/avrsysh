#ifndef _TIMER_H_
#define _TIMER_H_

#ifndef F_CPU
	#error F_CPU not defined
#endif

#define TIMER_SYSTEM_CLKS_PER_TICK 65536
#define TIMER_TICKS_PER_SECOND (F_CPU / TIMER_SYSTEM_CLKS_PER_TICK)
#define TIMER_SECONDS_PER_UPPER_TICK (65536 / TIMER_TICKS_PER_SECOND)


void timer_init();
void timer_get_tick_count(unsigned short* t);
unsigned char timer_get_tick_count_lsbyte();
short timer_compare(unsigned short* t0, unsigned short* t1);
void timer_add_seconds(unsigned short* t0, unsigned short seconds);
unsigned short timer_get_diff_seconds(unsigned short* t0, unsigned short* t1);


typedef struct {
	unsigned short t[2];
	bool notify;
} timer_notify_t;

bool timer_notify_register(timer_notify_t* tn);
unsigned short timer_get_notify_registered_count();


#endif // _TIMER_H_

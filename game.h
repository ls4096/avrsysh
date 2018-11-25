#ifndef _GAME_H_
#define _GAME_H_

#include "pm.h"
#include "serial.h"
#include "timer.h"


typedef struct
{
	unsigned short next_frame_time[2];
	unsigned short game_frame_ticks;
} game_context_t;


static void game_context_init(game_context_t* ctx, unsigned short game_fps);
static bool game_should_draw_frame(game_context_t* ctx);
static unsigned char game_get_char(game_context_t* ctx, bool block);
static void game_add_frame_time(game_context_t* ctx);


static void game_context_init(game_context_t* ctx, unsigned short game_fps)
{
	ctx->next_frame_time[0] = 0;
	ctx->next_frame_time[1] = 0;

	ctx->game_frame_ticks = TIMER_TICKS_PER_SECOND / game_fps;
}

static bool game_should_draw_frame(game_context_t* ctx)
{
	if (ctx->next_frame_time[0] == 0 && ctx->next_frame_time[1] == 0)
	{
		timer_get_tick_count(ctx->next_frame_time);
		game_add_frame_time(ctx);
		return true;
	}

	unsigned short t[2];
	timer_get_tick_count(t);
	if (timer_compare(t, ctx->next_frame_time) >= 0)
	{
		ctx->next_frame_time[0] = t[0];
		ctx->next_frame_time[1] = t[1];
		game_add_frame_time(ctx);
		return true;
	}

	return false;
}

static unsigned char game_get_char(game_context_t* ctx, bool block)
{
	if (!block)
	{
		unsigned short t[2];
		timer_get_tick_count(t);
		if (timer_compare(t, ctx->next_frame_time) >= 0)
		{
			return 0;
		}
		else
		{
			timer_notify_t tn;

			tn.t[0] = ctx->next_frame_time[0];
			tn.t[1] = ctx->next_frame_time[1];
			tn.notify = false;

			timer_notify_register(&tn);
			while (!tn.notify)
			{
				pm_yield();

				if (serial_has_next_byte())
				{
					return serial_read_next_byte();
				}
			}

			return 0;
		}
	}

	return serial_read_next_byte();
}

static void game_add_frame_time(game_context_t* ctx)
{
	ctx->next_frame_time[1] += ctx->game_frame_ticks;
	if (ctx->next_frame_time[1] < ctx->game_frame_ticks)
	{
		ctx->next_frame_time[0]++;
	}
}

#endif // _GAME_H_

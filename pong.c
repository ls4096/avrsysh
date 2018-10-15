#include <stdbool.h>

#include "pong.h"

#include "pm.h"
#include "rng.h"
#include "serial.h"
#include "timer.h"


#define PONG_WIDTH 80
#define PONG_HEIGHT 20

#define WALL_C '#'
#define PADDLE_C '|'
#define BALL_C '+'
#define SPACE_C ' '
#define CLEAR_C SPACE_C
#define RESUME_C SPACE_C
#define QUIT_C 'Q'

#define PADDLE_H (3)
#define PADDLE_Y_INIT (PONG_HEIGHT / 2 - 1)

#define PADDLE0_X (2)
#define PADDLE1_X (PONG_WIDTH - 1)

#define BALL_X_INIT_C (PONG_WIDTH / 2 - 1)
#define BALL_Y_INIT_C (PONG_HEIGHT / 2 - 1)

#define SCORE_POS_X (PONG_WIDTH / 2 - 5)
#define SCORE_POS_Y (PONG_HEIGHT + 2)

#define GAME_FPS 24
#define GAME_FRAME_TICKS (TIMER_TICKS_PER_SECOND / GAME_FPS)


typedef struct
{
	short y;
	short y_prev;
} paddle_t;

typedef struct
{
	// All ball values are in 1/256th units.
	short x;
	short y;
	short vx;
	short vy;
} ball_t;


static void move_cursor(short x, short y);
static void draw_vertical(short x, short y, short h, char c);
static void paddle_up(paddle_t* p);
static void paddle_down(paddle_t* p);
static void update_ball(ball_t* b);
static short check_ball_position(ball_t* b, paddle_t* p0, paddle_t* p1);
static void draw_frame(paddle_t* p0, paddle_t* p1, ball_t* ball, unsigned short* scores);
static bool should_draw_frame();
static void add_frame_time(unsigned short* t);
static unsigned char get_char(bool block);
static short ball_c2v(short c);
static short ball_v2c(short v);


static unsigned short _next_frame_time[2] = { 0, 0 };


void pong_init()
{
	// Disable cursor.
	serial_write("\e[?25l", 6);

	// Clear screen.
	serial_write("\e[2J\e[H", 7);

	// Draw walls.
	short i, j;
	char buf[16];
	for (i = 1; i <= PONG_HEIGHT; i++)
	{
		if (i == 1 || i == PONG_HEIGHT)
		{
			for (j = 1; j <= PONG_WIDTH; j++)
			{
				serial_tx_byte(WALL_C);
			}
		}
		else
		{
			serial_tx_byte(WALL_C);
			sprintf(buf, "\e[%uC", PONG_WIDTH - 2);
			serial_write(buf, strlen(buf));
			serial_tx_byte(WALL_C);
		}
		sprintf(buf, "\r\n", 2);
		serial_write(buf, strlen(buf));
	}

	// Draw paddles.
	draw_vertical(PADDLE0_X, PADDLE_Y_INIT, PADDLE_H, PADDLE_C);
	draw_vertical(PADDLE1_X, PADDLE_Y_INIT, PADDLE_H, PADDLE_C);

	// Draw ball.
	draw_vertical(BALL_X_INIT_C, BALL_Y_INIT_C, 1, BALL_C);

	// Return cursor to home.
	serial_write("\e[H", 3);
}

void pong_start()
{
	paddle_t p0 = { PADDLE_Y_INIT, PADDLE_Y_INIT };
	paddle_t p1 = { PADDLE_Y_INIT, PADDLE_Y_INIT };

	// Randomize ball starting direction.
	short ball_vx = 1 << 8;
	short ball_vy = 1 << 7;
	short r = rng_rand();
	if (r & 0x0001)
	{
		ball_vx = -ball_vx;
	}
	if (r & 0x0002)
	{
		ball_vy = -ball_vy;
	}

	ball_t ball = { ball_c2v(BALL_X_INIT_C), ball_c2v(BALL_Y_INIT_C), ball_vx, ball_vy };

	unsigned short scores[2] = { 0, 0 };

	enum { NORMAL, ESC, MOVE_P1 } s = NORMAL;

	unsigned char c = get_char(true);
	short check = 0;
	while (1)
	{
		if (s == NORMAL && c == QUIT_C)
		{
			break;
		}

		switch (s)
		{
		case NORMAL:
			if (c == 0x1b)
			{
				s = ESC;
			}
			else if (c == 'w')
			{
				paddle_up(&p0);
			}
			else if (c == 's')
			{
				paddle_down(&p0);
			}
			break;

		case ESC:
			if (c == '[')
			{
				s = MOVE_P1;
			}
			else
			{
				s = NORMAL;
			}
			break;

		case MOVE_P1:
			if (c == 'A')
			{
				paddle_up(&p1);
			}
			else if (c == 'B')
			{
				paddle_down(&p1);
			}

			s = NORMAL;
			break;
		}

		if (should_draw_frame())
		{
			update_ball(&ball);

			check = check_ball_position(&ball, &p0, &p1);
			if (check < 0)
			{
				scores[0] -= check;
			}
			else if (check > 0)
			{
				scores[1] += check;
			}

			draw_frame(&p0, &p1, &ball, check == 0 ? 0 : scores);
		}

		do {
			c = get_char(check != 0);
		} while (check != 0 && c != RESUME_C && c != QUIT_C);
	}

	// Clear screen.
	serial_write("\e[2J\e[H", 7);

	// Re-enable cursor.
	serial_write("\e[?25h", 6);
}


static void draw_vertical(short x, short y, short h, char c)
{
	move_cursor(x, y);

	for (short i = 0; i < h; i++)
	{
		if (i != 0)
		{
			serial_write("\e[B\e[D", 6);
		}
		serial_tx_byte(c);
	}
}

static void paddle_up(paddle_t* p)
{
	if (p->y <= 2)
	{
		return;
	}

	p->y--;
}

static void paddle_down(paddle_t* p)
{
	if (p->y >= PONG_HEIGHT - PADDLE_H)
	{
		return;
	}

	p->y++;
}

static void update_ball(ball_t* b)
{
	if ((b->vx > 0 && ball_v2c(b->x) >= PONG_WIDTH - 1) || ((b->vx < 0 && ball_v2c(b->x) <= 2)))
	{
		b->vx = -b->vx;
	}

	if ((b->vy > 0 && ball_v2c(b->y) >= PONG_HEIGHT - 1) || ((b->vy < 0 && ball_v2c(b->y) <= 2)))
	{
		b->vy = -b->vy;
	}

	b->x += b->vx;
	b->y += b->vy;
}

static short check_ball_position(ball_t* b, paddle_t* p0, paddle_t* p1)
{
	short check = 0;

	if (ball_v2c(b->x) == PADDLE0_X)
	{
		if (p0->y > ball_v2c(b->y) || p0->y < ball_v2c(b->y) - PADDLE_H)
		{
			// Point for P1.
			check = 1;
		}
	}
	else if (ball_v2c(b->x) == PADDLE1_X)
	{
		if (p1->y > ball_v2c(b->y) || p1->y < ball_v2c(b->y) - PADDLE_H)
		{
			// Point for P0.
			check = -1;
		}
	}

	return check;
}

static void draw_frame(paddle_t* p0, paddle_t* p1, ball_t* ball, unsigned short* scores)
{
	// Erase old paddles.
	draw_vertical(PADDLE0_X, p0->y_prev, PADDLE_H, CLEAR_C);
	draw_vertical(PADDLE1_X, p1->y_prev, PADDLE_H, CLEAR_C);

	// Draw new paddles.
	draw_vertical(PADDLE0_X, p0->y, PADDLE_H, PADDLE_C);
	draw_vertical(PADDLE1_X, p1->y, PADDLE_H, PADDLE_C);

	// Erase and draw new ball position.
	draw_vertical(ball_v2c(ball->x - ball->vx), ball_v2c(ball->y - ball->vy), 1, CLEAR_C);
	draw_vertical(ball_v2c(ball->x), ball_v2c(ball->y), 1, BALL_C);

	// Update "previous" paddle y values.
	p0->y_prev = p0->y;
	p1->y_prev = p1->y;

	if (scores)
	{
		char buf[16];
		move_cursor(SCORE_POS_X, SCORE_POS_Y);
		sprintf(buf, "%u  -  %u", scores[0], scores[1]);
		serial_write(buf, strlen(buf));
	}
}

static bool should_draw_frame()
{
	if (_next_frame_time[0] == 0 && _next_frame_time[1] == 0)
	{
		timer_get_tick_count(_next_frame_time);
		add_frame_time(_next_frame_time);
		return true;
	}

	unsigned short t[2];
	timer_get_tick_count(t);
	if ((t[0] > _next_frame_time[0]) ||
		((t[0] == _next_frame_time[0]) && (t[1] >= _next_frame_time[1])))
	{
		_next_frame_time[0] = t[0];
		_next_frame_time[1] = t[1];
		add_frame_time(_next_frame_time);
		return true;
	}

	return false;
}

static void add_frame_time(unsigned short* t)
{
	t[1] += GAME_FRAME_TICKS;
	if (t[1] < GAME_FRAME_TICKS)
	{
		t[0]++;
	}
}

static void move_cursor(short x, short y)
{
	char buf[16];
	sprintf(buf, "\e[%u;%uH", y, x);
	serial_write(buf, strlen(buf));
}

static unsigned char get_char(bool block)
{
	if (!block)
	{
		unsigned short t[2];
		timer_get_tick_count(t);
		if ((t[0] > _next_frame_time[0]) ||
			((t[0] == _next_frame_time[0]) && (t[1] >= _next_frame_time[1])))
		{
			return 0;
		}
		else
		{
			timer_notify_t tn;

			tn.t[0] = _next_frame_time[0];
			tn.t[1] = _next_frame_time[1];
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

static short ball_c2v(short c)
{
	return (c << 8);
}

static short ball_v2c(short v)
{
	return (v >> 8);
}

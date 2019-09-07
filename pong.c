#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "pong.h"
#include "game.h"

#include "draw.h"
#include "rng.h"
#include "serial.h"
#include "term.h"


#define PONG_WIDTH 80
#define PONG_HEIGHT 20

#define WALL_C '#'
#define PADDLE_C '|'
#define BALL_C '+'
#define SPACE_C ' '
#define CLEAR_C SPACE_C
#define RESUME_C SPACE_C
#define QUIT_C 'Q'

#define P0_UP_C 'w'
#define P0_DOWN_C 's'
#define P1_UP_C 'o'
#define P1_DOWN_C 'l'

#define PADDLE_H (3)
#define PADDLE_Y_INIT (PONG_HEIGHT / 2 - 1)

#define PADDLE0_X (2)
#define PADDLE1_X (PONG_WIDTH - 1)

#define BALL_X_INIT_C (PONG_WIDTH / 2 - 1)
#define BALL_Y_INIT_C (PONG_HEIGHT / 2 - 1)

#define SCORE_POS_X (PONG_WIDTH / 2 - 5)
#define SCORE_POS_Y (PONG_HEIGHT + 2)

#define PONG_GAME_FPS 24


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


static void paddle_up(paddle_t* p);
static void paddle_down(paddle_t* p);
static void update_ball(ball_t* b);
static short check_ball_position(ball_t* b, paddle_t* p0, paddle_t* p1);
static void draw_frame(paddle_t* p0, paddle_t* p1, ball_t* ball, unsigned short* scores);
static short ball_c2v(short c);
static short ball_v2c(short v);


void pong_main()
{
	// Init paddles.
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

	// Init ball.
	ball_t ball = { ball_c2v(BALL_X_INIT_C), ball_c2v(BALL_Y_INIT_C), ball_vx, ball_vy };

	// Init scores.
	unsigned short scores[2] = { 0, 0 };


	// Prepare for drawing on screen.
	term_set_cursor(false);
	term_clear_screen();

	// Draw walls.
	draw_border(PONG_WIDTH, PONG_HEIGHT, WALL_C);

	// Draw paddles.
	draw_vertical(PADDLE0_X, p0.y, PADDLE_H, PADDLE_C);
	draw_vertical(PADDLE1_X, p1.y, PADDLE_H, PADDLE_C);

	// Draw ball.
	draw_vertical(ball_v2c(ball.x), ball_v2c(ball.y), 1, BALL_C);

	term_cursor_home();


	game_context_t ctx;
	game_context_init(&ctx, PONG_GAME_FPS);

	unsigned char c = game_get_char(&ctx, true);
	short check = 0;
	while (1)
	{
		if (c == QUIT_C)
		{
			break;
		}
		else if (c == P0_UP_C)
		{
			paddle_up(&p0);
		}
		else if (c == P0_DOWN_C)
		{
			paddle_down(&p0);
		}
		else if (c == P1_UP_C)
		{
			paddle_up(&p1);
		}
		else if (c == P1_DOWN_C)
		{
			paddle_down(&p1);
		}

		if (game_should_draw_frame(&ctx))
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
			c = game_get_char(&ctx, check != 0);
		} while (check != 0 && c != RESUME_C && c != QUIT_C);
	}

	term_clear_screen();
	term_set_cursor(true);
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
	// Redraw paddle 0, if necessary.
	if (p0->y_prev != p0->y)
	{
		draw_vertical(PADDLE0_X, p0->y_prev, PADDLE_H, CLEAR_C);
		draw_vertical(PADDLE0_X, p0->y, PADDLE_H, PADDLE_C);
		p0->y_prev = p0->y;
	}

	// Redraw paddle 1, if necessary.
	if (p1->y_prev != p1->y)
	{
		draw_vertical(PADDLE1_X, p1->y_prev, PADDLE_H, CLEAR_C);
		draw_vertical(PADDLE1_X, p1->y, PADDLE_H, PADDLE_C);
		p1->y_prev = p1->y;
	}

	short ball_prev_x = ball_v2c(ball->x - ball->vx);
	short ball_prev_y = ball_v2c(ball->y - ball->vy);

	// Determine what is at the ball's previous position.
	char ball_prev_c;
	if (((ball_prev_x == PADDLE0_X) &&
		(ball_prev_y >= p0->y) &&
		(ball_prev_y < p0->y + PADDLE_H)) ||
		((ball_prev_x == PADDLE1_X) &&
		(ball_prev_y >= p1->y) &&
		(ball_prev_y < p1->y + PADDLE_H)))
	{
		// Previous ball position is on paddle.
		ball_prev_c = PADDLE_C;
	}
	else
	{
		// Previous ball position is empty space.
		ball_prev_c = CLEAR_C;
	}

	// Draw new ball position.
	draw_vertical(ball_prev_x, ball_prev_y, 1, ball_prev_c);
	draw_vertical(ball_v2c(ball->x), ball_v2c(ball->y), 1, BALL_C);

	// Print scores, if necessary.
	if (scores)
	{
		char buf[16];
		term_move_cursor(SCORE_POS_X, SCORE_POS_Y);
		sprintf(buf, "%u  -  %u", scores[0], scores[1]);
		serial_write(buf, strlen(buf));
	}
}

static short ball_c2v(short c)
{
	return (c << 8);
}

static short ball_v2c(short v)
{
	return (v >> 8);
}

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "bricks.h"
#include "game.h"

#include "draw.h"
#include "rng.h"
#include "serial.h"
#include "term.h"


#define BRICKS_WIDTH 80
#define BRICKS_HEIGHT 20

#define BRICK_MAP_WIDTH ((BRICKS_WIDTH >> 2) - 2)
#define BRICK_MAP_HEIGHT (BRICKS_HEIGHT - 6)

#define WALL_C '#'
#define PADDLE_C '='
#define BALL_C '+'
#define SPACE_C ' '
#define CLEAR_C SPACE_C
#define RESUME_C SPACE_C
#define QUIT_C 'Q'

#define P_LEFT_C 'a'
#define P_RIGHT_C 'd'

#define PADDLE_W (5)
#define PADDLE_X_INIT (BRICKS_WIDTH / 2 - 2)
#define PADDLE_Y (BRICKS_HEIGHT - 1)

#define SCORE_POS_X (BRICKS_WIDTH / 2 - 5)
#define SCORE_POS_Y (BRICKS_HEIGHT + 2)

#define BRICKS_GAME_FPS 24


typedef struct
{
	short x;
	short x_prev;
} paddle_t;

typedef struct
{
	// All ball values are in 1/256th units.
	short x;
	short y;
	short vx;
	short vy;

	bool brick;
} ball_t;


static void paddle_left(paddle_t* p);
static void paddle_right(paddle_t* p);
static short update_and_check_ball_position(ball_t* b, paddle_t* p, unsigned char* brick_map);
static void draw_frame(paddle_t* p, ball_t* ball, short score);
static short ball_c2v(short c);
static short ball_v2c(short v);
static short brick_map_x_to_tx(short x);
static short brick_map_y_to_ty(short y);
static short brick_map_tx_to_x(short tx);
static short brick_map_ty_to_y(short ty);


void bricks_main()
{
	// Init paddle.
	paddle_t p = { PADDLE_X_INIT, PADDLE_X_INIT };

	const short ball_vx_init = 1 << 7;
	const short ball_vy_init = -(1 << 7);

	// Init ball.
	ball_t ball = { ball_c2v(PADDLE_X_INIT + (PADDLE_W / 2)), ball_c2v(PADDLE_Y - 1), ball_vx_init, ball_vy_init, false };

	// Init score.
	short score = 0;

	// Init brick map.
	unsigned char bricks[BRICK_MAP_WIDTH * BRICK_MAP_HEIGHT];
	for (short i = 0; i < BRICK_MAP_HEIGHT; i++)
	{
		unsigned char brick_val = 0;

		// Create three rows of bricks.
		if (i >= 1 && i <= 3)
		{
			brick_val = 1;
		}

		for (short j = 0; j < BRICK_MAP_WIDTH; j++)
		{
			bricks[BRICK_MAP_WIDTH * i + j] = brick_val;
		}
	}


	// Prepare for drawing on screen.
	term_set_cursor(false);
	term_clear_screen();

	// Draw walls.
	draw_border(BRICKS_WIDTH, BRICKS_HEIGHT, WALL_C);

	// Draw bricks from map.
	for (short i = 0; i < BRICK_MAP_HEIGHT; i++)
	{
		const short y = brick_map_y_to_ty(i);

		for (short j = 0; j < BRICK_MAP_WIDTH; j++)
		{
			if (bricks[BRICK_MAP_WIDTH * i + j] == 0)
			{
				continue;
			}

			const short x = brick_map_x_to_tx(j);
			draw_horizontal_bg(x, y, 4, (y % 3) + 1);
		}
	}

	// Draw paddle.
	draw_horizontal(p.x, PADDLE_Y, PADDLE_W, PADDLE_C);

	// Draw ball.
	draw_horizontal(ball_v2c(ball.x), ball_v2c(ball.y), 1, BALL_C);

	term_cursor_home();


	game_context_t ctx;
	game_context_init(&ctx, BRICKS_GAME_FPS);

	unsigned char c = game_get_char(&ctx, true);
	short check = 0;
	while (1)
	{
		if (c == QUIT_C)
		{
			break;
		}
		else if (c == P_LEFT_C)
		{
			paddle_left(&p);
		}
		else if (c == P_RIGHT_C)
		{
			paddle_right(&p);
		}

		if (game_should_draw_frame(&ctx))
		{
			check = update_and_check_ball_position(&ball, &p, bricks);
			if (check != 0)
			{
				score += check;
			}

			draw_frame(&p, &ball, check == 0 ? 0 : score);
		}

		do {
			c = game_get_char(&ctx, check < 0);
		} while (check < 0 && c != RESUME_C && c != QUIT_C);

		if (check < 0)
		{
			// Move ball to paddle.
			draw_horizontal(ball_v2c(ball.x), ball_v2c(ball.y), 1, CLEAR_C);

			ball.x = ball_c2v(p.x + (PADDLE_W / 2));
			ball.y = ball_c2v(PADDLE_Y - 1);
			ball.vx = ball_vx_init;
			ball.vy = ball_vy_init;

			draw_horizontal(ball_v2c(ball.x), ball_v2c(ball.y), 1, BALL_C);
		}
	}

	term_clear_screen();
	term_set_cursor(true);
}


static void paddle_left(paddle_t* p)
{
	if (p->x <= 2)
	{
		return;
	}

	p->x--;
}

static void paddle_right(paddle_t* p)
{
	if (p->x >= BRICKS_WIDTH - PADDLE_W)
	{
		return;
	}

	p->x++;
}

static short update_and_check_ball_position(ball_t* b, paddle_t* p, unsigned char* brick_map)
{
	short check = 0;

	short ball_x = ball_v2c(b->x);
	short ball_y = ball_v2c(b->y);

	// Bounce off sides?
	if ((b->vx > 0 && ball_x >= BRICKS_WIDTH - 1) || ((b->vx < 0 && ball_x <= 2)))
	{
		b->vx = -b->vx;
	}

	// Bounce off top/bottom?
	if ((b->vy > 0 && ball_y >= BRICKS_HEIGHT - 1) || ((b->vy < 0 && ball_y <= 2)))
	{
		if (ball_y == PADDLE_Y && (ball_x < p->x || ball_x > p->x + PADDLE_W))
		{
			// Ball missed paddle.
			check = -10;
		}

		b->vy = -b->vy;
	}

	// Bounce of wall?
	const short mx = brick_map_tx_to_x(ball_x);
	const short my = brick_map_ty_to_y(ball_y);
	if (mx >= 0 && my >= 0 && mx < BRICK_MAP_WIDTH && my < BRICK_MAP_HEIGHT)
	{
		const short bricks_i = my * BRICK_MAP_WIDTH + mx;
		if (brick_map[bricks_i] != 0)
		{
			b->vy = -b->vy; // TODO: What if ball hit brick from the side?
			brick_map[bricks_i] = 0;
			b->brick = true;
			check = 10;
		}
	}

	b->x += b->vx;
	b->y += b->vy;

	return check;
}

static void draw_frame(paddle_t* p, ball_t* ball, short score)
{
	// Redraw paddle, if necessary.
	if (p->x_prev != p->x)
	{
		draw_horizontal(p->x_prev, PADDLE_Y, PADDLE_W, CLEAR_C);
		draw_horizontal(p->x, PADDLE_Y, PADDLE_W, PADDLE_C);
		p->x_prev = p->x;
	}

	short ball_prev_x = ball_v2c(ball->x - ball->vx);
	short ball_prev_y = ball_v2c(ball->y - ball->vy);

	// Determine what is at the ball's previous position.
	char ball_prev_c;
	if (((ball_prev_y == PADDLE_Y) &&
		(ball_prev_x >= p->x) &&
		(ball_prev_x < p->x + PADDLE_W)))
	{
		// Previous ball position is on paddle.
		ball_prev_c = PADDLE_C;
	}
	else
	{
		if (ball->brick)
		{
			// Clear brick at previous ball position.
			draw_horizontal_bg(brick_map_x_to_tx(brick_map_tx_to_x(ball_prev_x)), ball_prev_y, 4, DRAW_BG_RESET);
			ball->brick = false;
		}

		// Previous ball position is empty space.
		ball_prev_c = CLEAR_C;
	}

	// Draw new ball position.
	draw_horizontal(ball_prev_x, ball_prev_y, 1, ball_prev_c);
	draw_horizontal(ball_v2c(ball->x), ball_v2c(ball->y), 1, BALL_C);

	// Print score, if necessary.
	if (score)
	{
		char buf[8];
		term_move_cursor(SCORE_POS_X, SCORE_POS_Y);
		sprintf(buf, "%d", score);
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

static short brick_map_x_to_tx(short x)
{
	return ((x << 2) + 5);
}

static short brick_map_y_to_ty(short y)
{
	return (y + 2);
}

static short brick_map_tx_to_x(short tx)
{
	return ((tx - 5) >> 2);
}

static short brick_map_ty_to_y(short ty)
{
	return (ty - 2);
}

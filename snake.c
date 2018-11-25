#include <stdbool.h>
#include <string.h>

#include "snake.h"
#include "game.h"

#include "draw.h"
#include "rng.h"
#include "term.h"


// WARNING: Odd values for width/height not tested and not likely to work as is!
//          Additionally, these values must be chosen carefully to not run out of memory,
//          as the map requires (SNAKE_WIDTH * SNAKE_HEIGHT / 2) bytes of memory
#define SNAKE_WIDTH 80
#define SNAKE_HEIGHT 20

#define SNAKE_START_TAIL_X (SNAKE_WIDTH / 2 - 2)
#define SNAKE_START_TAIL_Y (SNAKE_HEIGHT / 2)
#define SNAKE_START_LEN 3

#define WALL_C '#'
#define SNAKE_C '%'
#define FOOD_C '+'
#define SPACE_C ' '
#define CLEAR_C SPACE_C
#define RESUME_C SPACE_C
#define QUIT_C 'Q'

#define KEY_UP_C 'w'
#define KEY_DOWN_C 's'
#define KEY_RIGHT_C 'd'
#define KEY_LEFT_C 'a'


#define MAP_SNAKE_BIT 0x08
#define MAP_WALL_BIT 0x04
#define MAP_SNAKE_DIR_MASK 0x03

#define SNAKE_DIR_UP 0x00
#define SNAKE_DIR_DOWN 0x01
#define SNAKE_DIR_RIGHT 0x02
#define SNAKE_DIR_LEFT 0x03

#define SNAKE_DIR_AXIS_BIT 0x02

#define MAX_FOOD_PLACE_ATTEMPTS 5

#define SNAKE_GAME_FPS 12


typedef struct
{
	short head_x;
	short head_y;
	short tail_x;
	short tail_y;
	short tail_prev_x;
	short tail_prev_y;
	unsigned char head_dir;
	bool grow;
} snake_t;

typedef struct
{
	short x;
	short y;
	bool active;
	bool need_draw;
} food_t;


static void snake_dir_change(snake_t* snake, unsigned char dir);
static bool update_snake(snake_t* snake, food_t* food, unsigned char* map);
static void update_food(food_t* food, unsigned char* map);
static void draw_frame(snake_t* snake, food_t* food);


void snake_main()
{
	// Init snake.
	snake_t snake = {
		SNAKE_START_TAIL_X + SNAKE_START_LEN - 1,
		SNAKE_START_TAIL_Y,
		SNAKE_START_TAIL_X,
		SNAKE_START_TAIL_Y,
		SNAKE_START_TAIL_X - 1,
		SNAKE_START_TAIL_Y,
		SNAKE_DIR_RIGHT,
		false
	};

	// Init food.
	food_t food = { 0, 0, false, false };

	// Init map.
	const unsigned short map_size = (SNAKE_WIDTH * SNAKE_HEIGHT) / 2;
	unsigned char map[map_size];
	memset(map, 0, map_size);

	// Place walls on map.
	for (unsigned short i = 0; i < SNAKE_HEIGHT; i++)
	{
		unsigned short map_row_pos = i * SNAKE_WIDTH;

		if (i == 0 || i == SNAKE_HEIGHT - 1)
		{
			// Top and bottom walls
			memset(map + (map_row_pos >> 1), ((MAP_WALL_BIT << 4) | MAP_WALL_BIT), (SNAKE_WIDTH >> 1));
		}
		else
		{
			// Side walls
			map[map_row_pos >> 1] |= (MAP_WALL_BIT << 4);
			map[(map_row_pos + SNAKE_WIDTH - 1) >> 1] |= MAP_WALL_BIT;
		}
	}

	// Place snake on map.
	unsigned short snake_tail_map_pos = snake.tail_y * SNAKE_WIDTH + snake.tail_x;
	for (short i = 0; i < SNAKE_START_LEN; i++)
	{
		unsigned short sh = (((snake_tail_map_pos + i) & 0x01) == 0x01 ? 0 : 4);
		map[(snake_tail_map_pos + i) >> 1] |= ((MAP_SNAKE_BIT | SNAKE_DIR_RIGHT) << sh);
	}


	// Prepare for drawing on screen.
	term_set_cursor(false);
	term_clear_screen();

	// Draw walls.
	draw_border(SNAKE_WIDTH, SNAKE_HEIGHT, WALL_C);

	// Draw snake.
	draw_horizontal(snake.tail_x + 1, snake.tail_y + 1, SNAKE_START_LEN, SNAKE_C);

	term_cursor_home();


	game_context_t ctx;
	game_context_init(&ctx, SNAKE_GAME_FPS);

	unsigned char c = game_get_char(&ctx, true);
	bool check = false;
	while (1)
	{
		if (game_should_draw_frame(&ctx))
		{
			update_food(&food, map);
			check = update_snake(&snake, &food, map);

			draw_frame(&snake, &food);
		}

		if (c == QUIT_C)
		{
			break;
		}
		else if (c == KEY_UP_C)
		{
			snake_dir_change(&snake, SNAKE_DIR_UP);
		}
		else if (c == KEY_DOWN_C)
		{
			snake_dir_change(&snake, SNAKE_DIR_DOWN);
		}
		else if (c == KEY_RIGHT_C)
		{
			snake_dir_change(&snake, SNAKE_DIR_RIGHT);
		}
		else if (c == KEY_LEFT_C)
		{
			snake_dir_change(&snake, SNAKE_DIR_LEFT);
		}

		do {
			c = game_get_char(&ctx, check);
		} while (check && c != RESUME_C && c != QUIT_C);
	}

	term_clear_screen();
	term_set_cursor(true);
}


static void snake_dir_change(snake_t* snake, unsigned char dir)
{
	// Only change direction if it's to a different axis than the current one.
	if ((snake->head_dir & SNAKE_DIR_AXIS_BIT) != (dir & SNAKE_DIR_AXIS_BIT))
	{
		snake->head_dir = dir;
	}
}

static bool update_snake(snake_t* snake, food_t* food, unsigned char* map)
{
	unsigned short map_head_pos_prev = snake->head_y * SNAKE_WIDTH + snake->head_x;

	// Update head position.
	switch (snake->head_dir)
	{
	case SNAKE_DIR_UP:
		snake->head_y--;
		break;
	case SNAKE_DIR_DOWN:
		snake->head_y++;
		break;
	case SNAKE_DIR_RIGHT:
		snake->head_x++;
		break;
	case SNAKE_DIR_LEFT:
		snake->head_x--;
		break;
	}

	unsigned short map_head_pos = snake->head_y * SNAKE_WIDTH + snake->head_x;
	unsigned short map_tail_pos = snake->tail_y * SNAKE_WIDTH + snake->tail_x;

	// Only update tail if the snake is not growing.
	if (!snake->grow)
	{
		snake->tail_prev_x = snake->tail_x;
		snake->tail_prev_y = snake->tail_y;

		// Update tail position.
		unsigned char tail_dir;
		if ((map_tail_pos & 0x01) == 0x01)
		{
			tail_dir = map[map_tail_pos >> 1] & MAP_SNAKE_DIR_MASK;
		}
		else
		{
			tail_dir = (map[map_tail_pos >> 1] & (MAP_SNAKE_DIR_MASK << 4)) >> 4;
		}

		switch (tail_dir)
		{
		case SNAKE_DIR_UP:
			snake->tail_y--;
			break;
		case SNAKE_DIR_DOWN:
			snake->tail_y++;
			break;
		case SNAKE_DIR_RIGHT:
			snake->tail_x++;
			break;
		case SNAKE_DIR_LEFT:
			snake->tail_x--;
			break;
		}

		if ((map_tail_pos & 0x01) == 0x01)
		{
			map[map_tail_pos >> 1] &= 0xf0;
		}
		else
		{
			map[map_tail_pos >> 1] &= 0x0f;
		}
	}
	else
	{
		snake->grow = false;
	}

	unsigned char map_val;
	if ((map_head_pos & 0x01) == 0x01)
	{
		map_val = map[map_head_pos >> 1] & 0x0f;
	}
	else
	{
		map_val = (map[map_head_pos >> 1] & 0xf0) >> 4;
	}

	if (map_val != 0)
	{
		// Snake has run into something.
		return true;
	}

	if ((map_head_pos_prev & 0x01) == 0x01)
	{
		map[map_head_pos_prev >> 1] |= (MAP_SNAKE_BIT | snake->head_dir);
	}
	else
	{
		map[map_head_pos_prev >> 1] |= ((MAP_SNAKE_BIT | snake->head_dir) << 4);
	}

	if (food->active && food->x == snake->head_x && food->y == snake->head_y)
	{
		// Eat food.
		food->active = false;
		snake->grow = true;
	}

	return false;
}

static void update_food(food_t* food, unsigned char* map)
{
	if (food->active)
	{
		return;
	}

	if ((((unsigned short)rng_rand()) & 0x1f) == 0x00)
	{
		short attempts = 0;
		for (; attempts < MAX_FOOD_PLACE_ATTEMPTS; attempts++)
		{
			food->x = (((unsigned short)rng_rand()) % (SNAKE_WIDTH - 2)) + 1;
			food->y = (((unsigned short)rng_rand()) % (SNAKE_HEIGHT - 2)) + 1;

			unsigned short map_pos = food->y * SNAKE_HEIGHT + food->x;
			unsigned char map_val;
			if ((map_pos & 0x01) == 0x01)
			{
				map_val = map[map_pos >> 1] & 0x0f;
			}
			else
			{
				map_val = (map[map_pos >> 1] & 0xf0) >> 4;
			}

			if (map_val == 0)
			{
				break;
			}
		}

		if (attempts == MAX_FOOD_PLACE_ATTEMPTS)
		{
			return;
		}

		food->active = true;
		food->need_draw = true;
	}
}

static void draw_frame(snake_t* snake, food_t* food)
{
	// Draw snake head.
	draw_horizontal(snake->head_x + 1, snake->head_y + 1, 1, SNAKE_C);

	// Clear snake tail.
	draw_horizontal(snake->tail_prev_x + 1, snake->tail_prev_y + 1, 1, CLEAR_C);

	// Draw food, if needed.
	if (food->need_draw && food->active)
	{
		draw_horizontal(food->x + 1, food->y + 1, 1, FOOD_C);
		food->need_draw = false;
	}
}

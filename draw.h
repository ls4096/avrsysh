#ifndef _DRAW_H_
#define _DRAW_H_

typedef enum {
	DRAW_BG_RESET = 0,
	DRAW_BG_RED = 1,
	DRAW_BG_GREEN = 2,
	DRAW_BG_BLUE = 3
} draw_bg_setting;

void draw_vertical(short x, short y, short h, char c);
void draw_horizontal(short x, short y, short l, char c);
void draw_border(short w, short h, short c);

void draw_vertical_bg(short x, short y, short h, draw_bg_setting bg);
void draw_horizontal_bg(short x, short y, short l, draw_bg_setting bg);

#endif // _DRAW_H_

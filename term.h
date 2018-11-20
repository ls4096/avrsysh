#ifndef _TERM_H_
#define _TERM_H_

#include <stdbool.h>

void term_set_cursor(bool enable);
void term_cursor_home();
void term_move_cursor(short x, short y);
void term_clear_screen();

#endif // _TERM_H_

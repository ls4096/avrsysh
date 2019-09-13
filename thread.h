#ifndef _THREAD_H_
#define _THREAD_H_

#include <stdbool.h>

typedef void (*thread_entry_func)(void*);

bool thread_is_running();
char thread_which_is_running();
void thread_create(thread_entry_func func, void* arg);
void thread_join();
void thread_switch();
unsigned short thread_switch_count();

bool thread_pipe_has_next_byte();
unsigned char thread_read_pipe();
bool thread_is_pipe_full();
void thread_write_pipe(unsigned char c);

#endif // _THREAD_H_

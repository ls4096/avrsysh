#ifndef _TIME_H_
#define _TIME_H_

#include <stdbool.h>

bool time_is_set();
bool time_set_time(const char* str);
void time_get_time(char* str);

#endif // _TIME_H_

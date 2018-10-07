#ifndef _PM_H_
#define _PM_H_

void pm_reset();
void pm_yield();

void pm_update_wake_counter(unsigned char c);
void pm_get_wake_count(unsigned short* w);

#endif // _PM_H_

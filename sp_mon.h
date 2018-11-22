#ifndef _SP_MON_H_
#define _SP_MON_H_

#include <avr/io.h>

#ifndef RAMEND
	#error RAMEND not defined
#endif

// Buckets of 64 bytes.
#define SP_MON_BUCKET_SIZE_BITS 6
#define SP_MON_NUM_BUCKETS ((RAMEND + 1) >> SP_MON_BUCKET_SIZE_BITS)

#include <stdbool.h>


void sp_mon_enable(bool enable);

void sp_mon_check();

volatile unsigned short* sp_mon_get_buckets();


#endif // _SP_MON_H_

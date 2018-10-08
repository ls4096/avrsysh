#include "rng.h"

// A simple 16-bit LCG implementation that allows for adding optional entropy.

// Constants chosen to satisfy Hull-Dobell requirements (with implied m=2^16).
#define A 3677
#define C 17863

static volatile unsigned short _r = 8161;

void rng_add_entropy(unsigned char e)
{
	_r <<= 1;
	_r |= (e & 0x0001);
}

short rng_rand()
{
	_r = A * _r + C;
	return (short)_r;
}

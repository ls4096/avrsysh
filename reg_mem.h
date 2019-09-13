#ifndef _REG_MEM_H_
#define _REG_MEM_H_

#include <avr/io.h>

#define REG_SPL		_SFR_MEM8(0x005d)
#define REG_SPH		_SFR_MEM8(0x005e)
#define REG_SREG	_SFR_MEM8(0x005f)

#define REG_SP ((REG_SPH << 8) | REG_SPL)

#endif // _REG_MEM_H_

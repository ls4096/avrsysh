#include <string.h>
#include <stdint.h>

#include "thread.h"

#include "avr_mcu.h"
#include "dump.h"
#include "pm.h"
#include "reg_mem.h"

static char _running = -1;
static uint8_t* _saved_sp;
static unsigned char _pipe_buf[THREAD_PIPE_BUF_SIZE];
static unsigned char _pipe_buf_next_write;
static unsigned char _pipe_buf_next_read;
static bool _pipe_in_end;
static unsigned short _thread_switch_count = 0;


static void return_from_thread();
static bool is_returned_from_thread();


bool thread_is_running()
{
	return (_running != -1);
}

char thread_which_is_running()
{
	return _running;
}

void thread_create(thread_entry_func func, void* arg)
{
	_saved_sp = (uint8_t*)(REG_SP - THREAD_STACK_OFFSET);

	*(_saved_sp--) = (uint16_t)(&return_from_thread);
	*(_saved_sp--) = (((uint16_t)(&return_from_thread)) >> 8);
#if (PC_SIZE_BYTES == 3)
	*(_saved_sp--) = 0;
#endif

	*(_saved_sp--) = (uint16_t)func;
	*(_saved_sp--) = (((uint16_t)func) >> 8);
#if (PC_SIZE_BYTES == 3)
	*(_saved_sp--) = 0;
#endif

	for (short i = 31; i >= 0; i--)
	{
		switch (i)
		{
		case 25:
			*(_saved_sp--) = (((uint16_t)arg) >> 8);
			break;
		case 24:
			*(_saved_sp--) = ((uint16_t)arg);
			break;
		default:
			*(_saved_sp--) = 0x00;
			break;
		}
	}
	*(_saved_sp--) = 0x80;

	_pipe_buf_next_write = 0;
	_pipe_buf_next_read = 0;
	_pipe_in_end = false;
	_running = 0;
}

void thread_join()
{
	if (!thread_is_running())
	{
		dump_state();
	}

	_pipe_in_end = true;

	while (!is_returned_from_thread())
	{
		pm_yield();
	}

	_running = -1;
}

void thread_switch()
{
	asm volatile (
		"push	r31\r\n" \
		"push	r30\r\n" \
		"push	r29\r\n" \
		"push	r28\r\n" \
		"push	r27\r\n" \
		"push	r26\r\n" \
		"push	r25\r\n" \
		"push	r24\r\n" \
		"push	r23\r\n" \
		"push	r22\r\n" \
		"push	r21\r\n" \
		"push	r20\r\n" \
		"push	r19\r\n" \
		"push	r18\r\n" \
		"push	r17\r\n" \
		"push	r16\r\n" \
		"push	r15\r\n" \
		"push	r14\r\n" \
		"push	r13\r\n" \
		"push	r12\r\n" \
		"push	r11\r\n" \
		"push	r10\r\n" \
		"push	r9\r\n" \
		"push	r8\r\n" \
		"push	r7\r\n" \
		"push	r6\r\n" \
		"push	r5\r\n" \
		"push	r4\r\n" \
		"push	r3\r\n" \
		"push	r2\r\n" \
		"push	r1\r\n" \
		"push	r0\r\n" \
		"in	r24, 0x3f\r\n" \
		"push	r24\r\n"
	);

	asm volatile ( "cli" );

	uint8_t* sp = _saved_sp;
	_saved_sp = (uint8_t*)REG_SP;
	REG_SPH = (((uint16_t)sp) >> 8);
	REG_SPL = ((uint16_t)sp);

	if (_running != -2)
	{
		_running = (_running == 1 ? 0 : 1);
	}

	_thread_switch_count++;

	asm volatile ( "sei" );

	asm volatile (
		"pop	r24\r\n" \
		"out	0x3f, r24\r\n" \
		"pop	r0\r\n" \
		"pop	r1\r\n" \
		"pop	r2\r\n" \
		"pop	r3\r\n" \
		"pop	r4\r\n" \
		"pop	r5\r\n" \
		"pop	r6\r\n" \
		"pop	r7\r\n" \
		"pop	r8\r\n" \
		"pop	r9\r\n" \
		"pop	r10\r\n" \
		"pop	r11\r\n" \
		"pop	r12\r\n" \
		"pop	r13\r\n" \
		"pop	r14\r\n" \
		"pop	r15\r\n" \
		"pop	r16\r\n" \
		"pop	r17\r\n" \
		"pop	r18\r\n" \
		"pop	r19\r\n" \
		"pop	r20\r\n" \
		"pop	r21\r\n" \
		"pop	r22\r\n" \
		"pop	r23\r\n" \
		"pop	r24\r\n" \
		"pop	r25\r\n" \
		"pop	r26\r\n" \
		"pop	r27\r\n" \
		"pop	r28\r\n" \
		"pop	r29\r\n" \
		"pop	r30\r\n" \
		"pop	r31\r\n"
	);
}

unsigned short thread_switch_count()
{
	return _thread_switch_count;
}

bool thread_pipe_has_next_byte()
{
	return !(_pipe_buf_next_read == _pipe_buf_next_write);
}

unsigned char thread_read_pipe()
{
	while (!thread_pipe_has_next_byte())
	{
		if (_pipe_in_end)
		{
			// Pipe is empty and first thread is done.
			return 0x04;
		}

		pm_yield();
	}

	unsigned char c = _pipe_buf[_pipe_buf_next_read];
	_pipe_buf_next_read = ((_pipe_buf_next_read + 1) % THREAD_PIPE_BUF_SIZE);

	return c;
}

bool thread_is_pipe_full()
{
	return (((_pipe_buf_next_write + 1) % THREAD_PIPE_BUF_SIZE) == _pipe_buf_next_read);
}

void thread_write_pipe(unsigned char c)
{
	if (is_returned_from_thread())
	{
		return;
	}

	while (thread_is_pipe_full())
	{
		pm_yield();
	}

	_pipe_buf[_pipe_buf_next_write] = c;
	_pipe_buf_next_write = ((_pipe_buf_next_write + 1) % THREAD_PIPE_BUF_SIZE);
}


static void return_from_thread()
{
	if (_running != 1)
	{
		dump_state();
	}

	_running = -2;
	thread_switch();
}

static bool is_returned_from_thread()
{
	return (_running == -2);
}

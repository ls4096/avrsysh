#include <avr/interrupt.h>
#include <stdbool.h>
#include <string.h>

#include "command.h"
#include "led.h"
#include "serial.h"
#include "thermal.h"
#include "timer.h"

#define CMD_BUF_SIZE 32

static const unsigned char* GREETING_1 = "Welcome to avrsysh!";
static const unsigned char* GREETING_2 = "Enter command:";
static const unsigned char* PROMPT = "> ";
static const unsigned char BACKSPACE[6] = { 0x1b, '[', 'D', 0x1b, '[', 'K' };

static void reset(void);
static short loop(void);
 
void main(void)
{
	serial_init();
	timer_init();
	thermal_init();
	led_init();

	sei();

	serial_write_newline();
	serial_write(GREETING_1, strlen(GREETING_1));
	serial_write_newline();
	serial_write(GREETING_2, strlen(GREETING_2));
	serial_write_newline();

	if (loop() == PC_RC_RESET)
	{
		reset();
	}
}

static void reset(void)
{
	asm volatile (
		"cli\r\n" \
		"jmp 0\r\n"
	);
}

static short loop(void)
{
	unsigned char buf[CMD_BUF_SIZE];
	unsigned char last_cmd[CMD_BUF_SIZE];
	short buf_i = 0;
	bool esc = false;

	serial_write(PROMPT, strlen(PROMPT));

	memset(buf, 0, CMD_BUF_SIZE);
	memset(last_cmd, 0, CMD_BUF_SIZE);

	while (1)
	{
		unsigned char c = serial_read_next_byte();

		if (esc)
		{
			if (c != '[')
			{
				esc = false;
			}

			if (c == 'A')
			{
				// First, erase anything currently in the command buffer.
				short back = strlen(buf);
				for (short i = 0; i < back; i++)
				{
					serial_write(BACKSPACE, sizeof(BACKSPACE));
				}

				strcpy(buf, last_cmd);
				serial_write(buf, strlen(buf));
				buf_i = strlen(buf);
			}
		}
		else if (c == 0x08 || c == 0x7f)
		{
			if (buf_i != 0)
			{
				serial_write(BACKSPACE, sizeof(BACKSPACE));
				buf[--buf_i] = 0;
			}
		}
		else if (c == '\t')
		{
			if (buf_i == 0)
			{
				continue;
			}

			unsigned short match_count;
			const char* completed = command_tab_complete(buf, buf_i, &match_count);
			if (completed != 0)
			{
				for (short i = 0; i < buf_i; i++)
				{
					serial_write(BACKSPACE, sizeof(BACKSPACE));
				}
				strcpy(buf, completed);
				buf_i = strlen(completed);
				serial_write(buf, buf_i);
			}
			else if (match_count > 1)
			{
				serial_write_newline();
				serial_write(PROMPT, strlen(PROMPT));
				serial_write(buf, strlen(buf));
			}
		}
		else if (c == 0x0d)
		{
			serial_write_newline();
			buf[buf_i++] = 0;

			if (buf_i > 1)
			{
				strcpy(last_cmd, buf);
			}

			char rc = command_process(buf);
			if (rc < 0)
			{
				return rc;
			}

			short buf_len = strlen(buf) + 1;
			memset(buf, 0, buf_len);
			buf_i = 0;

			serial_write(PROMPT, strlen(PROMPT));
		}
		else if (c >= 0x20 && c <= 0x7e)
		{
			if (buf_i == CMD_BUF_SIZE - 1)
			{
				continue;
			}

			serial_tx_byte(c);
			buf[buf_i++] = c;
		}
		else if (c == 0x1b)
		{
			esc = true;
		}
	}

	return 0;
}

#include <avr/interrupt.h>
#include <stdbool.h>

#include "led.h"
#include "serial.h"
#include "thermal.h"

#define CMD_BUF_SIZE 32

static const unsigned char* GREETING = "\r\nWelcome to avrsysh!\r\nEnter command:\r\n";
static const unsigned char* PROMPT = "> ";
static const unsigned char NEWLINE[2] = { 0x0d, 0x0a };
static const unsigned char BACKSPACE[6] = { 0x1b, '[', 'D', 0x1b, '[', 'K' };

static int loop(void);
 
int main(void)
{
	serial_init();
	timer_init();
	thermal_init();
	led_init();

	sei();

	serial_write(GREETING, strlen(GREETING));

	return loop();
}

static int loop(void)
{
	unsigned char buf[CMD_BUF_SIZE];
	unsigned char last_cmd[CMD_BUF_SIZE];
	short i = 0;
	bool esc = false;

	serial_write(PROMPT, strlen(PROMPT));

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
				strcpy(buf, last_cmd);
				serial_write(buf, strlen(buf));
				i = strlen(buf);
			}
		}
		else if (c == 0x08 || c == 0x7f)
		{
			if (i != 0)
			{
				serial_write(BACKSPACE, sizeof(BACKSPACE));
				buf[--i] = 0;
			}
		}
		else if (c == 0x0d)
		{
			serial_write(NEWLINE, sizeof(NEWLINE));
			buf[i++] = 0;

			strcpy(last_cmd, buf);
			process_command(buf);

			i = 0;

			serial_write(PROMPT, strlen(PROMPT));
		}
		else if (c >= 0x20 && c <= 0x7e)
		{
			if (i == CMD_BUF_SIZE - 1)
			{
				continue;
			}

			serial_write(&c, 1);
			buf[i++] = c;
		}
		else if (c == 0x1b)
		{
			esc = true;
		}
	}

	return 0;
}

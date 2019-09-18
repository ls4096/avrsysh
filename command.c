#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "command.h"

#include "bricks.h"
#include "dump.h"
#include "grep.h"
#include "led.h"
#include "pm.h"
#include "pong.h"
#include "rng.h"
#include "seq.h"
#include "serial.h"
#include "snake.h"
#include "sp_mon.h"
#include "term.h"
#include "thermal.h"
#include "thread.h"
#include "time.h"
#include "timer.h"
#include "util.h"
#include "wc.h"

#define HELP_LINE_MAX_LENGTH 48

#define PC_PT_EXEC (0)
#define PC_PT_ALLOW_FIRST (1)
#define PC_PT_ALLOW_SECOND (2)

static const char* CMD_HELP = "help";
static const char* CMD_RESET = "reset";
static const char* CMD_STOP = "stop";
static const char* CMD_DUMP = "dump";
static const char* CMD_LED_ON = "led_on";
static const char* CMD_LED_OFF = "led_off";
static const char* CMD_SYS_INFO = "sysinfo";
static const char* CMD_TIME = "time";
static const char* CMD_SET_TIME = "settime";
static const char* CMD_CLEAR = "clear";
static const char* CMD_SLEEP = "sleep";
static const char* CMD_RAND = "rand";
static const char* CMD_SP_MON_ON = "spm_on";
static const char* CMD_SP_MON_OFF = "spm_off";
static const char* CMD_SP_MON_INFO = "spm_info";
static const char* CMD_PONG = "pong";
static const char* CMD_SNAKE = "snake";
static const char* CMD_BRICKS = "bricks";
static const char* CMD_GREP = "grep";
static const char* CMD_SEQ = "seq";
static const char* CMD_WC = "wc";

static char command_process_internal(unsigned char* cmd_str, char process_type);
static unsigned char* is_pipe_cmd(unsigned char* cmd_str);
static char process_second_command(unsigned char* cmd_str);
static void* get_entry_for_second(const unsigned char* cmd_str);
static bool begins_with_cmd(const char* str, const char* cmd);

static void pc_help();
static void help_print_f0(const char* s);
static void help_print_f1(const char* cmd);
static void help_print_f2(const char* cmd, const char* cmd_help);
static void help_print_f2a(const char* cmd, const char* cmd_help);
static void pc_led(bool set);
static void pc_sys_info();
static void pc_time();
static void pc_settime(const char* cmd_str);
static void pc_clear();
static void pc_sleep(const char* cmd_str);
static void pc_rand();
static void pc_sp_mon_enable(bool enable);
static void pc_sp_mon_info();


char command_process(unsigned char* cmd_str)
{
	return command_process_internal(cmd_str, PC_PT_EXEC);
}

const char* command_tab_complete(const char* cmd, unsigned short cmd_len, unsigned short* match_count)
{
	// All commands, ordered alphabetically
	const char* CMDS[] = {
		CMD_BRICKS,
		CMD_CLEAR,
		CMD_DUMP,
		CMD_GREP,
		CMD_HELP,
		CMD_LED_OFF,
		CMD_LED_ON,
		CMD_PONG,
		CMD_RAND,
		CMD_RESET,
		CMD_SEQ,
		CMD_SET_TIME,
		CMD_SLEEP,
		CMD_SNAKE,
		CMD_SP_MON_INFO,
		CMD_SP_MON_OFF,
		CMD_SP_MON_ON,
		CMD_STOP,
		CMD_SYS_INFO,
		CMD_TIME,
		CMD_WC
	};

	const char* last_match = 0;
	unsigned short matches = 0;
	unsigned short cmd_num = sizeof(CMDS) / sizeof(const char*);

	for (short i = 0; i < cmd_num; i++)
	{
		if (strncmp(cmd, CMDS[i], cmd_len) == 0)
		{
			matches++;

			if (matches == 2)
			{
				serial_write_newline();
				serial_write(last_match, strlen(last_match));
			}

			if (matches >= 2)
			{
				serial_tx_byte(' ');
				serial_tx_byte(' ');
				serial_tx_byte(' ');
				serial_write(CMDS[i], strlen(CMDS[i]));
			}

			last_match = CMDS[i];
		}
	}

	*match_count = matches;
	return ((matches == 1) ? last_match : 0);
}


static char command_process_internal(unsigned char* cmd_str, char process_type)
{
	if (strlen(cmd_str) == 0)
	{
		return (process_type == PC_PT_EXEC ? 0 : -1);
	}

	if (process_type == PC_PT_EXEC)
	{
		unsigned char* cmd_str_2 = is_pipe_cmd(cmd_str);
		if (cmd_str_2 != 0)
		{
			if (command_process_internal(cmd_str, PC_PT_ALLOW_FIRST) != 0 || process_second_command(cmd_str_2) != 0)
			{
				char buf[8];
				sprintf(buf, "invalid");
				serial_write(buf, strlen(buf));
				serial_write_newline();

				return 0;
			}
		}
	}

	if (begins_with_cmd(cmd_str, CMD_RESET))
	{
		return (process_type == PC_PT_EXEC ? PC_RC_RESET : -1);
	}
	else if (begins_with_cmd(cmd_str, CMD_STOP))
	{
		return (process_type == PC_PT_EXEC ? PC_RC_STOP : -1);
	}
	else if (begins_with_cmd(cmd_str, CMD_DUMP))
	{
		if (process_type == PC_PT_EXEC)
		{
			dump_state();
		}
		else
		{
			return -1;
		}
	}
	else if (begins_with_cmd(cmd_str, CMD_HELP))
	{
		switch (process_type)
		{
		case PC_PT_EXEC:
			pc_help();
			break;
		case PC_PT_ALLOW_FIRST:
			return 0;
		default:
			return -1;
		}
	}
	else if (begins_with_cmd(cmd_str, CMD_LED_ON))
	{
		if (process_type == PC_PT_EXEC)
		{
			pc_led(true);
		}
		else
		{
			return -1;
		}
	}
	else if (begins_with_cmd(cmd_str, CMD_LED_OFF))
	{
		if (process_type == PC_PT_EXEC)
		{
			pc_led(false);
		}
		else
		{
			return -1;
		}
	}
	else if (begins_with_cmd(cmd_str, CMD_SYS_INFO))
	{
		switch (process_type)
		{
		case PC_PT_EXEC:
			pc_sys_info();
			break;
		case PC_PT_ALLOW_FIRST:
			return 0;
		default:
			return -1;
		}
	}
	else if (begins_with_cmd(cmd_str, CMD_TIME))
	{
		switch (process_type)
		{
		case PC_PT_EXEC:
			pc_time();
			break;
		case PC_PT_ALLOW_FIRST:
			return 0;
		default:
			return -1;
		}
	}
	else if (begins_with_cmd(cmd_str, CMD_SET_TIME))
	{
		switch (process_type)
		{
		case PC_PT_EXEC:
			pc_settime(cmd_str);
			break;
		case PC_PT_ALLOW_FIRST:
			return 0;
		default:
			return -1;
		}
	}
	else if (begins_with_cmd(cmd_str, CMD_CLEAR))
	{
		if (process_type == PC_PT_EXEC)
		{
			pc_clear();
		}
		else
		{
			return -1;
		}
	}
	else if (begins_with_cmd(cmd_str, CMD_SLEEP))
	{
		if (process_type == PC_PT_EXEC)
		{
			pc_sleep(cmd_str);
		}
		else
		{
			return -1;
		}
	}
	else if (begins_with_cmd(cmd_str, CMD_RAND))
	{
		switch (process_type)
		{
		case PC_PT_EXEC:
			pc_rand();
			break;
		case PC_PT_ALLOW_FIRST:
			return 0;
		default:
			return -1;
		}
	}
	else if (begins_with_cmd(cmd_str, CMD_SP_MON_ON))
	{
		if (process_type == PC_PT_EXEC)
		{
			pc_sp_mon_enable(true);
		}
		else
		{
			return -1;
		}
	}
	else if (begins_with_cmd(cmd_str, CMD_SP_MON_OFF))
	{
		if (process_type == PC_PT_EXEC)
		{
			pc_sp_mon_enable(false);
		}
		else
		{
			return -1;
		}
	}
	else if (begins_with_cmd(cmd_str, CMD_SP_MON_INFO))
	{
		switch (process_type)
		{
		case PC_PT_EXEC:
			pc_sp_mon_info();
			break;
		case PC_PT_ALLOW_FIRST:
			return 0;
		default:
			return -1;
		}
	}
	else if (begins_with_cmd(cmd_str, CMD_PONG))
	{
		if (process_type == PC_PT_EXEC)
		{
			pong_main();
		}
		else
		{
			return -1;
		}
	}
	else if (begins_with_cmd(cmd_str, CMD_SNAKE))
	{
		if (process_type == PC_PT_EXEC)
		{
			snake_main();
		}
		else
		{
			return -1;
		}
	}
	else if (begins_with_cmd(cmd_str, CMD_BRICKS))
	{
		if (process_type == PC_PT_EXEC)
		{
			bricks_main();
		}
		else
		{
			return -1;
		}
	}
	else if (begins_with_cmd(cmd_str, CMD_GREP))
	{
		switch (process_type)
		{
		case PC_PT_EXEC:
			grep_main(cmd_str);
			break;
		case PC_PT_ALLOW_FIRST:
		case PC_PT_ALLOW_SECOND:
			return 0;
		default:
			return -1;
		}
	}
	else if (begins_with_cmd(cmd_str, CMD_SEQ))
	{
		switch (process_type)
		{
		case PC_PT_EXEC:
			seq_main(cmd_str);
			break;
		case PC_PT_ALLOW_FIRST:
			return 0;
		default:
			return -1;
		}
	}
	else if (begins_with_cmd(cmd_str, CMD_WC))
	{
		switch (process_type)
		{
		case PC_PT_EXEC:
			wc_main(cmd_str);
			break;
		case PC_PT_ALLOW_FIRST:
		case PC_PT_ALLOW_SECOND:
			return 0;
		default:
			return -1;
		}
	}
	else
	{
		if (process_type != PC_PT_EXEC)
		{
			return -1;
		}

		char buf[20];
		sprintf(buf, "?: Try \'%s\'.", CMD_HELP);
		serial_write(buf, strlen(buf));
		serial_write_newline();
	}

	if (thread_is_running())
	{
		thread_join();
	}

	return 0;
}

static unsigned char* is_pipe_cmd(unsigned char* cmd_str)
{
	while (*cmd_str != 0x00)
	{
		if (*cmd_str == '|')
		{
			return cmd_str + 1;
		}
		cmd_str++;
	}

	return 0;
}

static char process_second_command(unsigned char* cmd_str)
{
	while (*cmd_str == ' ')
	{
		cmd_str++;
	}

	if (*cmd_str == 0x00)
	{
		return -1;
	}

	if (command_process_internal(cmd_str, PC_PT_ALLOW_SECOND) != 0)
	{
		return -1;
	}

	if (thread_is_running())
	{
		dump_state();
	}

	thread_entry_func entry_func = get_entry_for_second(cmd_str);
	if (entry_func == 0)
	{
		dump_state();
	}

	thread_create(entry_func, cmd_str);

	return 0;
}

static void* get_entry_for_second(const unsigned char* cmd_str)
{
	if (begins_with_cmd(cmd_str, CMD_GREP))
	{
		return &grep_main;
	}
	else if (begins_with_cmd(cmd_str, CMD_WC))
	{
		return &wc_main;
	}

	return 0;
}

static bool begins_with_cmd(const char* str, const char* cmd)
{
	const unsigned short cmd_len = strlen(cmd);

	return (strncmp(str, cmd, cmd_len) == 0 && (str[cmd_len] == 0x00 || str[cmd_len] == ' ' || str[cmd_len] == '|'));
}

static void pc_help()
{
	// General
	help_print_f0("General:");
	help_print_f1(CMD_HELP);
	help_print_f2(CMD_SYS_INFO, "show system info");
	help_print_f2(CMD_CLEAR, "clear screen");
	help_print_f2a(CMD_SLEEP, "N: sleep N seconds");
	help_print_f2(CMD_RAND, "get random number");
	help_print_f1(CMD_LED_ON);
	help_print_f1(CMD_LED_OFF);
	help_print_f1(CMD_RESET);
	help_print_f1(CMD_STOP);

	// Time
	help_print_f0("Time:");
	help_print_f1(CMD_TIME);
	help_print_f2a(CMD_SET_TIME, "HH:MM:SS");

	// Stack Pointer Monitor
	help_print_f0("SP Monitor:");
	help_print_f2(CMD_SP_MON_ON, "start");
	help_print_f2(CMD_SP_MON_OFF, "stop");
	help_print_f2(CMD_SP_MON_INFO, "show results");

	// Games
	help_print_f0("Games:");
	help_print_f1(CMD_PONG);
	help_print_f1(CMD_SNAKE);
	help_print_f1(CMD_BRICKS);

	// Utils
	help_print_f0("Utils:");
	help_print_f2a(CMD_GREP, "S");
	help_print_f2a(CMD_SEQ, "X Y");
	help_print_f1(CMD_WC);
}

static void help_print_f0(const char* s)
{
	serial_write(s, strlen(s));
	serial_write_newline();
}

static void help_print_f1(const char* cmd)
{
	char buf[HELP_LINE_MAX_LENGTH];

	sprintf(buf, " %s", cmd);
	serial_write(buf, strlen(buf));
	serial_write_newline();
}

static void help_print_f2(const char* cmd, const char* cmd_help)
{
	char buf[HELP_LINE_MAX_LENGTH];

	sprintf(buf, " %s: %s", cmd, cmd_help);
	serial_write(buf, strlen(buf));
	serial_write_newline();
}

static void help_print_f2a(const char* cmd, const char* cmd_help)
{
	char buf[HELP_LINE_MAX_LENGTH];

	sprintf(buf, " %s %s", cmd, cmd_help);
	serial_write(buf, strlen(buf));
	serial_write_newline();
}

static void pc_led(bool set)
{
	if (set)
	{
		led_on();
	}
	else
	{
		led_off();
	}
}

static void pc_sys_info()
{
	char buf[40];

	unsigned short ticks[2];
	timer_get_tick_count(ticks);

	// TODO: Fix? This will overflow after 65535 seconds.
	short s = ticks[0] * TIMER_SECONDS_PER_UPPER_TICK + (ticks[1] / TIMER_TICKS_PER_SECOND);

	sprintf(buf, "uptime: %u s\r\n", s);
	serial_write(buf, strlen(buf));

	unsigned short w[2];
	pm_get_wake_count(w);

	sprintf(buf, "recent CPU usage: %u/%u\r\n", w[0], w[1]);
	serial_write(buf, strlen(buf));

	sprintf(buf, "ticks: 0x%04x 0x%04x\r\n", ticks[0], ticks[1]);
	serial_write(buf, strlen(buf));

	sprintf(buf, "ticks/s: %u\r\n", TIMER_TICKS_PER_SECOND);
	serial_write(buf, strlen(buf));

	sprintf(buf, "notify timers: %u\r\n", timer_get_notify_registered_count());
	serial_write(buf, strlen(buf));

	sprintf(buf, "thread switches: %u\r\n", thread_switch_count());
	serial_write(buf, strlen(buf));

	short temp = thermal_read_temperature();
	sprintf(buf, "internal temp: ", temp);
	serial_write(buf, strlen(buf));
	if (temp != THERMAL_TEMP_NONE)
	{
		sprintf(buf, "%d C", temp);
	}
	else
	{
		sprintf(buf, "N/A");
	}
	serial_write(buf, strlen(buf));
	serial_write_newline();
}

static void pc_time()
{
	char buf[10];
	if (!time_is_set())
	{
		sprintf(buf, "not set\r\n");
	}
	else
	{
		char timebuf[10];
		time_get_time(timebuf);
		sprintf(buf, "%s\r\n", timebuf);
	}

	serial_write(buf, strlen(buf));
}

static void pc_settime(const char* cmd_str)
{
	char buf[40];
	if (strlen(cmd_str) < strlen(CMD_SET_TIME) + 1 + 8)
	{
		sprintf(buf, "invalid format\r\n");
	}
	else
	{
		if (time_set_time(cmd_str + strlen(CMD_SET_TIME) + 1))
		{
			sprintf(buf, "time set: %s\r\n", cmd_str + strlen(CMD_SET_TIME) + 1);
		}
		else
		{
			sprintf(buf, "time not set\r\n");
		}
	}

	serial_write(buf, strlen(buf));
}

static void pc_clear()
{
	term_clear_screen();
}

static void pc_sleep(const char* cmd_str)
{
	if (strlen(cmd_str) < strlen(CMD_SLEEP) + 1 + 1 ||
		strlen(cmd_str) > strlen(CMD_SLEEP) + 1 + 4)
	{
		return;
	}

	short i;

	const unsigned short nlen = strlen(cmd_str) - strlen(CMD_SLEEP) - 1;
	unsigned short mul = 1;
	for (i = 1; i < nlen; i++)
	{
		mul *= 10;
	}

	unsigned short sleep_sec = 0;
	const char* cmd_str_n = cmd_str + strlen(CMD_SLEEP) + 1;
	for (i = 0; i < nlen; i++)
	{
		if (!util_is_numeric(cmd_str_n[i]))
		{
			return;
		}
		sleep_sec += ((cmd_str_n[i] - '0') * mul);
		mul /= 10;
	}

	timer_notify_t notify;
	timer_get_tick_count(notify.t);
	timer_add_seconds(notify.t, sleep_sec);
	timer_notify_register(&notify);

	while (!notify.notify)
	{
		pm_yield();
	}
}

static void pc_rand()
{
	char buf[10];
	short r = rng_rand();
	sprintf(buf, "%d\r\n", r);
	serial_write(buf, strlen(buf));
}

static void pc_sp_mon_enable(bool enable)
{
	sp_mon_enable(enable);
}

static void pc_sp_mon_info()
{
	volatile unsigned short* buckets = sp_mon_get_buckets();

	char buf[64];
	char bar[34];

	bool show = false;
	short i;
	for (i = 0; i < SP_MON_NUM_BUCKETS; i++)
	{
		if (buckets[i] == 0 && !show)
		{
			continue;
		}
		show = true;

		unsigned short b = buckets[i];
		short j = 0;
		while (b != 0)
		{
			bar[j++] = '#';
			bar[j++] = '#';
			b >>= 1;
		}
		if (j > 0)
		{
			bar[j++] = ' ';
		}
		bar[j] = 0;

		sprintf(buf,
			"0x%04x-0x%04x: %s%u\r\n",
			i * (1 << SP_MON_BUCKET_SIZE_BITS),
			(i + 1) * (1 << SP_MON_BUCKET_SIZE_BITS) - 1,
			bar,
			buckets[i]);
		serial_write(buf, strlen(buf));
	}

	if (!show)
	{
		sprintf(buf, "no data\r\n");
		serial_write(buf, strlen(buf));
	}
}

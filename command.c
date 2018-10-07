#include <stdbool.h>

#include "command.h"

#include "led.h"
#include "pm.h"
#include "pong.h"
#include "serial.h"
#include "sp_mon.h"
#include "thermal.h"
#include "time.h"
#include "timer.h"
#include "util.h"

#define CMD_HELP "help"
#define CMD_LED_ON "led_on"
#define CMD_LED_OFF "led_off"
#define CMD_SYS_INFO "sysinfo"
#define CMD_TIME "time"
#define CMD_SET_TIME "settime"
#define CMD_CLEAR "clear"
#define CMD_SLEEP "sleep"
#define CMD_SP_MON_ON "spm_on"
#define CMD_SP_MON_OFF "spm_off"
#define CMD_SP_MON_INFO "spm_info"
#define CMD_PONG "pong"

static void pc_help();
static void pc_led(bool set);
static void pc_sys_info();
static void pc_time();
static void pc_settime(const char* cmd_str);
static void pc_clear();
static void pc_sleep(const char* cmd_str);
static void pc_sp_mon_enable(bool enable);
static void pc_sp_mon_info();
static void pc_pong();

void process_command(unsigned char* cmd_str)
{
	if (strlen(cmd_str) == 0)
	{
		return;
	}

	if (strcmp(cmd_str, CMD_HELP) == 0)
	{
		pc_help();
	}
	else if (strcmp(cmd_str, CMD_LED_ON) == 0)
	{
		pc_led(true);
	}
	else if (strcmp(cmd_str, CMD_LED_OFF) == 0)
	{
		pc_led(false);
	}
	else if (strcmp(cmd_str, CMD_SYS_INFO) == 0)
	{
		pc_sys_info();
	}
	else if (strcmp(cmd_str, CMD_TIME) == 0)
	{
		pc_time();
	}
	else if (strncmp(cmd_str, CMD_SET_TIME, strlen(CMD_SET_TIME)) == 0)
	{
		pc_settime(cmd_str);
	}
	else if (strcmp(cmd_str, CMD_CLEAR) == 0)
	{
		pc_clear();
	}
	else if (strncmp(cmd_str, CMD_SLEEP, strlen(CMD_SLEEP)) == 0)
	{
		pc_sleep(cmd_str);
	}
	else if (strcmp(cmd_str, CMD_SP_MON_ON) == 0)
	{
		pc_sp_mon_enable(true);
	}
	else if (strcmp(cmd_str, CMD_SP_MON_OFF) == 0)
	{
		pc_sp_mon_enable(false);
	}
	else if (strcmp(cmd_str, CMD_SP_MON_INFO) == 0)
	{
		pc_sp_mon_info();
	}
	else if (strcmp(cmd_str, CMD_PONG) == 0)
	{
		pc_pong();
	}
	else
	{
		static const char* UNKNOWN_COMMAND_RESPONSE = "???: Try \'" CMD_HELP "\'.\r\n";
		serial_write(UNKNOWN_COMMAND_RESPONSE, strlen(UNKNOWN_COMMAND_RESPONSE));
	}
}

static void pc_help()
{
	static const char* HELP_RESPONSE = \
		"General:\r\n" \
		" " CMD_HELP		": show help\r\n" \
		" " CMD_SYS_INFO	": show system info\r\n" \
		" " CMD_CLEAR		": clear screen\r\n" \
		" " CMD_SLEEP		" N: sleep N seconds\r\n" \
		" " CMD_LED_ON		"\r\n" \
		" " CMD_LED_OFF		"\r\n" \
		"Time:\r\n" \
		" " CMD_TIME		"\r\n" \
		" " CMD_SET_TIME	" HH:MM:SS\r\n" \
		"Stack Pointer Monitor:\r\n" \
		" " CMD_SP_MON_ON	": start\r\n" \
		" " CMD_SP_MON_OFF	": stop\r\n" \
		" " CMD_SP_MON_INFO	": show results\r\n" \
		"Games:\r\n" \
		" " CMD_PONG		"\r\n";
	serial_write(HELP_RESPONSE, strlen(HELP_RESPONSE));
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
	timer_get_tick_count(&ticks);

	// TODO: Fix? This will overflow after 65535 seconds.
	short s = ticks[0] * TIMER_SECONDS_PER_UPPER_TICK + (ticks[1] / TIMER_TICKS_PER_SECOND);

	sprintf(buf, "uptime: %u seconds\r\n", s);
	serial_write(buf, strlen(buf));

	unsigned short w[2];
	pm_get_wake_count(&w);

	sprintf(buf, "recent CPU usage: %u/%u\r\n", w[0], w[1]);
	serial_write(buf, strlen(buf));

	sprintf(buf, "ticks: 0x%04x 0x%04x\r\n", ticks[0], ticks[1]);
	serial_write(buf, strlen(buf));

	sprintf(buf, "ticks/sec: %u\r\n", TIMER_TICKS_PER_SECOND);
	serial_write(buf, strlen(buf));

	sprintf(buf, "notify timers: %u\r\n", timer_get_notify_registered_count());
	serial_write(buf, strlen(buf));

	short temp = thermal_read_temperature();
	sprintf(buf, "internal temp: %d C\r\n", temp);
	serial_write(buf, strlen(buf));
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
	static const unsigned char CLEAR[7] = { 0x1b, '[', '2', 'J', 0x1b, '[', 'H' };
	serial_write(CLEAR, sizeof(CLEAR));
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

static void pc_sp_mon_enable(bool enable)
{
	sp_mon_enable(enable);
}

static void pc_sp_mon_info()
{
	unsigned short* buckets = sp_mon_get_buckets();

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

static void pc_pong()
{
	pong_init();
	pong_start();
}

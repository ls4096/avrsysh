#include <avr/io.h>
#include <avr/interrupt.h>

#include "serial.h"

#include "pm.h"
#include "rng.h"
#include "timer.h"

#ifndef F_CPU
	#error F_CPU not defined
#endif

#define RX_BUF_SIZE 32

#define BAUDRATE 57600
#define UBRR (F_CPU / 16 / BAUDRATE)


static volatile unsigned char _rx_buf[RX_BUF_SIZE];
static volatile unsigned char _rx_buf_next_read = 0;
static volatile unsigned char _rx_buf_next_write = 0;


ISR(USART_RX_vect)
{
	unsigned char c = UDR0;
	if (((_rx_buf_next_write + 1) % RX_BUF_SIZE) == _rx_buf_next_read)
	{
		return;
	}

	_rx_buf[_rx_buf_next_write] = c;
	_rx_buf_next_write = ((_rx_buf_next_write + 1) % RX_BUF_SIZE);

	rng_add_entropy(timer_get_tick_count_lsbyte());
}


void serial_init()
{
	UBRR0H = 0;
	UBRR0L = UBRR;
	UCSR0B = (1 << TXEN0) | (1 << RXEN0) | (1 << RXCIE0);
	UCSR0C = (1 << USBS0) | (3 << UCSZ00);
}

bool serial_has_next_byte()
{
	return !(_rx_buf_next_read == _rx_buf_next_write);
}

unsigned char serial_read_next_byte()
{
	while (!serial_has_next_byte())
	{
		pm_yield();
	}

	unsigned char c = _rx_buf[_rx_buf_next_read];
	_rx_buf_next_read = ((_rx_buf_next_read + 1) % RX_BUF_SIZE);

	return c;
}

void serial_write(unsigned char* data, short len)
{
	short i;
	for (i = 0; i < len; i++)
	{
		serial_tx_byte(data[i]);
	}
}

void serial_tx_byte(unsigned char data)
{
	while (!(UCSR0A & (1 << UDRE0))) { }
	UDR0 = data;
}

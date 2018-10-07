#include <avr/io.h>

#include "thermal.h"

void thermal_init()
{
	PRR &= ~(1 << PRADC);
	ADMUX = (1 << REFS1) | (1 << REFS0) | (1 << MUX3);
	ADCSRA |= (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

short thermal_read_temperature()
{
	ADCSRA |= (1 << ADSC);

	while ((ADCSRA & (1 << ADSC)) != 0) { }

	short adcr;
	short t;

	adcr = ADCL;
	adcr |= (ADCH << 8);

	// An approximate conversion to degrees C.
	adcr -= 324;
	t = adcr;
	t *= 55;
	t -= (adcr * 10);
	t /= 55;

	return t;
}

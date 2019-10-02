#include <avr/io.h>

#include "thermal.h"

#include "avr_mcu.h"

static void thermal_init_hw();

void thermal_init()
{
	thermal_init_hw();
}

short thermal_read_temperature()
{
#ifdef THERMAL_SUPPORT
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
#else
	return THERMAL_TEMP_NONE;
#endif
}

static void thermal_init_hw()
{
#ifdef THERMAL_SUPPORT

#if (defined AVRSYSH_MCU_328P)
	PRR &= ~(1 << PRADC);
	ADMUX = (1 << REFS1) | (1 << REFS0) | (1 << MUX3);
	ADCSRA |= (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
#else
	#error "MCU type not defined or not supported!"
#endif

#endif // THERMAL_SUPPORT
}

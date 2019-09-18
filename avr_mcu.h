#ifndef _AVR_MCU_H_
#define _AVR_MCU_H_

#if defined (__AVR_ATmega328P__)
#include "avr_mcu/328p.h"
#elif defined (__AVR_ATmega32U4__)
#include "avr_mcu/32u4.h"
#elif defined (__AVR_ATmega2560__)
#include "avr_mcu/2560.h"
#else
#error "AVR MCU type not defined or not supported!"
#endif

unsigned char avr_mcu_pc_size_bytes();

#endif // _AVR_MCU_H_

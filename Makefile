all: avrprog.hex

OBJS = \
	command.o \
	led.o \
	main.o \
	pm.o \
	pong.o \
	rng.o \
	serial.o \
	sp_mon.o \
	thermal.o \
	time.o \
	timer.o \
	util.o

%.o: %.c
	$(AVR_TOOLS_DIR)/bin/avr-gcc -c -g -Os -w -ffunction-sections -fdata-sections -mmcu=atmega328p -DF_CPU=16000000L -o $@ $<

avrprog.elf: $(OBJS)
	$(AVR_TOOLS_DIR)/bin/avr-gcc -Os -Wl,--gc-sections -mmcu=atmega328p -o avrprog.elf *.o -lm
	$(AVR_TOOLS_DIR)/bin/avr-size avrprog.elf

avrprog.hex: avrprog.elf
	$(AVR_TOOLS_DIR)/bin/avr-objcopy -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load --no-change-warnings --change-section-lma .eeprom=0 avrprog.elf avrprog.eep
	$(AVR_TOOLS_DIR)/bin/avr-objcopy -O ihex -R .eeprom avrprog.elf avrprog.hex
	$(AVR_TOOLS_DIR)/bin/avr-size avrprog.hex

clean:
	rm -rf *.o *.hex *.elf *.asm *.eep

flash: avrprog.hex
	$(AVR_TOOLS_DIR)/bin/avrdude -C $(AVR_TOOLS_DIR)/etc/avrdude.conf -p atmega328p -c stk500v1 -P $(AVR_FLASH_PORT) -b57600 -D -Uflash:w:avrprog.hex:i

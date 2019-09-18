all: avrsysh.hex

OBJS = \
	avr_mcu.o \
	bricks.o \
	command.o \
	draw.o \
	dump.o \
	grep.o \
	led.o \
	main.o \
	pm.o \
	pong.o \
	rng.o \
	seq.o \
	serial.o \
	snake.o \
	sp_mon.o \
	term.o \
	thermal.o \
	thread.o \
	time.o \
	timer.o \
	util.o \
	wc.o

%.o: %.c
	$(AVR_TOOLS_DIR)/bin/avr-gcc -c -Os -mmcu=$(AVR_MCU) -DF_CPU=16000000L -o $@ $<

%.o: %.S
	$(AVR_TOOLS_DIR)/bin/avr-as -c -mmcu=$(AVR_MCU) -o $@ $<

avrsysh.elf: $(OBJS)
	$(AVR_TOOLS_DIR)/bin/avr-gcc -Os -mmcu=$(AVR_MCU) -o avrsysh.elf *.o -lm
	$(AVR_TOOLS_DIR)/bin/avr-size avrsysh.elf

avrsysh.hex: avrsysh.elf
	$(AVR_TOOLS_DIR)/bin/avr-objcopy -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load --no-change-warnings --change-section-lma .eeprom=0 avrsysh.elf avrsysh.eep
	$(AVR_TOOLS_DIR)/bin/avr-objcopy -O ihex -R .eeprom avrsysh.elf avrsysh.hex
	$(AVR_TOOLS_DIR)/bin/avr-size avrsysh.hex

clean:
	rm -rf *.o *.hex *.elf *.asm *.eep

flash: avrsysh.hex
	$(AVR_TOOLS_DIR)/bin/avrdude -C $(AVR_TOOLS_DIR)/etc/avrdude.conf -p $(AVR_MCU) -c stk500v1 -P $(AVR_FLASH_PORT) -b57600 -D -Uflash:w:avrsysh.hex:i

flash_uno: avrsysh.hex
	$(AVR_TOOLS_DIR)/bin/avrdude -C $(AVR_TOOLS_DIR)/etc/avrdude.conf -p $(AVR_MCU) -c arduino -P $(AVR_FLASH_PORT) -b115200 -D -Uflash:w:avrsysh.hex:i

flash_2560: avrsysh.hex
	$(AVR_TOOLS_DIR)/bin/avrdude -C $(AVR_TOOLS_DIR)/etc/avrdude.conf -p $(AVR_MCU) -c wiring -P $(AVR_FLASH_PORT) -b115200 -D -Uflash:w:avrsysh.hex:i

flash_32u4: avrsysh.hex
	$(AVR_TOOLS_DIR)/bin/avrdude -C $(AVR_TOOLS_DIR)/etc/avrdude.conf -p $(AVR_MCU) -c avr109 -P $(AVR_FLASH_PORT) -b57600 -D -Uflash:w:avrsysh.hex:i

#MCU=atmega324pa
#MCU=at90usb1286	# Teensy 2.0++
MCU=atmega32u4		# Teensy 2.0
#CPUFREQ=8000000
#CPUFREQ=20000000
CPUFREQ=16000000
LOADER=teensy

#LOADER=avrdude
AVRDUDE_PORT=/dev/cuaU1
AVRDUDE_PART=m324pa # m328p m324pa t85 t861 t2313
AVRDUDE_HW=buspirate
AVRDUDE_EXTRA=-xspeed=7 -V

OPT=-Os

WARNFLAGS=-Wall -Wextra 
WARNFLAGS+=-Werror
WARNFLAGS+=-Wno-type-limits -Wno-unused -Wno-unused-parameter
WARNFLAGS+=-Wno-missing-field-initializers

CFLAGS=-mmcu=${MCU} -DF_CPU=${CPUFREQ}UL ${WARNFLAGS} ${OPT} -std=gnu99
CFLAGS+=-funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
CFLAGS+=-g

LIBAVR_OBJS=spi.o ad56x8.o encoder.o event.o
LIBAVR_OBJS+=midi.o lcd.o num_format.o menu.ui.o ui.o config.o
#LIBAVR_OBJS+=demux.o mcp23s1x.o ui.o rgbled.o

HOST_CC=clang-3.6
HOST_CFLAGS=--std=c99 -Weverything -g -Wno-unused-parameter -Werror

CC=avr-gcc
OBJCOPY=avr-objcopy

all: firmware.hex fakeui

fakeui: fakeui.c fakelcd.c event.c
	${HOST_CC} ${HOST_CFLAGS} -o $@ fakeui.c fakelcd.c event.c -lncurses

firmware.elf: main.o ${LIBAVR_OBJS}
	${CC} ${CFLAGS} -o $@ main.o ${LIBAVR_OBJS}

firmware.hex: firmware.elf
	${OBJCOPY} -j .text -j .data -O ihex firmware.elf $@

load: ${LOADER}

teensy: firmware.hex
	${SUDO} teensy_loader_cli -v -w -mmcu=${MCU} firmware.hex

avrdude: firmware.hex
	avrdude -P ${AVRDUDE_PORT} -p ${AVRDUDE_PART} -c ${AVRDUDE_HW} \
	    ${AVRDUDE_EXTRA} -e -U flash:w:firmware.hex

clean:
	rm -f *.elf *.hex *.o *.core *.hex

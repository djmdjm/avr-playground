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

# we need ncurses built with --enable-ext-mouse to support mouse-wheel.
# Most distributions don't do this unfortunately.
HOST_NCURSES_PATH=${HOME}/tmp/ncurses

HOST_CC=clang-3.6
HOST_CFLAGS=--std=c99 -Wextra -g -Wno-unused-parameter -Werror \
    -I${HOST_NCURSES_PATH}/include -I${HOST_NCURSES_PATH}/include/ncurses
HOST_LIBS=${HOST_NCURSES_PATH}/lib/libncurses.a

FAKEUI_SRCS=fakeui.c fakelcd.c event.c num_format.c menu.ui.c config.c

CC=avr-gcc
OBJCOPY=avr-objcopy

all: firmware.hex fakeui uigen/uigen

uigen/uigen:
	${MAKE} -C uigen

menu.ui.c: uigen/uigen menu.def uigen/uidata.c.t
	uigen/uigen --template=uigen/uidata.c.t menu.def > menu.ui.c

menu.ui.h: uigen/uigen menu.def uigen/uidata.c.t
	uigen/uigen --template=uigen/uidata.c.t --header --header-guard=_MENU \
	    menu.def > menu.ui.h

menu.ui.o: menu.ui.c menu.ui.h

fakeui: ${FAKEUI_SRCS} menu.ui.h
	${HOST_CC} ${HOST_CFLAGS} -o $@ ${FAKEUI_SRCS} ${HOST_LIBS}

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
	rm -f *.elf *.hex *.o *.core *.hex fakeui menu.ui.c menu.ui.h
	${MAKE} -C uigen clean

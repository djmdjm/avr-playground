/*
 * Copyright (c) 2013 Damien Miller <djm@mindrot.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "rgbled.h"
#include "demux.h"
#include "lcd.h"
#include "num_format.h"
#include "spi.h"
#include "ad56x8.h"
#include "encoder.h"
#include "event.h"
#include "event-types.h"
#include "mcp23s1x.h"
#include "midi.h"
#include "ui.h"

/* Interrupt handler for encoder and its pushbutton */
ISR(PCINT0_vect) 
{
	static uint8_t pb = 1; /* Active high */
	uint8_t newpb = (PINB >> 6) & 1;

	encoder_interrupt();
	if (newpb != pb) {
		event_enqueue(EV_BUTTON, 0, newpb == 0, 0, 0);
		pb = newpb;
	}
}

/* Serial RX interrupt handler to plumb data to MIDI decoder */
ISR(USART1_RX_vect)
{
	uint8_t rx;

	if ((UCSR1A & (1<<RXC1)) != 0) {
		rx = UDR1;
		midi_in(rx);
	}
}

static void
trig_out(uint8_t i, uint8_t on)
{
	switch (i) {
	case 0:
		PORTB = (PORTB & ~(1 << 7)) | (on ? 1 << 7 : 0);
		break;
	case 1:
		PORTD = (PORTD & ~(1 << 0)) | (on ? 1 << 0 : 0);
		break;
	case 2:
		PORTD = (PORTD & ~(1 << 1)) | (on ? 1 << 1 : 0);
		break;
	case 3:
		PORTD = (PORTD & ~(1 << 3)) | (on ? 1 << 3 : 0);
		break;
	case 4:
		PORTC = (PORTC & ~(1 << 6)) | (on ? 1 << 6 : 0);
		break;
	case 5:
		PORTC = (PORTC & ~(1 << 7)) | (on ? 1 << 7 : 0);
		break;
	}
}

int
main(void)
{
	uint8_t i, j, ev_type, ev_v1, ev_v2, ev_v3;
	uint16_t dac;

	CLKPR = 0x80;
	CLKPR = 0x00; /* 16 MHz */

	/* Prepare serial */
	DDRD &= ~(1 << 2);	/* rxd */
	PORTD |= 1 << 2;
	UCSR1A = 0;
	UCSR1B = 1 << RXEN1;	/* don't need xmit for now */
	UCSR1C= 3 << UCSZ10;	/* 8-N-1 */
	UBRR1H = 0;
	UBRR1L = 31;		/* MIDI 31250 baud (use 103 for 9600) */

	/* Prepare LCD backlight */
	DDRD |= (1 << 6);
	PORTD |= (1 << 6);

	event_setup();

	lcd_setup();
	lcd_clear();
	lcd_display(1, 0, 1);
	lcd_string("OK ");
	lcd_moveto(0, 0);

	spi_setup();
	lcd_string("OK2");
	ad56x8_setup(1);
	lcd_string("OK3");
	midi_init(0xffff);	/* XXX Omni for now */
	encoder_setup();
	sei();

	/* Prepare trigger outputs */
	DDRB |= 1 << 7;
	DDRD |= (1 << 0) | (1 << 1) | (1 << 3);
	DDRC |= (1 << 6) | (1 << 7);
	for (i = 0; i < 6; i++)
		trig_out(i, 0);

	/* Prepare DAC */
	lcd_string("OK4");

	/* Enable UART RX interrupts */
	UCSR1B|=(1<<RXCIE1);

	/* Enable encoder pushbutton interrupt */
	PCMSK0 |= PCINT6;

	/* Zero DAC */
	for (i = 0; i < 8; i++)
		ad56x8_write_update(0, 0x8000);

	for (i = 0; ; i++) {
		/* XXX sleep? */
		if (event_dequeue(&ev_type, &ev_v1, &ev_v2, &ev_v3)) {
			lcd_moveto(0, 0);
			lcd_string(ntoh(ev_type, 0));
			lcd_string(" ");
			lcd_string(ntoh(ev_v1, 0));
			lcd_string(" ");
			lcd_string(ntoh(ev_v2, 0));
			lcd_string(" ");
			lcd_string(ntoh(ev_v3, 0));
			lcd_string(" ");
			lcd_clear_eol();
			switch (ev_type) {
			case EV_ENCODER:
			case EV_BUTTON:
				break;
			case EV_MIDI_NOTE_ON:
				/* v1 = chan, v2 = note, v3 = velocity */
				dac = 0x8000 + ((ev_v2 - 33) * (3276/12));
				lcd_moveto(0, 1);
				lcd_string(ntod(ev_v2));
				lcd_char(' ');
				lcd_string(ntod(dac));
				lcd_clear_eol();
				ad56x8_write_update(0, dac);
				trig_out(0, 1);
				break;
			case EV_MIDI_NOTE_OFF:
			case EV_MIDI_ALL_OFF:
			case EV_MIDI_CONTROL_RESET:
				trig_out(0, 0);
				break;
			}

		}
	}
	/* NOTREACHED */
}

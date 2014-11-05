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
#include <util/delay.h>
#include <util/atomic.h>
#include <stdint.h>

#include "encoder.h"
#include "event.h"
#include "event-types.h"

#define ENC_MASK		((1 << ENC_PIN_A) | (1 << ENC_PIN_B))
#define ENC_INT_MASK		((1 << ENC_INT_A) | (1 << ENC_INT_B))

static int16_t enc_value;

void
encoder_setup(void)
{
	/* Pins -> input */
	ENC_DDR &= ~ENC_MASK;
	encoder_interrupt_enable();
}

void
encoder_interrupt_enable(void)
{
	/* Configure interrupts. XXX assumes nothing fiddles trig mode. */
	ENC_PCMSK |= ENC_INT_MASK;
	PCICR |= (1 << ENC_PCIE);
}

void
encoder_interrupt_disable(void)
{
	ENC_PCMSK &= ~ENC_INT_MASK;
	PCICR &= ~(1 << ENC_PCIE);
}

void
encoder_interrupt(void)
{
	static uint8_t code = 0x03;
	static int8_t decode_lut[] = {
		 0, -1,  1,  0,
		 1,  0,  0, -1,
		-1,  0,  0,  1,
		 0,  1, -1,  0,
	};
	static int pulses = 0;

	_delay_us(10); /* Allow input to settle a bit */
	code = ((code << 2) & 0xf) |
	    ((ENC_PIN & (1 << ENC_PIN_A)) ? 0x02 : 0x00) |
	    ((ENC_PIN & (1 << ENC_PIN_B)) ? 0x01 : 0x00);
	pulses += decode_lut[code];

	if (pulses >= ENC_PULSE_PER_DETENT) {
		if (enc_value < INT16_MAX)
			enc_value++;
		event_enqueue(EV_ENCODER, 1, 0, 0, 0);
		pulses = 0;
	} else if (pulses <= -ENC_PULSE_PER_DETENT) {
		if (enc_value > INT16_MIN)
			enc_value--;
		event_enqueue(EV_ENCODER, 0, 0, 0, 0);
		pulses = 0;
	}
}

#ifdef ENC_INTERRUPT_HANDLER
ISR(ENC_VECT)
{
	encoder_interrupt();
}
#endif

int16_t
encoder_value(void)
{
	int16_t r;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		r = enc_value;
	}
	return r;
}

/* Sets the running counter value */
void
encoder_setvalue(int16_t v)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		enc_value = v;
	}
}


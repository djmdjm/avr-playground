/*
 * Copyright (c) 2014 Damien Miller <djm@mindrot.org>
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

#include "lcd.h"
#include "num_format.h"
#include "encoder.h"
#include "event.h"
#include "event-types.h"
#include "ui.h"

static int editing = 0;
static int button_down = 0;
static int position = -1;

int
config_edit(uint8_t ev_type, uint8_t ev_v1, uint8_t ev_v2, uint8_t ev_v3)
{
	/* UI is only interested in UI events */
	switch (ev_type) {
	case EV_ENCODER:
	case EV_BUTTON:
		break;
	default:
		return 0;
	}

	switch (ev_type) {
	case EV_BUTTON:
		/* Swap editing modes on encoder button up */
		if (ev_v1 == 0 && ev_v2 == 0)
			editing = !editing;
		/* Record state of 2nd button for fast editing */
		if (ev_v1 == 1)
			button_down = ev_v2;
		break;
	}
	return 1;
}


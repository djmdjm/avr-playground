#ifndef EVENT_TYPES_H
#define EVENT_TYPES_H

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

/* Registry of event types */

/* NULL event; e.g. for UI refresh */
#define EV_NULL			0x00

/* Rotary encoder */
#define EV_ENCODER		0x01 /* v=1 clockwise, v=0 anti-clockwise */
/* XXX support numbered encoders */

/* Pushbuttons */
#define EV_BUTTON		0x02

/* MIDI events */
#define EV_MIDI_NOTE_ON		0x10 /* chan, note, velocity */
#define EV_MIDI_NOTE_OFF	0x11 /* chan, note, velocity */
#define EV_MIDI_CLOCK		0x12 /* 0=start, 1=stop, 2=cont, 3=tick */
#define EV_MIDI_RESET		0x13 /* empty */
#define EV_MIDI_PITCH_BEND	0x14 /* chan, MSB, LSB */
#define EV_MIDI_ATOUCH_CHAN	0x15 /* chan, value */
#define EV_MIDI_ALL_OFF		0x16 /* chan */
#define EV_MIDI_CONTROL_RESET	0x17 /* chan */
#define EV_MIDI_CONTROLLER	0x18 /* chan, controller number, MSB */

#endif /* EVENT_TYPES_H */


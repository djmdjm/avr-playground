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


static const struct config default_config = {
};

struct config cfg;


struct selection {
	size_t n;
	const char *labels[];
};

static const struct selection poly_mode = {
	POLY_MODE_MAX, { "last", "first" "lowest", "highest", "random" }
};

static const struct selection midi_ev = {
	MIDI_EV_MAX, { "note", "bend" "modwheel", "velocity", "ch.touch" }
};

static const struct selection dac_range = {
	DAC_RANGE_MAX, { "full", "half", "+full", "+half", "-full", "-half" }
};

struct config cfg;

/* Per output-config */
struct dac_config {
	int midi_ev;			/* MIDI event type assignment */
	int midi_ch;			/* MIDI channel assignment */
	int range;			/* Range: full / half / +/- */
	int offset;			/* Calibration: tuning offset adjust */
	int scale;			/* Calibration: tuning scale adjust */
};

/* Main configuration */
struct config {
	struct dac_config dac[8];	/* Per-DAC channel configuration */
	int poly_mode;			/* Polyphony mode */
};


/* Identifiers for UI inputs */
enum control_id {
	C_MODE,
	C_READY,
	C_TRIG_IN1, C_TRIG_COMBINE, C_TRIG_IN2,
	C_OUTPUT,
	C_WAIT, C_WAIT_U,
	C_WAIT2, C_WAIT2_U,
	C_ON, C_ON_U,
	C_FREQ, C_FREQ_U,
	C_DURATION, C_DURATION_U,
	C_HOLDOFF, C_HOLDOFF_U,
};

/* Identifiers for different types of input */
enum control_type {
	I_LAB,	/* Uneditable label */
	I_SEL,	/* Selection list */
	I_INT,	/* Integer */
	I_OTH,	/* Other: special case */
};

/*
 * UI element and layout/editing information
 * NB. must be increasing X, Y order.
 */
struct control {
	int x, y;
	int id;
	int type;
	size_t int_width;
	char *label;
	int *value;
	const struct selection *selection;
};

/* XXX make width mandatory for selectors? everything? */

/*
 * UI for oneshot mode:
 *
 * +--------------------+
 * |Mode:oneshot  ready |
 * |Trig:~1&~2  Out:both|
 * |Wait:XXXus CH2:XXXus|
 * |Dur:XXXus Hold:XXXus|
 * +--------------------+
 */
#define NUM_CONTROLS_ONESHOT		21
#define CONTROL_ONESHOT_STARTPOS	2 /* ready */
static const struct control oneshot_controls[NUM_CONTROLS_ONESHOT] = {
	{ 0,  0, -1,		I_LAB, 0, "Mode:", NULL, NULL },
	{ 5,  0, C_MODE,	I_SEL, 0, NULL, &cfg.mode, &modes },
	{ 13, 0, C_READY,	I_SEL, 0, NULL, &cfg.ready, &ready },

	{ 0,  1, -1,		I_LAB, 0, "TRIG:", NULL, NULL },
	{ 5,  1, C_TRIG_IN1,	I_SEL, 0, NULL, &cfg.trigger[0], &triggers },
	{ 7,  1, C_TRIG_COMBINE,I_SEL, 0, NULL, &cfg.combine, &combines },
	{ 8,  1, C_TRIG_IN2,	I_SEL, 0, NULL, &cfg.trigger[1], &triggers },
	{ 12, 1, -1,		I_LAB, 0, "Out:", NULL, NULL },
	{ 16, 1, C_OUTPUT,	I_SEL, 0, NULL, &cfg.output, &outputs },

	{ 0,  2, -1,		I_LAB, 0, "Wait:", NULL, NULL },
	{ 5,  2, C_WAIT,	I_INT, 3, NULL, &cfg.wait, NULL },
	{ 8,  2, C_WAIT_U,	I_SEL, 0, NULL, &cfg.wait_unit, &durations },
	{ 11, 2, -1,		I_LAB, 0, "CH2:", NULL, NULL },
	{ 15, 2, C_WAIT2,	I_INT, 3, NULL, &cfg.wait2, NULL },
	{ 18, 2, C_WAIT2_U,	I_SEL, 0, NULL, &cfg.wait2_unit, &durations },

	{ 0,  3, -1,		I_LAB, 0, "Dur:", NULL, NULL },
	{ 4,  3, C_ON,		I_INT, 3, NULL, &cfg.on, NULL },
	{ 7,  3, C_ON_U,	I_SEL, 0, NULL, &cfg.on_unit, &durations },
	{ 10, 3, -1,		I_LAB, 0, "Hold:", NULL, NULL },
	{ 15, 3, C_HOLDOFF,	I_OTH, 3, NULL, &cfg.holdoff, NULL },
	{ 18, 3, C_HOLDOFF_U,	I_SEL, 0, NULL, &cfg.holdoff_unit, &durations },
};

/*
 * UI for strobe mode:
 *
 * +--------------------+
 * |Mode:strobe   READY |
 * |TRIG:~1&~2  OUT:dual|
 * |WAIT:XXXus DUR:XXXus|
 * |FREQ:XXXMHz ON:XXXus|
 * +--------------------+
 */
#define NUM_CONTROLS_STROBE		21
#define CONTROL_STROBE_STARTPOS		2 /* ready */
static const struct control strobe_controls[NUM_CONTROLS_STROBE] = {
	{ 0,  0, -1,		I_LAB, 0, "Mode:", NULL, NULL },
	{ 5,  0, C_MODE,	I_SEL, 0, NULL, &cfg.mode, &modes },
	{ 13, 0, C_READY,	I_SEL, 0, NULL, &cfg.ready, &ready },

	{ 0,  1, -1,		I_LAB, 0, "TRIG:", NULL, NULL },
	{ 5,  1, C_TRIG_IN1,	I_SEL, 0, NULL, &cfg.trigger[0], &triggers },
	{ 7,  1, C_TRIG_COMBINE,I_SEL, 0, NULL, &cfg.combine, &combines },
	{ 8,  1, C_TRIG_IN2,	I_SEL, 0, NULL, &cfg.trigger[1], &triggers },
	{ 12, 1, -1,		I_LAB, 0, "Out:", NULL, NULL },
	{ 16, 1, C_OUTPUT,	I_SEL, 0, NULL, &cfg.output, &outputs },

	{ 0,  2, -1,		I_LAB, 0, "Wait:", NULL, NULL },
	{ 5,  2, C_WAIT,	I_INT, 3, NULL, &cfg.wait, NULL },
	{ 8,  2, C_WAIT_U,	I_SEL, 0, NULL, &cfg.wait_unit, &durations },
	{ 11, 2, -1,		I_LAB, 0, "Dur:", NULL, NULL },
	{ 15, 2, C_DURATION,	I_INT, 3, NULL, &cfg.len, NULL },
	{ 18, 2, C_DURATION_U,	I_SEL, 0, NULL, &cfg.len_unit, &durations },

	{ 0,  3, -1,		I_LAB, 0, "Freq:", NULL, NULL },
	{ 5,  3, C_FREQ,	I_INT, 3, NULL, &cfg.freq, NULL },
	{ 8,  3, C_FREQ_U,	I_SEL, 0, NULL, &cfg.freq_unit, &rates },
	{ 12, 3, -1,		I_LAB, 0, "On:", NULL, NULL },
	{ 15, 3, C_ON,		I_INT, 3, NULL, &cfg.on, NULL },
	{ 18, 3, C_ON_U,	I_SEL, 0, NULL, &cfg.on_unit, &durations },
};

/*
 * Returns true if the current control should be skipped due to the
 * config making it irrelevant.
 */
static int
control_skipped(int id)
{
	switch (id) {
	case -1:
		return 1;
	case C_WAIT2:
	case C_WAIT2_U:
		return cfg.output != OUT_BOTH;
	case C_TRIG_IN2:
		return cfg.combine == COMBINE_NONE;
	case C_HOLDOFF_U:
		return cfg.holdoff == -1;
	default:
		return 0;
	}
}

/*
 * Look up the index of the next control. 'current' is the index of the
 * current control. This handles controls that are disabled by the current
 * configuration (e.g. units for holdoff when manual holdoff selected).
 * If 'dec' is set then decrement rather than increment the control.
 */
static uint8_t
incdec_control(int current, int dec)
{
	int v;
	const struct control *controls = cfg.mode == MODE_ONESHOT ?
	    oneshot_controls : strobe_controls;
	size_t control_max = cfg.mode == MODE_ONESHOT ?
	    NUM_CONTROLS_ONESHOT : NUM_CONTROLS_STROBE;

	do {
		if (current == 0 && dec)
			current = control_max - 1;
		else {
			current = dec ? (current - 1) : (current + 1);
			current %= control_max;
		}
	} while (control_skipped(controls[current].id));

	return current;
}

static void
draw(uint8_t active, int *active_x, int *active_y)
{
	size_t i, l, w;
	int x, y, cursor_x, cursor_y;
	char nbuf[16];
	const char *s;
	const struct control *controls = cfg.mode == MODE_ONESHOT ?
	    oneshot_controls : strobe_controls;
	size_t control_max = cfg.mode == MODE_ONESHOT ?
	    NUM_CONTROLS_ONESHOT : NUM_CONTROLS_STROBE;

	cursor_x = cursor_y = -1;
	for (i = 0; i < control_max; i++) {
		const struct control *ctrl = &controls[i];
		const struct control *next_ctrl = (i + 1 > control_max) ?
		    &controls[i + 1] : NULL;

		/* Don't draw skipped controls */
		if (ctrl->id != -1 && control_skipped(ctrl->id)) {
			/* Blank characters to the next UI element */
			lcd_getpos(&x, NULL);
			if (next_ctrl == NULL || next_ctrl->y != ctrl->y) {
				lcd_getpos(&x, &y);
				if (y == ctrl->y && x < LCD_COLS)
					lcd_clear_eol();
			} else if (x < next_ctrl->x)
				lcd_fill(' ', next_ctrl->x - x);
			continue;
		}

		/* Record cursor position for editing */
		if (active == i) {
			cursor_x = ctrl->x;
			cursor_y = ctrl->y;
		}
		/* Draw control */
		lcd_moveto(ctrl->x, ctrl->y);
		switch (ctrl->type) {
		case I_LAB:
			lcd_string(ctrl->label);
			break;
		case I_INT:
			s = ntod(*ctrl->value);
			w = ctrl->int_width;
 draw_string:
			l = strlen(s);
			if (w >= sizeof(nbuf) || l > w) {
				lcd_string("BAD WIDTH");
				return;
			}
			lcd_string(rjustify(s, nbuf, w + 1));
			/* Position cursor on first char of active input */
			if (l > 0 && active == i)
				cursor_x += w - l;
			break;
		case I_SEL:
			s = ctrl->selection->labels[*ctrl->value];
			w = ctrl->selection->width;
			if (*ctrl->value < 0 || ctrl->selection == NULL ||
			    (size_t)*ctrl->value > ctrl->selection->n) {
				lcd_string("BAD SELECTION");
				return;
			}
			goto draw_string;
		case I_OTH:
			/* XXX Abstract special cases into a struct? */
			switch (ctrl->id) {
			case C_HOLDOFF:
				if (*ctrl->value == -1)
					s = "MAN";
				else
					s = ntod(*ctrl->value);
				w = ctrl->int_width;
				goto draw_string;
			}
			break;
		}
	}
	if (active_x != NULL)
		*active_x = cursor_x;
	if (active_y != NULL)
		*active_y = cursor_y;
}

void
edit(int active, int decrement, int fast)
{
	const int incr = fast ? 50 : 1;
	const struct control *ctrl = &(cfg.mode == MODE_ONESHOT ?
	    oneshot_controls : strobe_controls)[active];

	switch (ctrl->type) {
	case I_LAB:
		/* Shouldn't happen */
		return;
	case I_INT:
		switch (ctrl->id) {
		case C_WAIT:
		case C_ON:
		case C_FREQ:
		case C_DURATION:
			/* XXX: All values are [0:1000) for the moment */
			if (decrement)
				*ctrl->value -= incr;
			else
				*ctrl->value += incr;
			if (*ctrl->value < 0)
				*ctrl->value += 1000;
			else
				*ctrl->value %= 1000;
			break;
		}
		break;
	case I_SEL:
		if (decrement)
			*ctrl->value -= 1;
		else
			*ctrl->value += 1;
		if (*ctrl->value < 0)
			*ctrl->value += ctrl->selection->n;
		else
			*ctrl->value %= ctrl->selection->n;
		break;
	case I_OTH:
		switch (ctrl->id) {
		case C_HOLDOFF:
			if (decrement) {
				if (*ctrl->value == 0)
					*ctrl->value = -1; /* "manual" */
				else
					*ctrl->value -= incr;
			} else {
				if (*ctrl->value == 999)
					*ctrl->value = -1;
				else
					*ctrl->value += incr;
			}
			if (*ctrl->value < -1)
				*ctrl->value += 1001;
			else if (*ctrl->value > 0)
				*ctrl->value %= 1000;
			break;
		}
		break;
	}
}

void
config_edit(void)
{
	uint8_t active = cfg.mode == MODE_ONESHOT ?
	    CONTROL_ONESHOT_STARTPOS : CONTROL_STROBE_STARTPOS;
	uint8_t editing = 0, button_down = 0;
	uint8_t ev_type, ev_v1, ev_v2;
	int omode, i, active_x, active_y;

	lcd_moveto(0, 0);
	lcd_clear();

	while (1) {
		active_x = active_y = -1;
		draw(active, &active_x, &active_y);
		lcd_display(1, 1, editing ? 0 : 1);
		if (active_x != -1 && active_y != -1)
			lcd_moveto(active_x, active_y);
		else
			lcd_moveto(LCD_COLS - 1, LCD_ROWS - 1); /* visible */
		if (cfg.ready & !editing)
			break;
		event_sleep(SLEEP_MODE_IDLE, &ev_type, &ev_v1, &ev_v2, NULL);
		omode = cfg.mode;
		switch (ev_type) {
		case EV_ENCODER:
			if (editing)
				edit(active, ev_v1 ? 0 : 1, button_down);
			else
				active = incdec_control(active, ev_v1 ? 0 : 1);
			break;
		case EV_BUTTON:
			/* Swap editing modes on encoder button up */
			if (ev_v1 == 0 && ev_v2 == 0)
				editing = !editing;
			/* Record state of 2nd button for fast editing */
			if (ev_v1 == 1)
				button_down = ev_v2;
			break;
		}
		if (omode != cfg.mode)
			lcd_clear();
	}
}

void
reset_config(void)
{
	cfg = default_config;
}

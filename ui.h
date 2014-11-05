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

/* UI for lighting controller */

#ifndef _UI_H
#define _UI_H

/* Per output-config */
struct dac_config {
	int midi_ch;			/* MIDI channel assignment */
	int midi_ev;			/* MIDI event type assignment */
	int range;			/* Range: full / half / +/- */
	int offset;			/* Calibration: tuning offset adjust */
	int scale;			/* Calibration: tuning scale adjust */
};

/* Main configuration */
struct config {
	struct dac_config dac[8];	/* Per-DAC channel configuration */
	int poly_mode[8];		/* Per-input channel polyphony mode */
};

/*
 * Pipe a UI event to the configuration editor. Returns true if the config
 * was changed .
 */
int config_edit(uint8_t type, uint8_t v1, uint8_t v2, uint8_t v3);

/* Reset configuration to default. */
void reset_config(void);

/* The actual configuration. NB. must be initialised using reset_config() */
extern struct config cfg;

#endif /* _UI_H */


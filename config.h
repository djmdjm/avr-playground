/*
 * Copyright (c) 2016 Damien Miller <djm@mindrot.org>
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

#ifndef _CONFIG_H
#define _CONFIG_H

/* Polyphony modes */
#define POLY_LAST 1
#define POLY_FIRST 2
#define POLY_LOWEST 3
#define POLY_HIGHEST 4
#define POLY_RANDOM 5

/* Channel event modes */
#define CH_EVENT_NOTE 1
#define CH_EVENT_BEND 2
#define CH_EVENT_MODWHEEL 3
#define CH_EVENT_VELOCITY 4
#define CH_EVENT_CHANNEL_TOUCH 5

/* MIDI channel special modes */
#define MIDI_OMNI 0

/* Channel output range modes */
#define CH_RANGE_FULL 1
#define CH_RANGE_HALF 2
#define CH_RANGE_FULL_POS 3
#define CH_RANGE_HALF_POS 4
#define CH_RANGE_FULL_NEG 5
#define CH_RANGE_HALF_NEG 6

/* Callbacks from UI code to update configuration/state on edit events */
int get_evtype(void);
void set_evtype(int t);
int get_poly(void);
void set_poly(int p);
int get_midich(void);
void set_midich(int o);
int get_output_range(void);
void set_output_range(int p);
int get_scale(void);
void set_scale(int s);
int get_offset(void);
void set_offset(int o);
void set_channel(int c);
void reset_cfg(void);
void save_cfg(void);
void update_firmware(void);

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

/* The actual configuration. NB. must be initialised using reset_config() */
extern struct config cfg;

#endif /* _CONFIG_H */


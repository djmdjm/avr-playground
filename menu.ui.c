

/* This file is generated by uigen; DO NOT EDIT */


/* Generated from menu.def -- do not edit */

/*
 * Copyright (c) 2013-2015 Damien Miller <djm@mindrot.org>
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


#include "config.h"


#include <stddef.h>
#include <stdint.h>

struct menu;

struct editable_item {
	const char *label;	/* NULL if integer range */
	int value;
};

struct editable {
	const char *display;
	int (*get)(void);
	void (*set)(int);
	int set_every;
	int range_lo, range_hi;	/* If no integer range, then hi<lo */
	size_t n;
	const struct editable_item *items;
};

struct ask_item {
	const char *label;
	void (*action)(void);	/* NULL to return to previous menu */
};

struct ask {
	const char *display;
	size_t n;
	const struct ask_item *items;
};

struct submenu_range {
	const char *label;
	void (*set)(int);
	int range_lo, range_hi;	/* If no integer range, then hi<lo */
	struct menu *definition;
};

struct menu_item {
	const char *label;
	enum { M_EDITABLE, M_ASK, M_SUBMENU, M_SUBMENU_RANGE } mi_type;
	union {
		struct editable *editable;
		struct ask *ask;
		struct menu *submenu;
		struct submenu_range *submenu_range;
	} item;
};

struct menu {
	size_t n;
	struct menu_item *items;
	struct menu *parent;
};

/* Forward declaration of menus */
struct menu root;
struct menu channel_list;
struct menu channel;
struct menu tuning;

/* Root menu */



struct editable_item root_1_editable_poly_mode_items[] = {
	{ "last", POLY_LAST },
	{ "first", POLY_FIRST },
	{ "lowest", POLY_LOWEST },
	{ "highest", POLY_HIGHEST },
	{ "random", POLY_RANDOM },
};

struct editable root_1_editable_poly_mode = {
	"POLY MODE",
	get_poly,
	set_poly,
	0,
	0, -1, /* no integer range */
	5,
	root_1_editable_poly_mode_items,
};





void reset_cfg(void);

struct ask_item root_2_ask_reset_config_items[] = {
	{ "Y", reset_cfg },
	{ "N", NULL },
};

struct ask root_2_ask_reset_config = {
	"RESET CONFIG",
	2,
	root_2_ask_reset_config_items,
};



void save_cfg(void);

struct ask_item root_3_ask_save_config_items[] = {
	{ "Y", save_cfg },
	{ "N", NULL },
};

struct ask root_3_ask_save_config = {
	"SAVE CONFIG",
	2,
	root_3_ask_save_config_items,
};



void update_firmware(void);

struct ask_item root_4_ask_update_firmware_items[] = {
	{ "Y", update_firmware },
	{ "N", NULL },
};

struct ask root_4_ask_update_firmware = {
	"UPDATE FIRMWARE",
	2,
	root_4_ask_update_firmware_items,
};


struct menu_item root_items[] = {
	{
		.label = "EDIT CHANNEL",
		.mi_type = M_SUBMENU,
		.item = { .submenu = &channel_list }

	},
	{
		.label = "POLY MODE",
		.mi_type = M_EDITABLE,
		.item = { .editable = &root_1_editable_poly_mode },

	},
	{
		.label = "RESET CONFIG",
		.mi_type = M_ASK,
		.item = { .ask = &root_2_ask_reset_config }

	},
	{
		.label = "SAVE CONFIG",
		.mi_type = M_ASK,
		.item = { .ask = &root_3_ask_save_config }

	},
	{
		.label = "UPDATE FIRMWARE",
		.mi_type = M_ASK,
		.item = { .ask = &root_4_ask_update_firmware }

	},

};

struct menu root = {
	5, root_items,
	NULL
};

/* Menu "channel_list" */



struct submenu_range channel_list_0_submenu_range_channel = {
	"CHANNEL",
	set_channel,
	1, 8,
	&channel,
};



struct menu_item channel_list_items[] = {
	{
		.label = "CHANNEL",
		.mi_type = M_SUBMENU_RANGE,
		.item = { .submenu_range = &channel_list_0_submenu_range_channel }

	},

};

struct menu channel_list = {
	1, channel_list_items,
	&root
};

/* Menu "channel" */



struct editable_item channel_0_editable_midi_event_items[] = {
	{ "note", CH_EVENT_NOTE },
	{ "bender", CH_EVENT_BEND },
	{ "modwheel", CH_EVENT_MODWHEEL },
	{ "velocity", CH_EVENT_VELOCITY },
	{ "ch.touch", CH_EVENT_CHANNEL_TOUCH },
};

struct editable channel_0_editable_midi_event = {
	"MIDI EVENT",
	get_evtype,
	set_evtype,
	0,
	0, -1, /* no integer range */
	5,
	channel_0_editable_midi_event_items,
};


struct editable_item channel_1_editable_midi_channel_items[] = {
	{ "omni", MIDI_OMNI },
};

struct editable channel_1_editable_midi_channel = {
	"MIDI CHANNEL",
	get_midich,
	set_midich,
	1,
	1, 16,
	1,
	channel_1_editable_midi_channel_items,
};


struct editable_item channel_2_editable_output_range_items[] = {
	{ "-10>+10", CH_RANGE_FULL },
	{ "-5>+5", CH_RANGE_HALF },
	{ "0>+10", CH_RANGE_FULL_POS },
	{ "0>+5", CH_RANGE_HALF_POS },
	{ "-10>0", CH_RANGE_FULL_NEG },
	{ "-5>0", CH_RANGE_HALF_NEG },
};

struct editable channel_2_editable_output_range = {
	"OUTPUT RANGE",
	get_output_range,
	set_output_range,
	0,
	0, -1, /* no integer range */
	6,
	channel_2_editable_output_range_items,
};




struct menu_item channel_items[] = {
	{
		.label = "MIDI EVENT",
		.mi_type = M_EDITABLE,
		.item = { .editable = &channel_0_editable_midi_event },

	},
	{
		.label = "MIDI CHANNEL",
		.mi_type = M_EDITABLE,
		.item = { .editable = &channel_1_editable_midi_channel },

	},
	{
		.label = "OUTPUT RANGE",
		.mi_type = M_EDITABLE,
		.item = { .editable = &channel_2_editable_output_range },

	},
	{
		.label = "TUNING",
		.mi_type = M_SUBMENU,
		.item = { .submenu = &tuning }

	},

};

struct menu channel = {
	4, channel_items,
	&channel_list
};

/* Menu "tuning" */




struct editable tuning_1_editable_offset = {
	"OFFSET",
	get_offset,
	set_offset,
	1,
	-32768, 32767,
	0,
	NULL,
};



struct editable tuning_2_editable_scale = {
	"SCALE",
	get_scale,
	set_scale,
	1,
	-32768, 32767,
	0,
	NULL,
};





void reset_tune(void);

struct ask_item tuning_0_ask_reset_tuning_items[] = {
	{ "Y", reset_tune },
	{ "N", NULL },
};

struct ask tuning_0_ask_reset_tuning = {
	"RESET TUNING",
	2,
	tuning_0_ask_reset_tuning_items,
};


struct menu_item tuning_items[] = {
	{
		.label = "RESET TUNING",
		.mi_type = M_ASK,
		.item = { .ask = &tuning_0_ask_reset_tuning }

	},
	{
		.label = "OFFSET",
		.mi_type = M_EDITABLE,
		.item = { .editable = &tuning_1_editable_offset },

	},
	{
		.label = "SCALE",
		.mi_type = M_EDITABLE,
		.item = { .editable = &tuning_2_editable_scale },

	},

};

struct menu tuning = {
	3, tuning_items,
	&channel
};




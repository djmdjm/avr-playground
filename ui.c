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

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "lcd.h"
#include "num_format.h"
#include "encoder.h"
#include "event.h"
#include "event-types.h"
#include "ui.h"
#include "menu.ui.h"

static bool button_down = 0;
static bool editing = 0;
static int editval;
static size_t menu_pos[MENU_MAX_DEPTH]; /* menu positions in hierarchy */
static size_t menu_depth = 0;
static const struct menu *menu = NULL;

static const struct menu_item *
clean_state(void)
{
	const struct menu_item *menu_item = NULL;

	if (menu == NULL) {
		menu = menu_root;
		menu_depth = 0;
		return &menu->items[0];
	}

	/* Refresh display with current menu location */
	if (menu_depth >= MENU_MAX_DEPTH) {
		/* Shouldn't happen */
		menu_depth = MENU_MAX_DEPTH - 1;
		/* XXX report error */
	}
	/* Determine active menu item */
	if (menu_pos[menu_depth] < menu->n) {
		menu_item = &menu->items[menu_pos[menu_depth]];
	} else {
		if (menu == menu_root) {
			/* Shouldn't happen */
			menu_pos[menu_depth] = menu->n - 1; /* clamp */
			menu_item = &menu->items[menu_pos[menu_depth]];
			/* XXX report error */
		} else {
			menu_pos[menu_depth] = menu->n; /* clamp to BACK */
			menu_item = NULL;
		}
	}
	return menu_item;
}

/* Exits editing/ask mode */
static bool
finish_edit(const struct menu_item *menu_item)
{
	const struct editable *editable;

	if (!editing || menu_item == NULL)
		goto badedit;

	switch (menu_item->mi_type) {
	case M_EDITABLE:
		editable = menu_item->item.editable;
		if (editable->range_hi < editable->range_lo) {
			/* Editable items list */
			if (editval < 0 || (size_t)editval >= editable->n)
				goto badedit;
			editable->set(editable->items[editval].value);
			editing = false;
			return true;
		}
		/* Editable value */
		if (editval < editable->range_lo ||
		    editval > editable->range_hi)
			goto badedit;
		editable->set(editval);
		editing = false;
		return true;
	case M_ASK:
		if (editval < 0 || (size_t)editval > menu_item->item.ask->n)
			goto badedit;
		if (menu_item->item.ask->items[editval].action != NULL)
			menu_item->item.ask->items[editval].action();
		editing = false;
		return true;
	default:
		/* shouldn't happen */
		break;
	}
 badedit:
	lcd_moveto(0, 0);
	lcd_string("!BADEDIT");
	return false;
}

static bool
refresh_display(void)
{
	const struct menu_item *menu_item;
	size_t i;
	const struct editable *editable;
	bool found = false;

	menu_item = clean_state();

	lcd_clear();
	if (!editing) {
		if (menu_item == NULL) {
			/* XXX display context? */
			lcd_centered_string("BACK");
		} else {
			lcd_centered_string(menu_item->label);
			if (menu_item->mi_type == M_SUBMENU_RANGE)
				lcd_centred_string_row(1, ntod(editval));
		}
		return true;
	}

	switch (menu_item->mi_type) {
	case M_EDITABLE:
		editable = menu_item->item.editable;
		lcd_centred_string_row(0, editable->display);
		if (editable->range_hi >= editable->range_lo) {
			/* Editable value */
			lcd_centred_string_row(1, ntod(editval));
			return true;
		}
		/* Editable item: find it and display it */
		for (i = 0; i < editable->n; i++) {
			if (editval == editable->items[i].value) {
				found = true;
				break;
			}
		}
		if (!found)
			i = 0;
		lcd_centred_string_row(1, editable->items[i].label);
		return true;
	case M_ASK:
		lcd_centred_string_row(0, menu_item->item.ask->display);
		if (editval < 0 || (size_t)editval >= menu_item->item.ask->n)
			editval = 0;
		lcd_centred_string_row(1,
		    menu_item->item.ask->items[editval].label);
		return true;
	default:
		/* shouldn't happen */
		break;
	}
	lcd_moveto(0, 0);
	lcd_string("!BADDISP");
	return false;
}

bool
config_edit(uint8_t ev_type, uint8_t ev_v1, uint8_t ev_v2, uint8_t ev_v3)
{
	const struct menu_item *menu_item;
	bool found, enc_up = false, enc_down = false, click = false;
	size_t i;

	clean_state();

	switch (ev_type) {
	case EV_NULL:
		break;
	case EV_ENCODER:
		/* All encoder events are consumed below */
		if (ev_v1 == 0)
			enc_down = true;
		else if (ev_v1 == 1)
			enc_up = true;
		break;
	case EV_BUTTON:
		/* Only interested in first button */
		if (ev_v1 != 0)
			return false;
		if (ev_v2 == 0) {
			/* button up; record a click if the button was down */
			if (button_down)
				click = true;
			break;
		} else if (ev_v2 == 1) {
			button_down = true;
			break;
		}
		break;
	default:
		return false;
	}
 out:
	menu_item = clean_state();

	if (click) {
		if (editing) {
			/* Encoder press terminates editing */
			if (!finish_edit(menu_item))
				return false;
		} else if (menu_item == NULL) {
			/* Go back to previous menu */
			if (menu_depth > 0)
				menu_depth--;
		} else {
			switch (menu_item->mi_type) {
			case M_EDITABLE:
				/* start editing */
				editval = menu_item->item.editable->get();
				editing = true;
				break;
			case M_ASK:
				/* start ask menu */
				editing = true;
				editval = 0;
				break;
			case M_SUBMENU:
				/* XXX enter submenu, update pos */
				break;
			case M_SUBMENU_RANGE:
				/* XXX Call set func, update pos, enter submenu */
				break;
			default:
				goto badmenu;
			}
		}
	} else if (enc_up || enc_down) {
		switch (menu_item->mi_type) {
		case M_EDITABLE:
			/* XXX alter editval, maybe call setter */
			break;
		case M_ASK:
			/* XXX alter editval */
			break;
		case M_SUBMENU:
			/* XXX alter pos */
			break;
		case M_SUBMENU_RANGE:
			/* XXX Alter editval */
			break;
		default:
 badmenu:
			lcd_clear();
			lcd_moveto(0, 0);
			lcd_string("!BADMENU");
		}
	}
	return refresh_display();
}


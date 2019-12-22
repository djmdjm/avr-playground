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

/* Driver for event and fakelcd code for testing UI on development host */

#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <signal.h>
#include <curses.h>

#include "event.h"
#include "event-types.h"
#include "lcd.h"
#include "num_format.h"
#include "ui.h"

/* We need this for scroll wheel */
#if !defined(NCURSES_MOUSE_VERSION) || NCURSES_MOUSE_VERSION < 2
#error ncurses.h does not support mouse protocol 2
#endif

static void
sighand(int unused)
{
	endwin();
	_exit(1);
}

static void
exithand(void)
{
	endwin();
}

static void debug(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

static void
debug(const char *fmt, ...)
{
	int x, y;
	va_list args;

	getyx(stdscr, y, x);
	move(8, 0);
	va_start(args, fmt);
	vwprintw(stdscr, fmt, args);
	va_end(args);
	move(y, x);
}

int
handle_input(void)
{
	int ch;
	MEVENT mevent;

	ch = getch();
	switch (ch) {
	case 0x03: /* ^C */
	case 0x1b: /* ESC */
	case 'q':
	case 'Q':
		return 1;
	case KEY_UP:
	case KEY_LEFT:
		event_enqueue(EV_ENCODER, 1, 0, 0, 0);
		return 0;
	case KEY_DOWN:
	case KEY_RIGHT:
		event_enqueue(EV_ENCODER, 0, 0, 0, 0);
		return 0;
	case KEY_MOUSE:
		/* see below */
		break;
	default:
		return 0;
	}
	if (getmouse(&mevent) != OK)
		return 0;

	/* Mouse wheel click */
	if (mevent.bstate & BUTTON1_PRESSED)
		event_enqueue(EV_BUTTON, 0, 1, 0, 0);
	if (mevent.bstate & BUTTON1_RELEASED)
		event_enqueue(EV_BUTTON, 0, 0, 0, 0);

	/* Mouse wheel */
	if (mevent.bstate & BUTTON4_PRESSED)
		event_enqueue(EV_ENCODER, 1, 0, 0, 0);
	if (mevent.bstate & BUTTON5_PRESSED)
		event_enqueue(EV_ENCODER, 0, 0, 0, 0);
	return 0;
}

int
main(int argc, char **argv)
{
	uint8_t i, j, ev_type, ev_v1, ev_v2, ev_v3;

	initscr();
	nocbreak();
	raw();
	noecho();
	keypad(stdscr, TRUE);
	mouseinterval(0);
	mousemask(ALL_MOUSE_EVENTS, NULL);

	atexit(exithand);
	signal(SIGINT, sighand);
	signal(SIGTERM, sighand);

	move(6, 0);
	printw("Mouse wheel simulates encoder. Press 'q' to exit");

	lcd_setup();
	lcd_centered_string("hello\nthere!");
	sleep(5);

	/* init editor display */
	config_edit(EV_NULL, 0, 0, 0);

	for (;;) {
		if (handle_input())
			break;
		if (!event_dequeue(&ev_type, &ev_v1, &ev_v2, &ev_v3))
			continue;
		if (config_edit(ev_type, ev_v1, ev_v2, ev_v3))
			continue;
	}

	return 0;
}


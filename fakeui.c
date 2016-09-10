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
#include <ncurses.h>

#include "lcd.h"

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
main(int argc, char **argv)
{
	int ch, done;
	MEVENT mevent;

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
	lcd_string("hello!");

	done = 0;
	while (!done) {
		ch = getch();
		switch (ch) {
		case 0x03: /* ^C */
		case 0x1b: /* ESC */
		case 'q':
		case 'Q':
			done = 1;
			continue;
		case KEY_MOUSE:
			/* see below */
			break;
		default:
			debug("Key 0x%02x %c", ch, ch);
			continue;
		}
		if (getmouse(&mevent) != OK)
			continue;

		/* Mouse wheel click */
		char buf[20];
		size_t o;

		o = 0;
		if (mevent.bstate & BUTTON1_PRESSED) {
			buf[o++] = 'a';
			mevent.bstate &= ~(unsigned long)BUTTON1_PRESSED;
		}
		if (mevent.bstate & BUTTON1_RELEASED) {
			buf[o++] = 'A';
			mevent.bstate &= ~(unsigned long)BUTTON1_RELEASED;
		}
		if (mevent.bstate & BUTTON2_PRESSED) {
			buf[o++] = 'b';
			mevent.bstate &= ~(unsigned long)BUTTON2_PRESSED;
		}
		if (mevent.bstate & BUTTON2_RELEASED) {
			buf[o++] = 'B';
			mevent.bstate &= ~(unsigned long)BUTTON2_RELEASED;
		}
		if (mevent.bstate & BUTTON3_PRESSED) {
			buf[o++] = 'c';
			mevent.bstate &= ~(unsigned long)BUTTON3_PRESSED;
		}
		if (mevent.bstate & BUTTON3_RELEASED) {
			buf[o++] = 'C';
			mevent.bstate &= ~(unsigned long)BUTTON3_RELEASED;
		}
		if (mevent.bstate & BUTTON4_PRESSED) {
			buf[o++] = 'd';
			mevent.bstate &= ~(unsigned long)BUTTON4_PRESSED;
		}
		if (mevent.bstate & BUTTON4_RELEASED) {
			buf[o++] = 'D';
			mevent.bstate &= ~(unsigned long)BUTTON4_RELEASED;
		}
		if (mevent.bstate & BUTTON5_PRESSED) {
			buf[o++] = 'e';
			mevent.bstate &= ~(unsigned long)BUTTON5_PRESSED;
		}
		if (mevent.bstate & BUTTON5_RELEASED) {
			buf[o++] = 'E';
			mevent.bstate &= ~(unsigned long)BUTTON5_RELEASED;
		}
		while (o < sizeof(buf) - 2)
			buf[o++] = ' ';
		buf[o] = '\0';

		debug("%s 0x%04lx", buf, mevent.bstate);
#if 0
		debug("id: %d xyz: %d %d %d state: 0x%08lx",
		    mevent.id, mevent.x, mevent.y, mevent.z,
		    mevent.bstate);
#endif
	}

	return 0;
}


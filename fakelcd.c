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

/* Fake LCD library for testing UI code on a development host */

#include <sys/types.h>
#include <ncurses.h>
#include <signal.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>

#include "lcd.h"

void
lcd_setup(void)
{
	/* Assumes ncurses already started */
	lcd_clear();
}

void
lcd_display(int display_on, int cursor_on, int blink_on)
{
}

void lcd_clear(void)
{
	move(0, 0);
	printw("+--------+");
	move(1, 0);
	printw("+        +");
	move(2, 0);
	printw("+        +");
	move(3, 0);
	printw("+--------+");
	lcd_home();
	refresh();
}

void
lcd_clear_eol(void)
{
	int x, y;

	getyx(stdscr, y, x);
	if (y < 1 || y > 2)
		return;
	if (x < 1 || x >= 8)
		return;
	x = 8 - x;
	while (--x)
		addch(' ');
	refresh();
}

void
lcd_home(void)
{
	lcd_moveto(0, 0);
}

void
lcd_moveto(int x, int y)
{
	if (x < 0 || x > 7 || y < 0 || y > 1)
		return;
	move(y + 1, x + 1);
	refresh();
}

void
lcd_getpos(int *x, int *y)
{
	int px, py;

	if (x != NULL)
		*x = 0;
	if (y != NULL)
		*y = 0;
	getyx(stdscr, py, px);
	if (py < 1 || py > 2)
		return;
	if (px < 1 || px >= 8)
		return;
	if (x != NULL)
		*x = px - 1;
	if (y != NULL)
		*y = py - 1;
}

void
lcd_string(const char *s)
{
	size_t n = strlen(s);
	int x;

	lcd_getpos(&x, NULL);
	if (x < 0 || x > 7)
		return; /* XXX error? */
	x = 8 - x;
	if ((size_t)x < n)
		n = (size_t)x;
	while (n--)
		addch(*s++);
	refresh();
}

void
lcd_char(char c)
{
	int x;

	lcd_getpos(&x, NULL);
	if (x < 0 || x > 7)
		return; /* XXX error? */
	addch(c);
	refresh();
}

void
lcd_fill(char c, size_t n)
{
	int x;

	lcd_getpos(&x, NULL);
	if (x < 0 || x > 7)
		return; /* XXX error? */
	x = 8 - x;
	if ((size_t)x < n)
		n = (size_t)x;
	while (n--)
		addch(c);
	refresh();
}

void
lcd_program_char(int c, uint8_t *data, size_t len)
{
	/* XXX */
}


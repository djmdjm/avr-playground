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

#ifndef F_CPU
#error Must define F_CPU on commandline
#endif

/* XXX maintain location pointer to avoid getpos() calls */

#include <stddef.h>
#include <avr/io.h>
#include <util/delay.h>
#include <util/delay_basic.h>

#include "lcd.h"

#if !defined(LCD_CTL_DDR) || !defined(LCD_CTL_PORT) || \
    !defined(LCD_DB_PIN) || !defined(LCD_DB_DDR) || !defined(LCD_DB_PORT) || \
    !defined(LCD_CTL_RS) || !defined(LCD_CTL_RW) || \
    !defined(LCD_EN_DDR) || !defined(LCD_EN_PORT) || !defined(LCD_EN) || \
    !defined(LCD_DB_4) || !defined(LCD_DB_5) || !defined(LCD_DB_6) || \
    !defined(LCD_DB_7)
# error Please define pin/port configuration in lcd.h
#endif

#define LCD_DB_MASK	((1 << LCD_DB_4) | (1 << LCD_DB_5) | \
			 (1 << LCD_DB_6) | (1 << LCD_DB_7))
#define LCD_CTL_MASK	((1 << LCD_CTL_RS) | (1 << LCD_CTL_RW))

/* LCD constants */
#if LCD_ROWS == 4
static const uint8_t row_addrs[LCD_ROWS] = { 0x00, 0x40, 0x14, 0x54 };
#elif LCD_ROWS == 2
static const uint8_t row_addrs[LCD_ROWS] = { 0x00, 0x40 };
#elif LCD_ROWS == 1
static const uint8_t row_addrs[LCD_ROWS] = { 0x00 };
#else
# error Unsupported LCD_ROWS
#endif

#define LCD_NUM_CG_CHARS	8
#define LCD_CG_MASK		0x1f

/* LCD commands for write */
#define LCD_C_CLEAR		0x01	/* Clear display */
#define LCD_C_RETURN		0x02	/* Set DDRAM address to 0x00 */
#define LCD_C_ENTRY_MODE	0x04	/* Set entry mode */
#define LCD_O_ENTRY_LTR		0x02	/* Left-to-right text direction */
#define LCD_O_ENTRY_SHIFT	0x01	/* "shift of entire display" */
#define LCD_C_DISPLAY_ON_OFF	0x08	/* Control off/on, cursor and blink */
#define LCD_O_DISPLAY_ON	0x04	/* Display on */
#define LCD_O_DISPLAY_CURSOR	0x02	/* Show cursor */
#define LCD_O_DISPLAY_BLINK	0x01	/* Blinking cursor */
#define LCD_C_SHIFT		0x10	/* Shift cursor or display w/o write */
#define LCD_O_SHIFT_CURSOR_L	0x00	/* Shift cursor left */
#define LCD_O_SHIFT_CURSOR_R	0x04	/* Shift cursor left */
#define LCD_O_SHIFT_DISPLAY_L	0x08	/* Shift display left */
#define LCD_O_SHIFT_DISPLAY_R	0x0c	/* Shift display right */
#define LCD_C_FUNC_SET		0x20	/* Setup interface, num lines, font */
#define LCD_O_FUNC_8BIT		0x10	/* 8-bit bus mode; NOT SUPPORTED */
#define LCD_O_FUNC_4BIT		0x00	/* 4-bit bus mode */
#define LCD_O_FUNC_2LINES	0x08	/* Two display lines */
#define LCD_O_FUNC_1LINES	0x00	/* Single display line */
#define LCD_O_FUNC_FONT_5X11	0x04	/* Use 5x11 font */
#define LCD_O_FUNC_FONT_5X8	0x00	/* Use 5x8 font */
#define LCD_C_SET_CGRAM_ADDR	0x40	/* Set address pointer to CGRAM */
#define LCD_O_CGRAM_MASK	0x3f	/* Mask for CGRAM address pointer */
#define LCD_C_SET_DDRAM_ADDR	0x80	/* Set address pointer to DDRAM */
#define LCD_O_DDRAM_MASK	0x7f	/* Mask for DDRAM address pointer */

/* LCD commands for read */
#define LCD_R_BUSY		0x80
#define LCD_R_ADDR_MASK		0x7f

/*
 * Write a nibble to the LCD card; used for early setup to put it in
 * 4-wire mode.
 */
static void
lcd_write4(int rs, uint8_t val)
{
	/* We'll be using the DB pins as outputs here */
	LCD_DB_DDR |= LCD_DB_MASK;
	/* Select register and R/W mode */
	LCD_CTL_PORT = (LCD_CTL_PORT & ~LCD_CTL_MASK) |
	    (rs ? 1 << LCD_CTL_RS : 0);
	_delay_us(0.100); /* Tsp1 = 40ns */
	/* Assert 'enable', send data and hold */
	LCD_EN_PORT |= (1 << LCD_EN);
	LCD_DB_PORT = (LCD_DB_PORT & ~LCD_DB_MASK) |
		((val & 0x1) ? 1 << LCD_DB_4 : 0) |
		((val & 0x2) ? 1 << LCD_DB_5 : 0) |
		((val & 0x4) ? 1 << LCD_DB_6 : 0) |
		((val & 0x8) ? 1 << LCD_DB_7 : 0);
	_delay_us(0.250); /* Tpw = 230ns */
	/* Drop 'enable' while holding data for a period */
	LCD_EN_PORT &= ~(1 << LCD_EN);
	_delay_us(0.050); /* Thd2 = 10ns */
	/* De-assert all signals, float DB lines */
	LCD_CTL_PORT &= ~LCD_CTL_MASK;
	LCD_DB_PORT |= LCD_DB_MASK;
	/* Put DB pins back to HiZ input mode */
	LCD_DB_DDR &= ~LCD_DB_MASK;
	/* Ensure Tc=500ns is satisfied too */
	_delay_us(0.250); /* In addition to Tpw delay above */
}

/* Write a byte to the LCD card */
static void
lcd_write8(int rs, uint8_t val)
{
	/* We'll be using the DB pins as outputs here */
	LCD_DB_DDR |= LCD_DB_MASK;
	/* Select register and R/W mode */
	LCD_CTL_PORT = (LCD_CTL_PORT & ~LCD_CTL_MASK) |
	    (rs ? 1 << LCD_CTL_RS : 0);
	_delay_us(0.100); /* Tsp1 = 40ns */
	/* Assert 'enable', send data MSB and hold */
	LCD_EN_PORT |= (1 << LCD_EN);
	LCD_DB_PORT = (LCD_DB_PORT & ~LCD_DB_MASK) |
		((val & 0x10) ? 1 << LCD_DB_4 : 0) |
		((val & 0x20) ? 1 << LCD_DB_5 : 0) |
		((val & 0x40) ? 1 << LCD_DB_6 : 0) |
		((val & 0x80) ? 1 << LCD_DB_7 : 0);
	_delay_us(0.250); /* Tpw = 230ns */
	/* Drop 'enable' while holding data for a period */
	LCD_EN_PORT &= ~(1 << LCD_EN);
	_delay_us(0.050); /* Thd2 = 10ns */
	/* Reassert 'enable' and send data LSB */
	LCD_EN_PORT |= (1 << LCD_EN);
	LCD_DB_PORT = (LCD_DB_PORT & ~LCD_DB_MASK) |
		((val & 0x01) ? 1 << LCD_DB_4 : 0) |
		((val & 0x02) ? 1 << LCD_DB_5 : 0) |
		((val & 0x04) ? 1 << LCD_DB_6 : 0) |
		((val & 0x08) ? 1 << LCD_DB_7 : 0);
	_delay_us(0.250); /* Tpw = 230ns */
	/* Drop 'enable' while holding data for a period */
	LCD_EN_PORT &= ~(1 << LCD_EN);
	_delay_us(0.050); /* Thd2 = 10ns */
	/* De-assert all signals, float DB lines */
	LCD_CTL_PORT &= ~LCD_CTL_MASK;
	LCD_DB_PORT |= LCD_DB_MASK;
	/* Put DB pins back to HiZ input mode */
	LCD_DB_DDR &= ~LCD_DB_MASK;
	/* Ensure Tc=500ns is satisfied too */
	_delay_us(0.250); /* In addition to Tpw delay above */
}

/* Read a byte from the LCD card */
static uint8_t
lcd_read8(int rs)
{
	uint8_t pin_h, pin_l;

	/* Select register and R/W mode */
	LCD_CTL_PORT = (LCD_CTL_PORT & ~LCD_CTL_MASK) |
	    (rs ? (1 << LCD_CTL_RS) : 0) | (1 << LCD_CTL_RW);
	/* We'll be using the DB pins as inputs here */
	LCD_DB_DDR &= ~LCD_DB_MASK;
	_delay_us(0.100); /* Tsp1 = 40ns */
	/* Assert 'enable' and hold for data */
	LCD_EN_PORT |= (1 << LCD_EN);
	_delay_us(0.500); /* Td = 120ns, also satisfies Tpw=230ns, Tc=500ns */
	pin_h = LCD_DB_PIN;
	/* Toggle 'enable' */
	LCD_EN_PORT &= ~(1 << LCD_EN);
	_delay_us(0.050); /* Thd1 = 10ns */
	LCD_EN_PORT |= (1 << LCD_EN);
	_delay_us(0.500); /* Td = 120ns, also satisfies Tpw=230ns, Tc=500ns */
	pin_l = LCD_DB_PIN;
	/* Drop 'enable' */
	LCD_EN_PORT &= ~(1 << LCD_EN);
	_delay_us(0.050); /* Thd1 = 10ns */
	/* De-assert all signals, float DB lines */
	LCD_CTL_PORT &= ~LCD_CTL_MASK;
	LCD_DB_PORT |= LCD_DB_MASK;
	return ((pin_l & (1 << LCD_DB_4)) ? 0x01 : 0) |
	    ((pin_l & (1 << LCD_DB_5)) ? 0x02 : 0) |
	    ((pin_l & (1 << LCD_DB_6)) ? 0x04 : 0) |
	    ((pin_l & (1 << LCD_DB_7)) ? 0x08 : 0) |
	    ((pin_h & (1 << LCD_DB_4)) ? 0x10 : 0) |
	    ((pin_h & (1 << LCD_DB_5)) ? 0x20 : 0) |
	    ((pin_h & (1 << LCD_DB_6)) ? 0x40 : 0) |
	    ((pin_h & (1 << LCD_DB_7)) ? 0x80 : 0);
}

/* Wait until the LCD card's busy flag is clear; return the current address */
static uint8_t
lcd_waitbusy(void)
{
	while ((lcd_read8(0) & LCD_R_BUSY) != 0)
		;
	
	_delay_us(1);
	return lcd_read8(0) & LCD_R_ADDR_MASK;
}

/* Issue a command to the LCD driver */
static void
lcd_command(int rs, uint8_t val)
{
	(void)lcd_waitbusy();
	lcd_write8(rs, val);
}

static int
getpos(int *x, int *y)
{
	uint8_t i, xx, yy, addr = lcd_waitbusy();

	xx = yy = 0xff;
	for (i = 0; i < LCD_ROWS; i++) {
		if (addr >= row_addrs[i] && addr < (row_addrs[i] + LCD_COLS)) {
			xx = addr - row_addrs[i];
			yy = i;
			break;
		}
	}
	if (xx >= LCD_COLS || yy >= LCD_ROWS) {
		/* wtf? */
		return 0;
	} else {
		if (x != NULL)
			*x = xx;
		if (y != NULL)
			*y = yy;
		return 1;
	}
}

void
lcd_home(void)
{
	lcd_command(0, LCD_C_RETURN);
}

void
lcd_display(int display_on, int cursor_on, int blink_on)
{
	lcd_command(0, LCD_C_DISPLAY_ON_OFF |
	    (display_on ? LCD_O_DISPLAY_ON : 0) |
	    (cursor_on ? LCD_O_DISPLAY_CURSOR : 0) |
	    (blink_on ? LCD_O_DISPLAY_BLINK : 0));
}

void
lcd_entry_mode(int rtl, int shift)
{
	lcd_command(0, LCD_C_ENTRY_MODE |
	    (rtl ? 0 : LCD_O_ENTRY_LTR) |
	    (shift ? LCD_O_ENTRY_SHIFT : 0));
}

/* Clear the screen */
void
lcd_clear(void)
{
	lcd_command(0, LCD_C_CLEAR);
}

/* Clear to the end of the current line only */
void
lcd_clear_eol(void)
{
	int x, i;

	if (getpos(&x, NULL)) {
		for (i = LCD_COLS - x; i > 0; i--)
			lcd_command(1, ' ');
	}
}

/* Write a string to the LCD without bounds-checking */
static void
lcd_write_string_raw(const char *s, int len)
{
	while (*s != '\0' && (len == -1 || len-- > 0))
		lcd_command(1, *s++);
}

/* Display an error message */
static void
lcd_error(const char *s)
{
	lcd_clear();
	lcd_home();
	lcd_write_string_raw("XXXXXXXXXXXXXXXXXXXX", 20);
	lcd_command(0, LCD_C_SET_DDRAM_ADDR | row_addrs[(LCD_ROWS - 1)]);
	lcd_write_string_raw(s, 20);
	lcd_home();
}

void
lcd_moveto(int x, int y)
{
	if (x >= LCD_COLS || y >= LCD_ROWS) {
		lcd_error("lcd moveto error");
		return;
	}
	lcd_command(0, LCD_C_SET_DDRAM_ADDR | (row_addrs[y] + x));
}

void
lcd_getpos(int *x, int *y)
{
	if (!getpos(x, y)) {
		*x = *y = 0;
		lcd_error("bad lcd address");
	}
}

void
lcd_string(const char *s)
{
	int x;

	lcd_getpos(&x, NULL);
	if (x >= LCD_COLS)
		return; /* Shouldn't happen */
	lcd_write_string_raw(s, LCD_COLS - x);
}

void
lcd_chars(const char *s, size_t len)
{
	int x;

	lcd_getpos(&x, NULL);
	if (x >= LCD_COLS)
		return; /* Shouldn't happen */
	if ((len + x) >= LCD_COLS)
		len = LCD_COLS - x;
	for (; len > 0; len--)
		lcd_command(1, *s++);
}

void
lcd_char(char c)
{
	int x;

	lcd_getpos(&x, NULL);
	if (x >= LCD_COLS)
		return;
	lcd_command(1, c);
}

void
lcd_fill(char c, size_t n)
{
	int x;

	lcd_getpos(&x, NULL);
	if (x >= LCD_COLS)
		return;
	if ((n + x) >= LCD_COLS)
		n = LCD_COLS - x;
	while (n-- > 0)
		lcd_command(1, c);
}

void
lcd_program_char(int c, uint8_t *data, size_t len)
{
	if (c > LCD_NUM_CG_CHARS || len != 8) {
		lcd_error("bad character data");
		return;
	}
	lcd_command(0, LCD_C_SET_CGRAM_ADDR | (c * 8));
	for (; len > 0; len--)
		lcd_command(1, *data++);
	lcd_home(); /* back to display memory */
}

void
lcd_brightness(int level)
{
	if (level < 0)
		level = 0;
	else if (level > 3)
		level = 3;

	/* Brightness has to follow function-set */
	lcd_command(0, LCD_C_FUNC_SET | LCD_O_FUNC_4BIT);
	lcd_command(1, 3 - level);
}

void
lcd_setup(void)
{
	LCD_CTL_DDR |= LCD_CTL_MASK;
	LCD_DB_DDR |= LCD_DB_MASK;
	LCD_EN_DDR |= (1 << LCD_EN);
	LCD_CTL_PORT &= ~LCD_CTL_MASK;
	LCD_EN_PORT &= ~(1 << LCD_EN);
	/* Ensure LCD has had enough time to settle */
	_delay_ms(100);
	/* Put it into 4-bit mode */
	lcd_write4(0, 0x3);
	_delay_ms(10);
	lcd_write4(0, 0x3);
	_delay_ms(1);
	lcd_write4(0, 0x3);
	_delay_ms(1);
	lcd_command(0, LCD_C_FUNC_SET | LCD_O_FUNC_4BIT);
	_delay_ms(10);
	/* Display is now in 4-bit mode */

	/* Two-line, 5x8 font */
	lcd_command(0, LCD_C_FUNC_SET | LCD_O_FUNC_4BIT | LCD_O_FUNC_2LINES |
	    LCD_O_FUNC_FONT_5X8);

	/* Display off */
	lcd_display(0, 0, 0);
	lcd_clear();
	/* Entry mode: advance and scroll XXX */
	lcd_entry_mode(0, 0);
	lcd_home();
	/* Display enable, cursor off, blink off */
	lcd_display(1, 0, 0);
}

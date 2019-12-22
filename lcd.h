#ifndef LCD_H
#define LCD_H

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

/* Driver for a HD44870-style 20x2 LCD */

#define LCD_ROWS	4
#define LCD_COLS	8

/* Maximum number of words supported for lcd_centered_string() */
#define LCD_MAX_WORDS	16

/*
 * Pin/port configuration. It's possible to have the control and data signals
 * on different ports.
 */
#define LCD_CTL_DDR	DDRF
#define LCD_CTL_PORT	PORTF
#define LCD_CTL_RS	0
#define LCD_CTL_RW	1

#define LCD_EN_DDR	DDRD
#define LCD_EN_PORT	PORTD
#define LCD_EN		7

#define LCD_DB_DDR	DDRF
#define LCD_DB_PORT	PORTF
#define LCD_DB_PIN	PINF
#define LCD_DB_4	7
#define LCD_DB_5	6
#define LCD_DB_6	5
#define LCD_DB_7	4

/* LCD special characters */
#define LCD_CHAR_ARROW_R	0x7e
#define LCD_CHAR_ARROW_L	0x7f
/* XXX TODO: Japanese characters if I ever need them; 0xa1-0xdf */
#define LCD_CHAR_DEGREES	0xdf	/* Actually a maru diacritic */
#define LCD_CHAR_ALPHA		0xe0
#define LCD_CHAR_A_UMLAUT	0xe1
#define LCD_CHAR_BETA		0xe2
#define LCD_CHAR_EPSILON	0xe3
#define LCD_CHAR_MU		0xe4
#define LCD_CHAR_SIGMA		0xe5
#define LCD_CHAR_RHO		0xe6
/* 0xe7 is a stylised 'g'; doesn't look like gamma */
#define LCD_CHAR_SQRT		0xe8
#define LCD_CHAR_INV		0xe9 /* ^{-1} */
#define LCD_CHAR_U_ASTERISK	0xea /* ^{*} */
/* 0xeb is a stylised 'j'; no idea... */
#define LCD_CHAR_CENT		0xec
/* 0xed looks like a superscripted Hiragana 'mo' */
#define LCD_CHAR_N_MACRON	0xee
#define LCD_CHAR_O_UMLAUT	0xef
/* 0xf0 is a stylised 'p'; no idea... */
/* 0xf1 is a stylised 'q'; maybe a vertically-mirrored 'p'; cyrillic? */
#define LCD_CHAR_THETA		0xf2
#define LCD_CHAR_INFINITY	0xf3 /* At least I guess so... */
#define LCD_CHAR_OMEGA		0xf4
#define LCD_CHAR_U_UMLAUT	0xf5
#define LCD_CHAR_U_SIGMA	0xf6
#define LCD_CHAR_PI		0xf7
#define LCD_CHAR_X_BAR		0xf8 /* no idea... maybe multiplication? */
/* 0xf9 is a stylised 'y'; no idea... */
#define LCD_CHAR_SEN		0xfa /* Kanji for thousand */
#define LCD_CHAR_MAN		0xfb /* Kanji for ten thousand */
#define LCD_CHAR_YEN		0xfc
#define LCD_CHAR_DIVIDE		0xfd
/* 0xfe is blank */
#define LCD_CHAR_BLOCK		0xff


/*
 * Prepare the display.
 * After this the LCD will be blank with cursor visible.
 */
void lcd_setup(void);

/* Turn the display, cursor and cursor blinking off or on */
void lcd_display(int display_on, int cursor_on, int blink_on);

/*
 * Control the entry mode, whether text flow is left-to-right (rtl = 0) or
 0 right-to-left (rtl = 1) and whether the cursor shifts after text entry
 * (shift = 0) or the text * shifts and the cursor remains stationary
 * (shift = 1).
 */
void lcd_entry_mode(int rtl, int shift);

/* Clear the screen */
void lcd_clear(void);

/* Clear to the end of line only */
void lcd_clear_eol(void);

/* Return the cursor to (0, 0) */
void lcd_home(void);

/*
 * Move the cursor to a particular location. Out-of-bounds locations will
 * cause an error to be displayed.
 */
void lcd_moveto(int x, int y);

/* Return the current cursor position */
void lcd_getpos(int *x, int *y);

/*
 * Write a string at the current cursor position. Strings that won't fit
 * the display will be truncated.
 */
void lcd_string(const char *s);

/*
 * Write an array of characters to the screen. Useful for writing the
 * \0 custom character.
 */
void lcd_chars(const char *s, size_t len);

/* Write a single character to the screen. */
void lcd_char(char c);

/* Write 'n' characters of 'c' to the screen. */
void lcd_fill(char c, size_t n);

/*
 * Program one of the user-defined characters.
 * data must be an 8 byte array containing the row bitmaps of the character.
 */
void lcd_program_char(int c, uint8_t *data, size_t len);

/* Output a \n-broken string string, trying to centre each line on the screen */
void lcd_centered_string(const char *s);

/* Centre a string on a single row */
void lcd_centred_string_row(int row, const char *s);

#endif /* LCD_H */

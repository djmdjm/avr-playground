/*
 * Copyright (c) 2019 Damien Miller <djm@mindrot.org>
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

void
lcd_centred_string_row(int row, const char *s)
{
	size_t need = strlen(s);

	lcd_moveto((LCD_COLS - need) / 2, row);
	lcd_chars(s, need);
}

/*
 * Split a string into lines (each line must end with \n) and centre each
 * line on its display row.
 */
void
lcd_centered_string(const char *s)
{
	const char *cp, *word;
	size_t i = 0, need;

	/* split string into words */
	for (cp = word = s; ; cp++) {
		if (*cp != '\n' && *cp != '\0')
			continue;
		/* end of word */
		need = cp - word;
		if (need > LCD_COLS)
			goto badstr;
		lcd_moveto((LCD_COLS - need) / 2, i);
		lcd_chars(word, need);
		i++;
		if (*cp == '\0')
			break;
		if (i >= LCD_ROWS) {
 badstr:
			lcd_clear();
			lcd_moveto(0, 0);
			lcd_string("!");
			lcd_moveto(2, 0);
			lcd_string(s);
			return;
		}
		word = cp + 1;
	}
}

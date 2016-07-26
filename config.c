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

#include "config.h"

static const struct config default_cfg = {
	/* XXX */
};

struct config cfg; /* XXX init */

void
reset_cfg(void)
{
	cfg = default_cfg;
}

void
reset_tune(void)
{
	/* XXX */
}

void
save_cfg(void)
{
	/* XXX */
}

void
update_firmware(void)
{
	/* XXX */
}

int
get_evtype(void)
{
	/* XXX */
	return -1;
}

void
set_evtype(int t)
{
	/* XXX */
}

int
get_poly(void)
{
	/* XXX */
	return -1;
}

void
set_poly(int p)
{
	/* XXX */
}

int
get_midich(void)
{
	/* XXX */
	return -1;
}

void
set_midich(int o)
{
	/* XXX */
}

int
get_output_range(void)
{
	/* XXX */
	return -1;
}

void
set_output_range(int p)
{
	/* XXX */
}

int
get_scale(void)
{
	/* XXX */
	return -1;
}

void
set_scale(int s)
{
	/* XXX */
}

int
get_offset(void)
{
	/* XXX */
	return -1;
}

void
set_offset(int o)
{
	/* XXX */
}

void
set_channel(int c)
{
	/* XXX */
}


#ifndef EVENT_H
#define EVENT_H

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
 * ACTION OF CONTRACT, NEGLIGEVENTE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Simple event queue */

/* Initialise the event queue */
void event_setup(void);

/*
 * Enqueue an event. Events marked as 'important' will clobber existing
 * entries on the queue rather than overflowing. Returns true if the event
 * was successfully enqueued or false if an overflow prevented the queueing.
 */
bool event_enqueue(uint8_t type, uint8_t v1, uint8_t v2, uint8_t v3,
    int important);

/* Drain all events from queue */
void event_drain(void);

/*
 * Poll to dequeue an event. Returns true if an event was dequeued or
 * false if none were pending.
 */
bool event_dequeue(uint8_t *type, uint8_t *v1, uint8_t *v2, uint8_t *v3);

/*
 * Return the number of pending events on the queue. Beware TOCOTU if the
 * queue is accessed in interrupt context.
 */
size_t event_nqueued(void);

/* Returns the maximum depth of the event queue to date */
size_t event_maxqueued(void);

/* Return non-zero if the event queue has overflowed */
bool event_queue_overflowed(void);

/* Reset overflowed flag */
void event_reset_overflowed(void);

/* Sleep the CPU until an interrupt generated an dequeueable event. */
void event_sleep(int sleep_mode, uint8_t *type,
    uint8_t *v1, uint8_t *v2, uint8_t *v3);

#endif /* EVENT_H */


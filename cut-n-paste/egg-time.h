/* egg-time.h
 *
 * Copyright (C) 2010 Christian Hergert <chris@dronelabs.com>
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __EGG_TIME_H__
#define __EGG_TIME_H__

#include <glib.h>
#include <time.h>
#include <math.h>

#ifndef G_NSEC_PER_SEC
#define G_NSEC_PER_SEC 1000000000
#endif

/**
 * timespec_compare:
 * @ts1: A timespec.
 * @ts2: A timespec.
 *
 * A qsort style compare function.
 *
 * Returns: greater than zero if @ts1 is greater than @ts2. less than one
 *   if @ts2 is greater than @ts1. otherwise, zero.
 * Side effects: None.
 */
static inline gint
timespec_compare (const struct timespec *ts1, /* IN */
                  const struct timespec *ts2) /* IN */
{
	if ((ts1->tv_sec == ts2->tv_sec) && (ts1->tv_nsec == ts2->tv_nsec)) {
		return 0;
	} else if ((ts1->tv_sec > ts2->tv_sec) ||
	         ((ts1->tv_sec == ts2->tv_sec) &&
	          (ts1->tv_nsec > ts2->tv_nsec))) {
		return 1;
	} else {
		return -1;
	}
}

/**
 * timespec_subtract:
 * @x: A struct timespec.
 *
 * Subtracts @y from @x and stores the result in @z.
 *
 * Returns: None.
 * Side effects: None.
 */
static inline void
timespec_subtract (struct timespec *x, /* IN */
                   struct timespec *y, /* IN */
                   struct timespec *z) /* OUT */
{
	z->tv_sec = (x->tv_sec - y->tv_sec);
	if ((z->tv_nsec = (x->tv_nsec - y->tv_nsec)) < 0) {
		z->tv_nsec += G_NSEC_PER_SEC;
		z->tv_sec -= 1;
	}
}

/**
 * timespec_add:
 * @x: A struct timespec to add.
 * @y: A struct timespec to add.
 * @z: A struct timespec to store the result.
 *
 * Adds @x and @y together and stores the result in @z.
 *
 * Returns: None.
 * Side effects: None.
 */
static inline void
timespec_add (struct timespec *x, /* IN */
              struct timespec *y, /* IN */
              struct timespec *z) /* OUT */
{
	z->tv_sec = x->tv_sec + y->tv_sec;
	if ((z->tv_nsec = (x->tv_nsec + y->tv_nsec)) > G_NSEC_PER_SEC) {
		z->tv_nsec -= G_NSEC_PER_SEC;
		z->tv_sec += 1;
	}
}

/**
 * timespec_from_double:
 * @d: A potentially-fractional number of seconds.
 * @ts: A destination struct timespec.
 *
 * Sets @ts to represent the number of seconds denoted in @d.
 *
 * Returns: Value converted to timespec in @ts.
 * Side effects: None.
 */
static inline void
timespec_from_double(double d,            /* IN */
                     struct timespec *ts) /* OUT */
{
	double feh;

	ts->tv_sec = (time_t)d;
	ts->tv_nsec = modf(d, &feh) * 1e9;
}

/**
 * timespec_to_usec:
 * @t: A struct timespec.
 * @u: A location for the uint64 encoding.
 *
 * Encodes @t in microseconds since the Epoch and stores the result in @u.
 *
 * Returns: None.
 * Side effects: None.
 */
static inline void
timespec_to_usec (struct timespec *t, /* IN */
                  guint64         *u) /* IN */
{
	*u = (t->tv_sec * (guint64)G_USEC_PER_SEC);
	*u += (t->tv_nsec / G_GUINT64_CONSTANT(1000));
}

/**
 * timespec_from_usec:
 * @t: A struct timespec.
 * @u: Microseconds since the Epoch.
 *
 * Decodes u back into a struct timespec and stores it in @t.
 *
 * Returns: None.
 * Side effects: None.
 */
static inline void
timespec_from_usec (struct timespec *t, /* OUT */
                    guint64          u) /* IN */
{
	t->tv_sec = u / (guint64)G_USEC_PER_SEC;
	t->tv_nsec = (u % (guint64)G_USEC_PER_SEC) * G_GUINT64_CONSTANT(1000);
}

#endif /* __EGG_TIME_H__ */

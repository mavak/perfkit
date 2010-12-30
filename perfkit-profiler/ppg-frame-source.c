/*
 * Clutter.
 *
 * An OpenGL based 'interactive canvas' library.
 *
 * Authored By Neil Roberts  <neil@linux.intel.com>
 *
 * Copyright (C) 2008 OpenedHand
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 *
 */

/* Modified by Christian Hergert for use in Ppg Graph. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ppg-frame-source.h"
#include "ppg-timeout-interval.h"

typedef struct _PpgFrameSource PpgFrameSource;

struct _PpgFrameSource
{
  GSource source;

  PpgTimeoutInterval timeout;
};

static gboolean ppg_frame_source_prepare  (GSource     *source,
                                               gint        *timeout);
static gboolean ppg_frame_source_check    (GSource     *source);
static gboolean ppg_frame_source_dispatch (GSource     *source,
                                               GSourceFunc  callback,
                                               gpointer     user_data);

static GSourceFuncs ppg_frame_source_funcs =
{
  ppg_frame_source_prepare,
  ppg_frame_source_check,
  ppg_frame_source_dispatch,
  NULL
};

/**
 * ppg_frame_source_add_full:
 * @priority: the priority of the frame source. Typically this will be in the
 *   range between %G_PRIORITY_DEFAULT and %G_PRIORITY_HIGH.
 * @fps: the number of times per second to call the function
 * @func: function to call
 * @data: data to pass to the function
 * @notify: function to call when the timeout source is removed
 *
 * Sets a function to be called at regular intervals with the given
 * priority.  The function is called repeatedly until it returns
 * %FALSE, at which point the timeout is automatically destroyed and
 * the function will not be called again.  The @notify function is
 * called when the timeout is destroyed.  The first call to the
 * function will be at the end of the first @interval.
 *
 * This function is similar to g_timeout_add_full() except that it
 * will try to compensate for delays. For example, if @func takes half
 * the interval time to execute then the function will be called again
 * half the interval time after it finished. In contrast
 * g_timeout_add_full() would not fire until a full interval after the
 * function completes so the delay between calls would be 1.0 / @fps *
 * 1.5. This function does not however try to invoke the function
 * multiple times to catch up missing frames if @func takes more than
 * @interval ms to execute.
 *
 * Return value: the ID (greater than 0) of the event source.
 *
 * Since: 0.8
 */
guint
ppg_frame_source_add_full (gint           priority,
                           guint          fps,
                           GSourceFunc    func,
                           gpointer       data,
                           GDestroyNotify notify)
{
  guint ret;
  GSource *source = g_source_new (&ppg_frame_source_funcs,
                                  sizeof (PpgFrameSource));
  PpgFrameSource *frame_source = (PpgFrameSource *) source;

  _ppg_timeout_interval_init (&frame_source->timeout, fps);

  if (priority != G_PRIORITY_DEFAULT)
    g_source_set_priority (source, priority);

#if GLIB_CHECK_VERSION (2, 25, 8)
  g_source_set_name (source, "Ppg frame timeout");
#endif

  g_source_set_callback (source, func, data, notify);

  ret = g_source_attach (source, NULL);

  g_source_unref (source);

  return ret;
}

/**
 * ppg_frame_source_add:
 * @fps: the number of times per second to call the function
 * @func: function to call
 * @data: data to pass to the function
 *
 * Simple wrapper around ppg_frame_source_add_full().
 *
 * Return value: the ID (greater than 0) of the event source.
 *
 * Since: 0.8
 */
guint
ppg_frame_source_add (guint       fps,
                      GSourceFunc func,
                      gpointer    data)
{
  return ppg_frame_source_add_full (G_PRIORITY_DEFAULT, fps, func, data, NULL);
}

static gboolean
ppg_frame_source_prepare (GSource *source,
                          gint    *delay)
{
  PpgFrameSource *frame_source = (PpgFrameSource *) source;
  GTimeVal current_time;

  g_source_get_current_time (source, &current_time);

  return _ppg_timeout_interval_prepare (&current_time,
                                        &frame_source->timeout,
                                        delay);
}

static gboolean
ppg_frame_source_check (GSource *source)
{
  return ppg_frame_source_prepare (source, NULL);
}

static gboolean
ppg_frame_source_dispatch (GSource     *source,
                           GSourceFunc  callback,
                           gpointer     user_data)
{
  PpgFrameSource *frame_source = (PpgFrameSource *) source;

  return _ppg_timeout_interval_dispatch (&frame_source->timeout,
                                         callback, user_data);
}

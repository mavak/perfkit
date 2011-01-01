/* ppg-util.h
 *
 * Copyright (C) 2010 Christian Hergert <chris@dronelabs.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PPG_UTIL_H__
#define __PPG_UTIL_H__

#include <gtk/gtk.h>

#include "ppg-animation.h"

G_BEGIN_DECLS

#if GTK_CHECK_VERSION(2, 91, 0)
#define HEXPAND(_tf) "hexpand", (_tf),
#define VEXPAND(_tf) "vexpand", (_tf),
#else
#define HEXPAND(_tf)
#define VEXPAND(_tf)
#endif

#define TO_CAIRO_RGB(c)    \
    ((c).red   / 65535.0), \
    ((c).green / 65535.0), \
    ((c).blue  / 65535.0)

#define LOAD_INLINE_PIXBUF(n) gdk_pixbuf_new_from_inline(-1, (n), FALSE, NULL)

#define CAIRO_ASSERT_OK(cr) \
    g_assert_cmpint(cairo_status((cr)), ==, \
                    CAIRO_STATUS_SUCCESS)

#define RPC_OR_GOTO(_rpc, _args, _label)  \
    G_STMT_START {                        \
        ret = pk_connection_##_rpc _args; \
        if (!ret) {                       \
            goto _label;                  \
        }                                 \
    } G_STMT_END

#define RPC_OR_FAILURE(_rpc, _args) RPC_OR_GOTO(_rpc, _args, failure)

GType   ppg_cairo_surface_get_type (void) G_GNUC_CONST;
gchar*  ppg_color_to_hex           (const GdkColor  *color);
guint   ppg_color_to_uint          (const GdkColor  *color,
                                    guint            alpha);
void    ppg_cairo_add_color_stop   (cairo_pattern_t *pattern,
                                    gdouble          offset,
                                    const GdkColor  *color);
gchar*  ppg_format_time            (gdouble          time_);
guint   ppg_get_num_cpus           (void) G_GNUC_CONST;
gdouble ppg_get_time               (GTimeVal *tv);
gdouble ppg_get_current_time       (void);
const gchar*  ppg_util_uname             (void);
gsize         ppg_util_get_total_memory  (void);
void          ppg_util_load_ui           (GtkWidget       *widget,
                                          GtkActionGroup **actions,
                                          const gchar     *ui_data,
                                          const gchar     *first_widget,
                                          ...);
GtkWidget*    ppg_util_header_item_new   (const gchar      *label);

PpgAnimation* g_object_animate      (gpointer          object,
                                     PpgAnimationMode  mode,
                                     guint             duration_msec,
                                     const gchar      *first_property,
                                     ...) G_GNUC_NULL_TERMINATED;
PpgAnimation* g_object_animate_full (gpointer          object,
                                     PpgAnimationMode  mode,
                                     guint             duration_msec,
                                     guint             frame_rate,
                                     GDestroyNotify    notify,
                                     gpointer          notify_data,
                                     const gchar      *first_property,
                                     ...) G_GNUC_NULL_TERMINATED;

/**
 * ppg_clear_pointer:
 * @addr: (in): A pointer to the location of a pointer.
 * @destroy: (in): A function to free the location if needed.
 *
 * A helper function similar to g_clear_object() for freeing and
 * setting a location to %NULL. This allows for a configurable
 * free function.
 *
 * Returns: None.
 * Side effects: Pointer dereferenced is freed if non %NULL.
 */
static inline void
ppg_clear_pointer_ (gpointer       *addr,
                    GDestroyNotify  destroy)
{
	gpointer real_addr;

	g_return_if_fail(addr != NULL);

	if ((real_addr = *addr)) {
		*addr = NULL;
		destroy(real_addr);
	}
}


static inline void
ppg_clear_source (guint *handler)
{
	if (*handler) {
		g_source_remove(*handler);
		*handler = 0;
	}
}

#define ppg_clear_pointer(p,d) \
    ppg_clear_pointer_((gpointer *)(p), (GDestroyNotify)(d))

#define ppg_clear_object(p) ppg_clear_pointer((p), g_object_unref)

G_END_DECLS

#endif /* __PPG_UTIL_H__ */

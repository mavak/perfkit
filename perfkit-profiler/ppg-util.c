/* ppg-util.c
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

#include <glib-object.h>
#include <gobject/gvaluecollector.h>
#include <math.h>
#include <string.h>
#if __linux__
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#endif

#include "ppg-actions.h"
#include "ppg-util.h"

enum
{
	PALETTE_BASE,
	PALETTE_BG,
	PALETTE_FG,
};

const gchar*
ppg_util_uname (void)
{
	static gsize initialized = FALSE;
	static gchar *uname_str = NULL;
	struct utsname u;

	if (g_once_init_enter(&initialized)) {
		uname(&u);
		uname_str = g_strdup_printf("%s", u.release);
		g_once_init_leave(&initialized, TRUE);
	}

	return uname_str;
}

gsize
ppg_util_get_total_memory (void)
{
	return sysconf(_SC_PHYS_PAGES) * getpagesize();
}

void
ppg_util_load_ui (GtkWidget       *widget,
                  GtkActionGroup **actions,
                  const gchar     *ui_data,
                  const gchar     *first_widget,
                  ...)
{
	GtkUIManager *ui_manager;
	GtkActionGroup *action_group;
	GtkAccelGroup *accel_group;
	const gchar *name;
	GtkWidget **dst_widget;
	GError *error = NULL;
	GType type;
	va_list args;

	g_return_if_fail(GTK_IS_WIDGET(widget));
	g_return_if_fail(ui_data != NULL);
	g_return_if_fail(first_widget != NULL);

	ui_manager = gtk_ui_manager_new();
	if (!gtk_ui_manager_add_ui_from_string(ui_manager, ui_data, -1, &error)) {
		g_error("%s", error->message); /* Fatal */
	}

	type = G_TYPE_FROM_INSTANCE(widget);
	accel_group = gtk_ui_manager_get_accel_group(ui_manager);
	action_group = gtk_action_group_new(g_type_name(type));
	ppg_actions_load(widget, accel_group, action_group);
	gtk_ui_manager_insert_action_group(ui_manager, action_group, 0);

	name = first_widget;
	va_start(args, first_widget);

	do {
		dst_widget = va_arg(args, GtkWidget**);
		*dst_widget = gtk_ui_manager_get_widget(ui_manager, name);
	} while ((name = va_arg(args, const gchar*)));

	if (actions) {
		*actions = action_group;
	}
}

static void
ppg_util_header_item_on_style_set (GtkWidget *widget,
                                   GtkStyle  *old_style,
                                   GtkWidget *child)
{
	GtkStyle *style;

	style = gtk_widget_get_style(widget);
	gtk_widget_set_style(child, style);
}

GtkWidget*
ppg_util_header_item_new (const gchar *label)
{
	GtkWidget *item;
	GtkLabel *child;

	child = g_object_new(GTK_TYPE_LABEL,
	                     "label", label,
	                     "visible", TRUE,
	                     "xalign", 0.0f,
	                     "yalign", 0.5f,
	                     NULL);

	item = g_object_new(GTK_TYPE_MENU_ITEM,
	                    "child", child,
	                    NULL);

	g_signal_connect(item, "style-set",
	                 G_CALLBACK(ppg_util_header_item_on_style_set),
	                 child);

	gtk_menu_item_select(GTK_MENU_ITEM(item));

	return item;
}

PpgAnimation*
g_object_animatev (gpointer          object,
                   PpgAnimationMode  mode,
                   guint             duration_msec,
                   guint             frame_rate,
                   const gchar      *first_property,
                   va_list           args)
{
	PpgAnimation *animation;
	GObjectClass *klass;
	GObjectClass *pklass;
	const gchar *name;
	GParamSpec *pspec;
	GtkWidget *parent;
	GValue value = { 0 };
	gchar *error = NULL;
	GType type;
	GType ptype;

	g_return_val_if_fail(first_property != NULL, NULL);
	g_return_val_if_fail(mode < PPG_ANIMATION_LAST, NULL);

	name = first_property;
	type = G_TYPE_FROM_INSTANCE(object);
	klass = G_OBJECT_GET_CLASS(object);
	animation = g_object_new(PPG_TYPE_ANIMATION,
	                         "duration", duration_msec,
	                         "frame-rate", frame_rate ? frame_rate : 60,
	                         "mode", mode,
	                         "target", object,
	                         NULL);

	do {
		/*
		 * First check for the property on the object. If that does not exist
		 * then check if the object has a parent and look at its child
		 * properties (if its a GtkWidget).
		 */
		if (!(pspec = g_object_class_find_property(klass, name))) {
			if (!g_type_is_a(type, GTK_TYPE_WIDGET)) {
				g_critical("Failed to find property %s in %s",
				           name, g_type_name(type));
				goto failure;
			}
			if (!(parent = gtk_widget_get_parent(object))) {
				g_critical("Failed to find property %s in %s",
				           name, g_type_name(type));
				goto failure;
			}
			pklass = G_OBJECT_GET_CLASS(parent);
			ptype = G_TYPE_FROM_INSTANCE(parent);
			if (!(pspec = gtk_container_class_find_child_property(pklass, name))) {
				g_critical("Failed to find property %s in %s or parent %s",
				           name, g_type_name(type), g_type_name(ptype));
				goto failure;
			}
		}

		g_value_init(&value, pspec->value_type);
		G_VALUE_COLLECT(&value, args, 0, &error);
		if (error != NULL) {
			g_critical("Failed to retrieve va_list value: %s", error);
			g_free(error);
			goto failure;
		}

		ppg_animation_add_property(animation, pspec, &value);
		g_value_unset(&value);
	} while ((name = va_arg(args, const gchar *)));

	ppg_animation_start(animation);

	return animation;

failure:
	g_object_ref_sink(animation);
	g_object_unref(animation);
	return NULL;
}

PpgAnimation*
g_object_animate (gpointer          object,
                  PpgAnimationMode  mode,
                  guint             duration_msec,
                  const gchar      *first_property,
                  ...)
{
	PpgAnimation *animation;
	va_list args;

	va_start(args, first_property);
	animation = g_object_animatev(object, mode, duration_msec, 0,
	                              first_property, args);
	va_end(args);
	return animation;
}

PpgAnimation*
g_object_animate_full (gpointer          object,
                       PpgAnimationMode  mode,
                       guint             duration_msec,
                       guint             frame_rate,
                       GDestroyNotify    notify,
                       gpointer          notify_data,
                       const gchar      *first_property,
                       ...)
{
	PpgAnimation *animation;
	va_list args;

	va_start(args, first_property);
	animation = g_object_animatev(object, mode, duration_msec,
	                              frame_rate, first_property, args);
	va_end(args);
	g_object_weak_ref(G_OBJECT(animation), (GWeakNotify)notify, notify_data);
	return animation;
}


/**
 * ppg_color_to_hex:
 * @color: (in): A #GdkColor.
 *
 * Converts the color represented by @color into an HTML hex string
 * such as "#ffffff".
 *
 * Returns: A newly allocated string that should be freed with g_free().
 * Side effects: None.
 */
gchar *
ppg_color_to_hex (const GdkColor *color)
{
	return g_strdup_printf("#%0x%02x%02x",
	                       color->red / 255,
	                       color->green / 255,
	                       color->blue / 255);
}


/**
 * ppg_color_to_uint:
 * @color: (in): A #GdkColor.
 * @alpha: (in): An alpha value between 0x00 and 0xFF.
 *
 * Creates a 32-bit integer containing the value of @color and @alpha
 * in the format of 0xRRGGBBAA.
 *
 * Returns: A 32-bit integer containing the color value with alpha.
 * Side effects: None.
 */
guint
ppg_color_to_uint (const GdkColor *color,
                   guint           alpha)
{
	g_return_val_if_fail(color != NULL, 0);
	g_return_val_if_fail(alpha <= 0xFF, 0);

	return ((color->red / 255) << 24)
		 | ((color->green / 255) << 16)
		 | ((color->blue / 255) << 8)
		 | alpha;
}


/**
 * ppg_cairo_add_color_stop:
 * @pattern: (in): A #cairo_pattern_t.
 * @offset: (in): The offset for the color.
 * @color: (in): A #GdkColor.
 *
 * Adds a color stop for a gradient using a #GdkColor.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_cairo_add_color_stop (cairo_pattern_t *pattern,
                          gdouble          offset,
                          const GdkColor  *color)
{
	g_return_if_fail(pattern != NULL);
	g_return_if_fail(color != NULL);

	cairo_pattern_add_color_stop_rgb(pattern, offset, TO_CAIRO_RGB(*color));
}


/**
 * ppg_format_time:
 * @time_: (in): A double representing a precision time.
 *
 * Converts @time_ into a string representation such as "HH:MM:SS.MSEC".
 *
 * Returns: A newly allocated string that should be freed with g_free().
 * Side effects: None.
 */
gchar *
ppg_format_time (gdouble time_)
{
	gdouble frac;
	gdouble dummy;

	frac = modf(time_, &dummy);
	return g_strdup_printf("%02d:%02d:%02d.%04d",
	                       (gint)floor(time_/ 3600.0),
	                       (gint)floor(((gint)time_ % 3600) / 60.0),
	                       (gint)floor((gint)time_ % 60),
	                       (gint)floor(frac * 10000));
}


/**
 * ppg_get_num_cpus:
 *
 * Retrieves the number of CPUs available on the system.
 *
 * Returns: None.
 * Side effects: None.
 */
guint
ppg_get_num_cpus (void)
{
#if __linux__
	return get_nprocs();
#else
	return 1;
#endif
}


gdouble
ppg_get_time (GTimeVal *tv)
{
	return tv->tv_sec + (tv->tv_usec / (gdouble)G_USEC_PER_SEC);
}


gdouble
ppg_get_current_time (void)
{
	GTimeVal tv;

	g_get_current_time(&tv);
	return tv.tv_sec + (tv.tv_usec / (gdouble)G_USEC_PER_SEC);
}

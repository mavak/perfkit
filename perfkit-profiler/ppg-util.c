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
#include <sys/utsname.h>

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

static gboolean
ppg_util_real_expose_event (GtkWidget      *widget,
                            GdkEventExpose *event,
                            gint            palette)
{
	GtkAllocation alloc;
	GtkStateType state;
	GtkStyle *style;
	GdkGC *gc = NULL;

	if (GTK_WIDGET_DRAWABLE(widget)) {
		gtk_widget_get_allocation(widget, &alloc);
		style = gtk_widget_get_style(widget);
		state = gtk_widget_get_state(widget);

		switch (palette) {
		case PALETTE_BASE:
			gc = style->base_gc[state];
			break;
		case PALETTE_BG:
			gc = style->bg_gc[state];
			break;
		case PALETTE_FG:
			gc = style->fg_gc[state];
			break;
		default:
			g_assert_not_reached();
		}

		gdk_draw_rectangle(event->window, gc, TRUE, alloc.x, alloc.y,
		                   alloc.width, alloc.height);
	}

	return FALSE;
}

gboolean
ppg_util_base_expose_event (GtkWidget      *widget,
                            GdkEventExpose *event)
{
	return ppg_util_real_expose_event(widget, event, PALETTE_BASE);
}

gboolean
ppg_util_bg_expose_event (GtkWidget      *widget,
                          GdkEventExpose *event)
{
	return ppg_util_real_expose_event(widget, event, PALETTE_BG);
}

gboolean
ppg_util_fg_expose_event (GtkWidget      *widget,
                          GdkEventExpose *event)
{
	return ppg_util_real_expose_event(widget, event, PALETTE_FG);
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
ppg_widget_animate (GtkWidget        *widget,
                    guint             duration_msec,
                    PpgAnimationMode  mode,
                    const gchar      *first_property,
                    ...)
{
	PpgAnimation *animation;
	va_list args;

	va_start(args, first_property);
	animation = ppg_widget_animatev(widget, duration_msec, mode,
	                                first_property, args);
	va_end(args);
	return animation;
}

PpgAnimation*
ppg_widget_animate_full (GtkWidget        *widget,
                         guint             duration_msec,
                         PpgAnimationMode  mode,
                         GDestroyNotify    notify,
                         gpointer          notify_data,
                         const gchar      *first_property,
                         ...)
{
	PpgAnimation *animation;
	va_list args;

	va_start(args, first_property);
	animation = ppg_widget_animatev(widget, duration_msec, mode,
	                                first_property, args);
	va_end(args);
	g_object_weak_ref(G_OBJECT(animation), (GWeakNotify)notify, notify_data);
	return animation;
}

PpgAnimation*
ppg_widget_animatev (GtkWidget        *widget,
                     guint             duration_msec,
                     PpgAnimationMode  mode,
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

	g_return_val_if_fail(GTK_IS_WIDGET(widget), NULL);
	g_return_val_if_fail(first_property != NULL, NULL);
	g_return_val_if_fail(mode < PPG_ANIMATION_LAST, NULL);

	name = first_property;
	type = G_TYPE_FROM_INSTANCE(widget);
	klass = G_OBJECT_GET_CLASS(widget);
	animation = g_object_new(PPG_TYPE_ANIMATION,
	                         "duration", duration_msec,
	                         "mode", mode,
	                         "target", widget,
	                         NULL);

	do {
		/*
		 * First check for the property on the object. If that does not exist
		 * then check if the widget has a parent and look at its child
		 * properties.
		 */
		if (!(pspec = g_object_class_find_property(klass, name))) {
			if (!(parent = gtk_widget_get_parent(widget))) {
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

		G_VALUE_COLLECT_INIT(&value, pspec->value_type, args, 0, &error);
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

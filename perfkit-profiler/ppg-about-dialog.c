/* ppg-about-dialog.c
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

#include <glib/gi18n.h>

#include "icons/perfkit.h"

#include "ppg-about-dialog.h"
#include "ppg-util.h"

G_DEFINE_TYPE(PpgAboutDialog, ppg_about_dialog, GTK_TYPE_WINDOW)

static gboolean
ppg_about_dialog_bg_draw (GtkWidget *widget,
                          cairo_t   *cr,
                          gpointer   user_data)
{
	GtkStyle *style;
	GtkStateType state;
	GtkAllocation a;

	if (gtk_widget_is_drawable(widget)) {
		state = gtk_widget_get_state(widget);
		style = gtk_widget_get_style(widget);
		gtk_widget_get_allocation(widget, &a);
		cairo_rectangle(cr, 0, 0, a.width, a.height);
		gdk_cairo_set_source_color(cr, &style->light[state]);
		cairo_set_source_rgb(cr, 1, 1, 1);
		cairo_fill(cr);
	}

	return FALSE;
}

#if !GTK_CHECK_VERSION(2, 91, 0)
static void
ppg_about_dialog_bg_expose_event (GtkWidget      *widget,
                                  GdkEventExpose *expose,
                                  gpointer        user_data)
{
	cairo_t *cr;

	if (gtk_widget_is_drawable(widget)) {
		cr = gdk_cairo_create(expose->window);
		gdk_cairo_rectangle(cr, &expose->area);
		cairo_clip(cr);
		ppg_about_dialog_bg_draw(widget, cr, user_data);
		cairo_destroy(cr);
	}
}
#endif

static void
ppg_about_dialog_class_init (PpgAboutDialogClass *klass)
{
}

static void
ppg_about_dialog_init (PpgAboutDialog *dialog)
{
	GtkWidget *table;
	GtkWidget *box;
	GtkWidget *content;
	GtkWidget *frame_;
	GtkWidget *vbox;
	GtkWidget *header;
	GtkWidget *hbox;
	GtkWidget *image;
	GtkWidget *l;
	GtkWidget *buttons;
	GtkWidget *b;
	gchar *str;

	g_object_set(dialog,
	             "border-width", 6,
	             "resizable", FALSE,
	             "title", _("About " PRODUCT_NAME),
	             "window-position", GTK_WIN_POS_CENTER_ON_PARENT,
	             NULL);

	content = g_object_new(GTK_TYPE_VBOX,
	                       "spacing", 12,
	                       "visible", TRUE,
	                       NULL);
	gtk_container_add(GTK_CONTAINER(dialog), content);

	buttons = g_object_new(GTK_TYPE_HBOX,
	                       "border-width", 6,
	                       "homogeneous", FALSE,
	                       "spacing", 6,
	                       "visible", TRUE,
	                       NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(content), buttons,
	                                  "expand", FALSE,
	                                  "pack-type", GTK_PACK_END,
	                                  NULL);

	hbox = g_object_new(GTK_TYPE_HBOX,
	                    "border-width", 6,
	                    "spacing", 12,
	                    "visible", TRUE,
	                    NULL);
	gtk_container_add(GTK_CONTAINER(content), hbox);

	image = g_object_new(GTK_TYPE_IMAGE,
	                     "pixbuf", LOAD_INLINE_PIXBUF(perfkit_pixbuf),
	                     "visible", TRUE,
	                     "xalign", 0.5f,
	                     "yalign", 0.0f,
	                     NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(hbox), image,
	                                  "expand", FALSE,
	                                  NULL);

	box = g_object_new(GTK_TYPE_VBOX,
	                   "spacing", 12,
	                   "visible", TRUE,
	                   NULL);
	gtk_container_add(GTK_CONTAINER(hbox), box);

	frame_ = g_object_new(GTK_TYPE_FRAME,
	                      "visible", TRUE,
	                      NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(box), frame_,
	                                  "expand", FALSE,
	                                  NULL);
#if GTK_CHECK_VERSION(2, 91, 0)
	g_signal_connect(frame_, "draw",
	                 G_CALLBACK(ppg_about_dialog_bg_draw),
	                 NULL);
#else
	g_signal_connect(frame_, "expose-event",
	                 G_CALLBACK(ppg_about_dialog_bg_expose_event),
	                 NULL);
#endif

	vbox = g_object_new(GTK_TYPE_VBOX,
	                    "border-width", 12,
	                    "spacing", 6,
	                    "visible", TRUE,
	                    NULL);
	gtk_container_add(GTK_CONTAINER(frame_), vbox);

	table = g_object_new(GTK_TYPE_TABLE,
	                     "column-spacing", 0,
	                     "n-columns", 2,
	                     "n-rows", 8,
	                     "row-spacing", 6,
	                     "visible", TRUE,
	                     NULL);
	gtk_container_add(GTK_CONTAINER(vbox), table);

	header = ppg_util_header_item_new("Product Information");
	gtk_widget_show(header);
	gtk_container_add_with_properties(GTK_CONTAINER(table), header,
	                                  "left-attach", 0,
	                                  "right-attach", 2,
	                                  "top-attach", 0,
	                                  "bottom-attach", 1,
	                                  "y-options", GTK_FILL,
	                                  NULL);

	l = g_object_new(GTK_TYPE_LABEL,
	                 "label", "<b>Name:</b>",
	                 "use-markup", TRUE,
	                 "visible", TRUE,
	                 "xalign", 1.0f,
	                 "xpad", 12,
	                 NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(table), l,
	                                  "left-attach", 0,
	                                  "right-attach", 1,
	                                  "top-attach", 1,
	                                  "bottom-attach", 2,
	                                  "y-options", GTK_FILL,
	                                  "x-options", GTK_FILL,
	                                  NULL);

	l = g_object_new(GTK_TYPE_LABEL,
	                 "label", PRODUCT_NAME,
	                 "selectable", TRUE,
	                 "visible", TRUE,
	                 "xalign", 0.0f,
	                 NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(table), l,
	                                  "left-attach", 1,
	                                  "right-attach", 2,
	                                  "top-attach", 1,
	                                  "bottom-attach", 2,
	                                  "y-options", GTK_FILL,
	                                  NULL);

	l = g_object_new(GTK_TYPE_LABEL,
	                 "label", "<b>Version:</b>",
	                 "use-markup", TRUE,
	                 "visible", TRUE,
	                 "xalign", 1.0f,
	                 "xpad", 12,
	                 NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(table), l,
	                                  "left-attach", 0,
	                                  "right-attach", 1,
	                                  "top-attach", 2,
	                                  "bottom-attach", 3,
	                                  "y-options", GTK_FILL,
	                                  "x-options", GTK_FILL,
	                                  NULL);

	l = g_object_new(GTK_TYPE_LABEL,
	                 "label", PACKAGE_VERSION,
	                 "selectable", TRUE,
	                 "visible", TRUE,
	                 "xalign", 0.0f,
	                 NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(table), l,
	                                  "left-attach", 1,
	                                  "right-attach", 2,
	                                  "top-attach", 2,
	                                  "bottom-attach", 3,
	                                  "y-options", GTK_FILL,
	                                  NULL);

	header = ppg_util_header_item_new("System Information");
	gtk_widget_show(header);
	gtk_container_add_with_properties(GTK_CONTAINER(table), header,
	                                  "left-attach", 0,
	                                  "right-attach", 2,
	                                  "top-attach", 3,
	                                  "bottom-attach", 4,
	                                  "y-options", GTK_FILL,
	                                  NULL);

	l = g_object_new(GTK_TYPE_LABEL,
	                 "label", "<b>Machine:</b>",
	                 "use-markup", TRUE,
	                 "visible", TRUE,
	                 "xalign", 1.0f,
	                 "xpad", 12,
	                 NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(table), l,
	                                  "left-attach", 0,
	                                  "right-attach", 1,
	                                  "top-attach", 4,
	                                  "bottom-attach", 5,
	                                  "y-options", GTK_FILL,
	                                  "x-options", GTK_FILL,
	                                  NULL);

	l = g_object_new(GTK_TYPE_LABEL,
	                 "label", g_get_host_name(),
	                 "selectable", TRUE,
	                 "visible", TRUE,
	                 "xalign", 0.0f,
	                 NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(table), l,
	                                  "left-attach", 1,
	                                  "right-attach", 2,
	                                  "top-attach", 4,
	                                  "bottom-attach", 5,
	                                  "y-options", GTK_FILL,
	                                  NULL);

	l = g_object_new(GTK_TYPE_LABEL,
	                 "label", "Copyright © 2009-2010 Christian Hergert and others",
	                 "visible", TRUE,
	                 "xalign", 0.0f,
	                 NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(box), l,
	                                  "expand", FALSE,
	                                  NULL);

	l = g_object_new(GTK_TYPE_LABEL,
	                 "label", "<b>Memory:</b>",
	                 "use-markup", TRUE,
	                 "visible", TRUE,
	                 "xalign", 1.0f,
	                 "xpad", 12,
	                 NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(table), l,
	                                  "left-attach", 0,
	                                  "right-attach", 1,
	                                  "top-attach", 5,
	                                  "bottom-attach", 6,
	                                  "y-options", GTK_FILL,
	                                  "x-options", GTK_FILL,
	                                  NULL);

	str = g_format_size_for_display(ppg_util_get_total_memory());
	l = g_object_new(GTK_TYPE_LABEL,
	                 "label", str,
	                 "selectable", TRUE,
	                 "visible", TRUE,
	                 "xalign", 0.0f,
	                 NULL);
	g_free(str);
	gtk_container_add_with_properties(GTK_CONTAINER(table), l,
	                                  "left-attach", 1,
	                                  "right-attach", 2,
	                                  "top-attach", 5,
	                                  "bottom-attach", 6,
	                                  "y-options", GTK_FILL,
	                                  NULL);

	l = g_object_new(GTK_TYPE_LABEL,
	                 "label", "<b>OS Version:</b>",
	                 "use-markup", TRUE,
	                 "visible", TRUE,
	                 "xalign", 1.0f,
	                 "xpad", 12,
	                 "yalign", 0.0f,
	                 NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(table), l,
	                                  "left-attach", 0,
	                                  "right-attach", 1,
	                                  "top-attach", 6,
	                                  "bottom-attach", 7,
	                                  "y-options", GTK_FILL,
	                                  "x-options", GTK_FILL,
	                                  NULL);

	l = g_object_new(GTK_TYPE_LABEL,
	                 "label", ppg_util_uname(),
	                 "selectable", TRUE,
	                 "visible", TRUE,
	                 "xalign", 0.0f,
	                 NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(table), l,
	                                  "left-attach", 1,
	                                  "right-attach", 2,
	                                  "top-attach", 6,
	                                  "bottom-attach", 7,
	                                  "y-options", GTK_FILL,
	                                  NULL);

	l = g_object_new(GTK_TYPE_LABEL,
	                 "label", "<b>Gtk+ Version:</b>",
	                 "use-markup", TRUE,
	                 "visible", TRUE,
	                 "xalign", 1.0f,
	                 "xpad", 12,
	                 "yalign", 0.0f,
	                 NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(table), l,
	                                  "left-attach", 0,
	                                  "right-attach", 1,
	                                  "top-attach", 7,
	                                  "bottom-attach", 8,
	                                  "y-options", GTK_FILL,
	                                  "x-options", GTK_FILL,
	                                  NULL);

	str = g_strdup_printf("%d.%d.%d",
#if GTK_CHECK_VERSION(2, 91, 0)
	                      gtk_get_major_version(),
	                      gtk_get_minor_version(),
	                      gtk_get_micro_version());
#else
	                      gtk_major_version,
	                      gtk_minor_version,
	                      gtk_micro_version);
#endif
	l = g_object_new(GTK_TYPE_LABEL,
	                 "label", str,
	                 "selectable", TRUE,
	                 "visible", TRUE,
	                 "xalign", 0.0f,
	                 NULL);
	g_free(str);
	gtk_container_add_with_properties(GTK_CONTAINER(table), l,
	                                  "left-attach", 1,
	                                  "right-attach", 2,
	                                  "top-attach", 7,
	                                  "bottom-attach", 8,
	                                  "y-options", GTK_FILL,
	                                  NULL);

	l = g_object_new(GTK_TYPE_LABEL,
	                 "label", "Perfkit is licensed under the GNU GPL; either version\n"
	                          "3 of the License, or (at your option) any later\n"
	                          "version.",
	                 "visible", TRUE,
	                 "justify", GTK_JUSTIFY_LEFT,
	                 "xalign", 0.0f,
	                 NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(box), l,
	                                  "expand", FALSE,
	                                  NULL);

	l = g_object_new(GTK_TYPE_LABEL,
	                 "label", "<a href=\"http://perfkit.org\">http://perfkit.org</a>",
	                 "use-markup", TRUE,
	                 "visible", TRUE,
	                 NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(box), l,
	                                  "expand", FALSE,
	                                  NULL);

	b = g_object_new(GTK_TYPE_BUTTON,
	                 "label", _("Collect _Support Data"),
	                 "use-underline", TRUE,
	                 "visible", TRUE,
	                 "width-request", 160,
	                 NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(buttons), b,
	                                  "expand", FALSE,
	                                  "pack-type", GTK_PACK_END,
	                                  NULL);

	b = g_object_new(GTK_TYPE_BUTTON,
	                 "can-default", TRUE,
	                 "label", GTK_STOCK_CLOSE,
	                 "use-stock", TRUE,
	                 "visible", TRUE,
	                 "width-request", 100,
	                 NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(buttons), b,
	                                  "expand", FALSE,
	                                  "pack-type", GTK_PACK_END,
	                                  "position", 0,
	                                  NULL);
	g_signal_connect_swapped(b, "clicked",
	                         G_CALLBACK(gtk_widget_destroy),
	                         dialog);

	gtk_window_set_default(GTK_WINDOW(dialog), b);
	gtk_window_set_focus(GTK_WINDOW(dialog), b);
}

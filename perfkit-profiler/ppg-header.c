/* ppg-header.c
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

#include "ppg-header.h"
#include "ppg-util.h"


G_DEFINE_TYPE(PpgHeader, ppg_header, GTK_TYPE_DRAWING_AREA)


struct _PpgHeaderPrivate
{
	gboolean right_separator;
	gboolean bottom_separator;
};


enum
{
	PROP_0,
	PROP_BOTTOM_SEPARATOR,
	PROP_RIGHT_SEPARATOR,
};


/**
 * ppg_header_draw:
 * @widget: (in): A #PpgHeader.
 * @cr: (in): A #cairo_t.
 *
 * Draws the background for the header widget.
 *
 * Returns: %FALSE always.
 * Side effects: Background is drawn to @cr.
 */
static gboolean
ppg_header_draw (GtkWidget *widget,
                 cairo_t   *cr)
{
	PpgHeader *header = (PpgHeader *)widget;
	PpgHeaderPrivate *priv;
	cairo_pattern_t *p;
	GtkAllocation alloc;
	GtkStateType state = GTK_STATE_NORMAL;
	GtkStyle *style;
	GdkColor begin;
	GdkColor end;
	GdkColor line;
	GdkColor v_begin;
	GdkColor v_end;

	g_return_val_if_fail(PPG_IS_HEADER(header), FALSE);

	priv = header->priv;

	gtk_widget_get_allocation(widget, &alloc);
	style = gtk_widget_get_style(widget);
	begin = style->light[state];
	end = style->mid[state];
	line = style->dark[state];
	v_begin = style->mid[state];
	v_end = style->dark[state];

	p = cairo_pattern_create_linear(0, 0, 0, alloc.height);

	cairo_pattern_add_color_stop_rgb(p, 0.0, TO_CAIRO_RGB(begin));
	cairo_pattern_add_color_stop_rgb(p, 1.0, TO_CAIRO_RGB(end));
	cairo_set_source(cr, p);
	cairo_rectangle(cr, 0, 0, alloc.width, alloc.height);
	cairo_fill(cr);
	cairo_pattern_destroy(p);

	if (priv->bottom_separator) {
		cairo_set_source_rgb(cr, TO_CAIRO_RGB(line));
		cairo_set_line_width(cr, 1.0);
		cairo_move_to(cr, 0, alloc.height - 0.5);
		cairo_line_to(cr, alloc.width, alloc.height - 0.5);
		cairo_stroke(cr);
	}

	if (priv->right_separator) {
		p = cairo_pattern_create_linear(0, 0, 0, alloc.height);
		cairo_pattern_add_color_stop_rgb(p, 0.0, TO_CAIRO_RGB(v_begin));
		cairo_pattern_add_color_stop_rgb(p, 1.0, TO_CAIRO_RGB(v_end));
		cairo_set_source(cr, p);
		cairo_set_line_width(cr, 1.0);
		cairo_move_to(cr, alloc.width - 0.5, 0);
		cairo_line_to(cr, alloc.width - 0.5, alloc.height);
		cairo_stroke(cr);
		cairo_pattern_destroy(p);
	}

	return FALSE;
}


/**
 * ppg_header_expose_event:
 * @header: (in): A #PpgHeader.
 *
 * Handles the "expose-event" on older versions of Gtk. Uses the new model
 * of GtkWidget::draw() to perform the drawing.
 *
 * Returns: %FALSE unless event propagation should stop.
 * Side effects: None.
 */
#if !GTK_CHECK_VERSION(2, 91, 0)
static gboolean
ppg_header_expose_event (GtkWidget      *widget,
                         GdkEventExpose *expose)
{
	gboolean ret = FALSE;
	cairo_t *cr;

	if (gtk_widget_is_drawable(widget)) {
		cr = gdk_cairo_create(expose->window);
		gdk_cairo_rectangle(cr, &expose->area);
		cairo_clip(cr);
		ret = ppg_header_draw(widget, cr);
		cairo_destroy(cr);
	}

	return ret;
}
#endif


/**
 * ppg_header_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (out): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Get a given #GObject property.
 */
static void
ppg_header_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
	PpgHeader *header = PPG_HEADER(object);

	switch (prop_id) {
	case PROP_BOTTOM_SEPARATOR:
		g_value_set_boolean(value, header->priv->right_separator);
		break;
	case PROP_RIGHT_SEPARATOR:
		g_value_set_boolean(value, header->priv->bottom_separator);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}


/**
 * ppg_header_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
static void
ppg_header_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
	PpgHeader *header = PPG_HEADER(object);
	GtkWidget *widget = GTK_WIDGET(object);

	switch (prop_id) {
	case PROP_BOTTOM_SEPARATOR:
		header->priv->bottom_separator = g_value_get_boolean(value);
		gtk_widget_queue_draw(widget);
		break;
	case PROP_RIGHT_SEPARATOR:
		header->priv->right_separator = g_value_get_boolean(value);
		gtk_widget_queue_draw(widget);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}


/**
 * ppg_header_class_init:
 * @klass: (in): A #PpgHeaderClass.
 *
 * Initializes the #PpgHeaderClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_header_class_init (PpgHeaderClass *klass)
{
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->get_property = ppg_header_get_property;
	object_class->set_property = ppg_header_set_property;
	g_type_class_add_private(object_class, sizeof(PpgHeaderPrivate));

	widget_class = GTK_WIDGET_CLASS(klass);
#if GTK_CHECK_VERSION(2, 91, 0)
	widget_class->draw = ppg_header_draw;
#else
	widget_class->expose_event = ppg_header_expose_event;
#endif

	g_object_class_install_property(object_class,
	                                PROP_BOTTOM_SEPARATOR,
	                                g_param_spec_boolean("bottom-separator",
	                                                     "bottom-separator",
	                                                     "bottom-separator",
	                                                     FALSE,
	                                                     G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_RIGHT_SEPARATOR,
	                                g_param_spec_boolean("right-separator",
	                                                     "right-separator",
	                                                     "right-separator",
	                                                     FALSE,
	                                                     G_PARAM_READWRITE));
}


/**
 * ppg_header_init:
 * @header: (in): A #PpgHeader.
 *
 * Initializes the newly created #PpgHeader instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_header_init (PpgHeader *header)
{
	header->priv = G_TYPE_INSTANCE_GET_PRIVATE(header, PPG_TYPE_HEADER,
	                                           PpgHeaderPrivate);
}

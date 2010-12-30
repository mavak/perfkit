/* ppg-ruler.c
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

#include <math.h>

#include "ppg-ruler.h"


#define ARROW_SIZE 17


G_DEFINE_TYPE(PpgRuler, ppg_ruler, PPG_TYPE_HEADER)


struct _PpgRulerPrivate
{
	PangoFontDescription *font_desc; /* Time position font styling */
	gdouble               lower;     /* Lower value of range */
	gdouble               upper;     /* Upper value of range */
	gdouble               pos;       /* Current position in range */
	gboolean              dirty;     /* If we need to redraw */
	cairo_surface_t      *ruler;     /* Ruler surface */
	cairo_surface_t      *arrow;     /* Arrow surface */
};


enum
{
	PROP_0,

	PROP_LOWER,
	PROP_UPPER,
	PROP_POSITION,
};


/**
 * ppg_ruler_contains:
 * @ruler: (in): A #PpgRuler.
 * @value: (in): A #gdouble containing the time.
 *
 * Checks to see if the rulers region contains @value.
 *
 * Returns: %TRUE if @value is within @ruler<!-- -->'s region.
 * Side effects: None.
 */
static inline gboolean
ppg_ruler_contains (PpgRuler *ruler,
                    gdouble   value)
{
	return ((value >= ruler->priv->lower) &&
	        (value <= ruler->priv->upper));
}


/**
 * ppg_ruler_update_layout_text:
 * @ruler: (in): A #PpgRuler.
 * @layout: (in): A #PangoLayout.
 * @t: (in): A #gdouble containing the time.
 *
 * Updates the text property for @layout using the time represented by @t.
 *
 * Returns: None.
 * Side effects: None.
 */
static inline void
ppg_ruler_update_layout_text (PpgRuler    *ruler,
                              PangoLayout *layout,
                              gdouble      t)
{
	gdouble fraction;
	gdouble iptr;
	gchar *markup;

	fraction = modf(t, &iptr);
	if (fraction == 0.0) {
		markup = g_strdup_printf("%02d:%02d:%02d",
		                         (gint)(t / 3600.0),
		                         (gint)(((gint)t % 3600) / 60.0),
		                         (gint)((gint)t % 60));
	} else {
		markup = g_strdup_printf("%02d:%02d:%02d.%03d",
		                         (gint)(t / 3600.0),
		                         (gint)(((gint)t % 3600) / 60.0),
		                         (gint)((gint)t % 60),
		                         (gint)(fraction * 1000.0));
	}
	pango_layout_set_text(layout, markup, -1);
	g_free(markup);
}


/**
 * ppg_ruler_get_range:
 * @ruler: (in): A #PpgRuler.
 * @lower: (out): A location for a #gdouble or %NULL.
 * @upper: (out): A location for a #gdouble or %NULL.
 * @position: (out): A location for a #gdouble or %NULL.
 *
 * Retrieves the range and position for the ruler widget.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_ruler_get_range (PpgRuler *ruler,
                     gdouble  *lower,
                     gdouble  *upper,
                     gdouble  *position)
{
	PpgRulerPrivate *priv;

	g_return_if_fail(PPG_IS_RULER(ruler));

	priv = ruler->priv;

	if (lower) {
		*lower = priv->lower;
	}
	if (upper) {
		*upper = priv->upper;
	}
	if (position) {
		*position = priv->pos;
	}
}


/**
 * ppg_ruler_set_range:
 * @ruler: (in): A #PpgRuler.
 * @lower: (in): The lower value for the range.
 * @upper: (in): The upper value for the range.
 * @position: (in): The current position in the range.
 *
 * Sets the visible range for the ruler.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_ruler_set_range (PpgRuler *ruler,
                     gdouble   lower,
                     gdouble   upper,
                     gdouble   position)
{
	PpgRulerPrivate *priv;

	g_return_if_fail(PPG_IS_RULER(ruler));

	priv = ruler->priv;

	priv->lower = lower;
	priv->upper = upper;
	priv->pos = position;
	priv->dirty = TRUE;

	gtk_widget_queue_draw(GTK_WIDGET(ruler));

	g_object_notify(G_OBJECT(ruler), "lower");
	g_object_notify(G_OBJECT(ruler), "upper");
	g_object_notify(G_OBJECT(ruler), "position");
}


/**
 * ppg_ruler_get_position:
 * @ruler: (in): A #PpgRuler.
 *
 * Retrieves the current position of the arrow in the ruler.
 *
 * Returns: The position of the arrow.
 * Side effects: None.
 */
gdouble
ppg_ruler_get_position (PpgRuler *ruler)
{
	g_return_val_if_fail(PPG_IS_RULER(ruler), 0.0);
	return ruler->priv->pos;
}


/**
 * ppg_ruler_set_position:
 * @ruler: (in): A #PpgRuler.
 * @position: (in): The position.
 *
 * Sets the position of the arrow in the ruler.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_ruler_set_position (PpgRuler *ruler,
                        gdouble   position)
{
	g_return_if_fail(PPG_IS_RULER(ruler));

	ruler->priv->pos = position;
	gtk_widget_queue_draw(GTK_WIDGET(ruler));
	g_object_notify(G_OBJECT(ruler), "position");
}


/**
 * ppg_ruler_set_lower:
 * @ruler: (in): A #PpgRuler.
 * @lower: (in): A #gdouble containing the lower bounds.
 *
 * Sets the lower range of the ruler.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_ruler_set_lower (PpgRuler *ruler,
                     gdouble   lower)
{
	PpgRulerPrivate *priv;

	g_return_if_fail(PPG_IS_RULER(ruler));

	priv = ruler->priv;
	priv->lower = lower;
	priv->dirty = TRUE;

	gtk_widget_queue_draw(GTK_WIDGET(ruler));
	g_object_notify(G_OBJECT(ruler), "lower");
}


/**
 * ppg_ruler_set_upper:
 * @ruler: (in): A #PpgRuler.
 * @upper: (in): A #gdouble containing the upper bounds.
 *
 * Sets the upper range of the ruler.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_ruler_set_upper (PpgRuler *ruler,
                     gdouble   upper)
{
	PpgRulerPrivate *priv;

	g_return_if_fail(PPG_IS_RULER(ruler));

	priv = ruler->priv;
	priv->upper = upper;
	priv->dirty = TRUE;

	gtk_widget_queue_draw(GTK_WIDGET(ruler));
	g_object_notify(G_OBJECT(ruler), "upper");
}


/**
 * ppg_ruler_motion_notify_event:
 * @ruler: (in): A #PpgRuler.
 *
 * Handle the "motion-notify-event" for the #PpgRuler. The position of the
 * arrow is updated.
 *
 * Returns: %FALSE always.
 * Side effects: None.
 */
static gboolean
ppg_ruler_motion_notify_event (GtkWidget      *widget,
                               GdkEventMotion *motion)
{
	PpgRulerPrivate *priv;
	PpgRuler *ruler = (PpgRuler *)widget;
	GtkAllocation alloc;
	gdouble pos;

	g_return_val_if_fail(PPG_IS_RULER(ruler), FALSE);

	priv = ruler->priv;

	gtk_widget_get_allocation(widget, &alloc);
	pos = priv->lower +
	      (motion->x / alloc.width * (priv->upper - priv->lower));
	ppg_ruler_set_position(ruler, pos);

	return FALSE;
}


/**
 * ppg_ruler_draw_arrow:
 * @ruler: (in): A #PpgRuler.
 *
 * Draw the arrow to a pixmap for rendering on top of the background.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_ruler_draw_arrow (PpgRuler *ruler)
{
	PpgRulerPrivate *priv;
	GtkStyle *style;
	GdkColor base_light;
	GdkColor base_dark;
	GdkColor hl_light;
	GdkColor hl_dark;
	cairo_t *cr;
	gint half;
	gint line_width;
	gint center;
	gint middle;
	gdouble top;
	gdouble bottom;
	gdouble left;
	gdouble right;

	g_return_if_fail(PPG_IS_RULER(ruler));

	priv = ruler->priv;
	style = gtk_widget_get_style(GTK_WIDGET(ruler));

	cr = cairo_create(priv->arrow);

	cairo_save(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
	cairo_rectangle(cr, 0, 0, ARROW_SIZE, ARROW_SIZE);
	cairo_fill(cr);
	cairo_restore(cr);

	center = middle = half = ARROW_SIZE / 2;
	line_width = half / 6;

	base_light = style->light[GTK_STATE_SELECTED];
	base_dark = style->dark[GTK_STATE_SELECTED];
	hl_light = style->light[GTK_STATE_SELECTED];
	hl_dark = style->mid[GTK_STATE_SELECTED];

	top = middle - half + line_width + 0.5;
	bottom = middle + half - line_width + 0.5;
	left = center - half + line_width + 0.5;
	right = center +half - line_width - 0.5;

	cairo_set_line_width(cr, line_width);

	cairo_move_to(cr, left + line_width, top + line_width);
	cairo_line_to(cr, right + line_width, top + line_width);
	cairo_line_to(cr, right + line_width, middle + line_width);
	cairo_line_to(cr, center + line_width, bottom + line_width);
	cairo_line_to(cr, left + line_width, middle + line_width);
	cairo_line_to(cr, left + line_width, top + line_width);
	cairo_close_path(cr);
	cairo_set_source_rgba(cr, 0, 0, 0, 0.5);
	cairo_fill(cr);

	cairo_move_to(cr, left, top);
	cairo_line_to(cr, center, top);
	cairo_line_to(cr, center, bottom);
	cairo_line_to(cr, left, middle);
	cairo_line_to(cr, left, top);
	cairo_close_path(cr);
	gdk_cairo_set_source_color(cr, &base_light);
	cairo_fill(cr);

	cairo_move_to(cr, center, top);
	cairo_line_to(cr, right, top);
	cairo_line_to(cr, right, middle);
	cairo_line_to(cr, center, bottom);
	cairo_line_to(cr, center, top);
	cairo_close_path(cr);
	gdk_cairo_set_source_color(cr, &base_light);
	cairo_fill_preserve(cr);
	cairo_set_source_rgba(cr,
	                      base_dark.red / 65535.0,
	                      base_dark.green / 65535.0,
	                      base_dark.blue / 65535.0,
	                      0.5);
	cairo_fill(cr);

	cairo_move_to(cr, left + line_width, top + line_width);
	cairo_line_to(cr, right - line_width, top + line_width);
	cairo_line_to(cr, right - line_width, middle);
	cairo_line_to(cr, center, bottom - line_width - 0.5);
	cairo_line_to(cr, left + line_width, middle);
	cairo_line_to(cr, left + line_width, top + line_width);
	cairo_close_path(cr);
	gdk_cairo_set_source_color(cr, &hl_light);
	cairo_stroke(cr);

	cairo_move_to(cr, left, top);
	cairo_line_to(cr, right, top);
	cairo_line_to(cr, right, middle);
	cairo_line_to(cr, center, bottom);
	cairo_line_to(cr, left, middle);
	cairo_line_to(cr, left, top);
	cairo_close_path(cr);
	gdk_cairo_set_source_color(cr, &base_dark);
	cairo_stroke(cr);

	cairo_destroy(cr);
}


/**
 * get_x_offset:
 *
 * Retrieves the x offset within the widget for a given time.
 *
 * Returns: None.
 * Side effects: None.
 */
static inline gdouble
get_x_offset (PpgRulerPrivate *priv,
              GtkAllocation *alloc,
              gdouble value)
{
	gdouble a;
	gdouble b;

	if (value < priv->lower) {
		return 0.0;
	} else if (value > priv->upper) {
		return alloc->width;
	}

	a = priv->upper - priv->lower;
	b = value - priv->lower;
	return floor((b / a) * alloc->width);
}


/**
 * ppg_ruler_draw_ruler:
 * @ruler: (in): A #PpgRuler.
 *
 * Draws the background of the ruler containing the time values and ticks
 * to an offscreen pixmap that can be blitted to the widget during
 * "expose-event".
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_ruler_draw_ruler (PpgRuler *ruler)
{
	PpgRulerPrivate *priv;
	GtkAllocation alloc;
	PangoLayout *layout;
	cairo_t *cr;
	GtkStyle *style;
	GdkColor text_color;
	gint text_width;
	gint text_height;
	gdouble every = 1.0;
	gdouble n_seconds;
	gdouble v;
	gdouble p;
	gint pw;
	gint ph;
	gint x;
	gint xx;
	gint n;
	gint z = 0;

	g_return_if_fail(PPG_IS_RULER(ruler));

	priv = ruler->priv;

	gtk_widget_get_allocation(GTK_WIDGET(ruler), &alloc);
	style = gtk_widget_get_style(GTK_WIDGET(ruler));
	cr = cairo_create(priv->ruler);

	cairo_save(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
	cairo_rectangle(cr, 0, 0, alloc.width, alloc.height);
	cairo_fill(cr);
	cairo_restore(cr);

	text_color = style->text[GTK_STATE_NORMAL];
	cairo_set_line_width(cr, 1.0);
	gdk_cairo_set_source_color(cr, &text_color);

	layout = pango_cairo_create_layout(cr);
	pango_layout_set_font_description(layout, priv->font_desc);
	pango_layout_set_markup(layout, "00:00:00.000", -1);
	pango_layout_get_pixel_size(layout, &text_width, &text_height);
	text_width += 5;

	n_seconds = priv->upper - priv->lower;
	if ((alloc.width / n_seconds) < text_width) {
		every = ceil(text_width / (alloc.width / n_seconds));
	}

	for (v = floor(priv->lower); v < priv->upper; v += every) {
		gdk_cairo_set_source_color(cr, &text_color);
		x = get_x_offset(priv, &alloc, v);
		cairo_move_to(cr, x + 0.5, alloc.height - 1.5);
		cairo_line_to(cr, x + 0.5, 0.5);

		/*
		 * Mini lines.
		 */
		for (p = v, n = 0, z = 0;
		     p < v + every;
		     p += (every / 10), n++, z++)
		{
			if (n == 0 || n == 10) {
				continue;
			}

			xx = get_x_offset(priv, &alloc, p);
			cairo_move_to(cr, xx + 0.5, alloc.height - 1.5);
			if (z % 2 == 0) {
				cairo_line_to(cr, xx + 0.5, text_height + 8.5);
			} else {
				cairo_line_to(cr, xx + 0.5, text_height + 5.5);
			}
		}

		cairo_stroke(cr);

		cairo_move_to(cr, x + 1.5, 1.5);
		ppg_ruler_update_layout_text(ruler, layout,
		                             CLAMP(v, priv->lower, priv->upper));

		/*
		 * If there is enough room to draw this layout before we get to the
		 * next layout, then draw it.
		 */
		pango_layout_get_pixel_size(layout, &pw, &ph);
		if ((x + pw) < get_x_offset(priv, &alloc, floor(v) + every)) {
			pango_cairo_show_layout(cr, layout);
		}
	}

	g_object_unref(layout);
	cairo_destroy(cr);
}


/**
 * ppg_ruler_realize:
 * @widget: (in): A #PpgRuler.
 *
 * Handle the "realize" event for the widget.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_ruler_realize (GtkWidget *widget)
{
	PpgRuler *ruler = (PpgRuler *)widget;
	PpgRulerPrivate *priv;
	GdkWindow *window;

	g_return_if_fail(PPG_IS_RULER(ruler));

	priv = ruler->priv;

	GTK_WIDGET_CLASS(ppg_ruler_parent_class)->realize(widget);
	gtk_widget_queue_resize(widget);

	if (priv->arrow) {
		cairo_surface_destroy(priv->arrow);
	}

	window = gtk_widget_get_window(widget);
	priv->arrow = gdk_window_create_similar_surface(window,
	                                                CAIRO_CONTENT_COLOR_ALPHA,
	                                                ARROW_SIZE, ARROW_SIZE);
}


/**
 * ppg_ruler_draw:
 * @ruler: (in): A #PpgRuler.
 * @cr: (in): A #cairo_t to draw to.
 *
 * Handle the "draw" event for the widget. Blit the background and position
 * arrow to the surface.
 *
 * Returns: FALSE always.
 * Side effects: None.
 */
static gboolean
ppg_ruler_draw (GtkWidget *widget,
                cairo_t   *cr)
{
	PpgRuler *ruler = (PpgRuler *)widget;
	PpgRulerPrivate *priv;
	GtkAllocation alloc;
	gboolean ret;
	gint x;
	gint y;

	g_return_val_if_fail(PPG_IS_RULER(ruler), FALSE);

#if GTK_CHECK_VERSION(2, 91, 0)
	ret = GTK_WIDGET_CLASS(ppg_ruler_parent_class)->draw(widget, cr);
#else
	ret = GTK_WIDGET_CLASS(ppg_ruler_parent_class)->
		expose_event(widget, (GdkEventExpose *)gtk_get_current_event());
#endif

	priv = ruler->priv;

	gtk_widget_get_allocation(widget, &alloc);

	/*
	 * Render the contents immediately if needed.
	 */
	if (priv->dirty) {
		ppg_ruler_draw_arrow(ruler);
		ppg_ruler_draw_ruler(ruler);
		priv->dirty = FALSE;
	}

	/*
	 * Blit the background to the surface.
	 */
	cairo_rectangle(cr, 0, 0, alloc.width, alloc.height);
	cairo_set_source_surface(cr, priv->ruler, 0, 0);
	cairo_fill(cr);

	/*
	 * Blit the arrow to the surface.
	 */
	x = (gint)(((priv->pos - priv->lower) /
                (priv->upper - priv->lower) *
                alloc.width) - (ARROW_SIZE / 2.0));
	y = alloc.height - ARROW_SIZE - 1;
	cairo_set_source_surface(cr, priv->arrow, x, y);
	cairo_rectangle(cr, x, y, ARROW_SIZE, ARROW_SIZE);
	cairo_fill(cr);

	return ret;
}


/**
 * ppg_ruler_expose_event:
 * @ruler: (in): A #PpgRuler.
 *
 * Handles the "expose-event" for older versions of Gtk+.
 *
 * Returns: %FALSE if signal emission should continue; otherwise %TRUE.
 * Side effects: None.
 */
#if !GTK_CHECK_VERSION(2, 91, 0)
static gboolean
ppg_ruler_expose_event (GtkWidget      *widget,
                        GdkEventExpose *expose)
{
	gboolean ret = FALSE;
	cairo_t *cr;

	if (gtk_widget_is_drawable(widget)) {
		cr = gdk_cairo_create(expose->window);
		gdk_cairo_rectangle(cr, &expose->area);
		cairo_clip(cr);
		ret = ppg_ruler_draw(widget, cr);
		cairo_destroy(cr);
	}

	return ret;
}
#endif


/**
 * ppg_ruler_finalize:
 * @object: (in): A #PpgRuler.
 *
 * Finalizer for a #PpgRuler instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_ruler_finalize (GObject *object)
{
	PpgRulerPrivate *priv = PPG_RULER(object)->priv;

	pango_font_description_free(priv->font_desc);
	priv->font_desc = NULL;

	if (priv->ruler) {
		cairo_surface_destroy(priv->ruler);
		priv->ruler = NULL;
	}

	if (priv->arrow) {
		cairo_surface_destroy(priv->arrow);
		priv->arrow = NULL;
	}

	G_OBJECT_CLASS(ppg_ruler_parent_class)->finalize(object);
}


/**
 * ppg_ruler_get_preferred_height:
 * @ruler: (in): A #PpgRuler.
 * @min_height: (out): A #gint.
 * @natural_height: (out): A #gint.
 *
 * Handle the "get_preferred_height" virtual function for the ruler.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_ruler_get_preferred_height (GtkWidget *widget,
                                gint      *min_height,
                                gint      *natural_height)
{
	PpgRuler *ruler = (PpgRuler *)widget;
	PpgRulerPrivate *priv;
	GdkWindow *window;
	PangoLayout *layout;
	cairo_t *cr;
	gint width;
	gint height;

	g_return_if_fail(PPG_IS_RULER(ruler));

	priv = ruler->priv;

	*min_height = *natural_height = 0;

	if ((window = gtk_widget_get_window(widget))) {
		cr = gdk_cairo_create(window);
		layout = pango_cairo_create_layout(cr);
		pango_layout_set_text(layout, "00:00:00", -1);

		pango_layout_get_pixel_size(layout, &width, &height);
		height += 12;

		g_object_unref(layout);
		cairo_destroy(cr);

		*min_height = *natural_height = height;
	}
}


/**
 * ppg_ruler_size_request:
 * @widget: (in): A #PpgRuler.
 * @req: (out): A #GtkRequisition.
 *
 * Handle the "size-request" event for the widget on older versions of Gtk+.
 *
 * Returns: None.
 * Side effects: None.
 */
#if !GTK_CHECK_VERSION(2, 91, 0)
static void
ppg_ruler_size_request (GtkWidget      *widget,
                        GtkRequisition *req)
{
	gint min_height;
	gint natural_height;

	ppg_ruler_get_preferred_height(widget, &min_height, &natural_height);
	req->height = min_height;
	req->width = 1;
}
#endif


/**
 * ppg_ruler_size_allocate:
 * @ruler: (in): A #PpgRuler.
 *
 * Handle the "size-allocate" for the #GtkWidget. The pixmap for the
 * background is created and drawn if necessary.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_ruler_size_allocate (GtkWidget     *widget,
                         GtkAllocation *alloc)
{
	PpgRuler *ruler = (PpgRuler *)widget;
	PpgRulerPrivate *priv;
	GdkWindow *window;

	g_return_if_fail(PPG_IS_RULER(ruler));

	priv = ruler->priv;

	GTK_WIDGET_CLASS(ppg_ruler_parent_class)->size_allocate(widget, alloc);

	if (priv->ruler) {
		cairo_surface_destroy(priv->ruler);
	}

	if (gtk_widget_is_drawable(widget)) {
		window = gtk_widget_get_window(widget);
		priv->ruler = gdk_window_create_similar_surface(window,
														CAIRO_CONTENT_COLOR_ALPHA,
														alloc->width,
														alloc->height);
		ppg_ruler_draw_ruler(ruler);
	}
}


/**
 * ppg_ruler_style_set:
 * @ruler: (in): A #PpgRuler.
 *
 * Handle the "style-set" event and force a redraw of the widget content.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_ruler_style_set (GtkWidget *widget,
                     GtkStyle *old_style)
{
	PpgRulerPrivate *priv;
	PpgRuler *ruler = (PpgRuler *)widget;

	g_return_if_fail(PPG_IS_RULER(ruler));

	priv = ruler->priv;

	GTK_WIDGET_CLASS(ppg_ruler_parent_class)->style_set(widget, old_style);

	priv->dirty = TRUE;
	gtk_widget_queue_draw(widget);
}


/**
 * ppg_ruler_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (out): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Get a given #GObject property.
 */
static void
ppg_ruler_get_property (GObject    *object,
                        guint       prop_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
	PpgRuler *ruler = PPG_RULER(object);

	switch (prop_id) {
	case PROP_LOWER:
		g_value_set_double(value, ruler->priv->lower);
		break;
	case PROP_UPPER:
		g_value_set_double(value, ruler->priv->upper);
		break;
	case PROP_POSITION:
		g_value_set_double(value, ruler->priv->pos);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}


/**
 * ppg_ruler_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
static void
ppg_ruler_set_property (GObject      *object,
                        guint         prop_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
	PpgRuler *ruler = PPG_RULER(object);

	switch (prop_id) {
	case PROP_LOWER:
		ppg_ruler_set_lower(ruler, g_value_get_double(value));
		break;
	case PROP_UPPER:
		ppg_ruler_set_upper(ruler, g_value_get_double(value));
		break;
	case PROP_POSITION:
		ppg_ruler_set_position(ruler, g_value_get_double(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}


/**
 * ppg_ruler_class_init:
 * @klass: (in): A #PpgRulerClass.
 *
 * Initializes the #PpgRulerClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_ruler_class_init (PpgRulerClass *klass)
{
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_ruler_finalize;
	object_class->get_property = ppg_ruler_get_property;
	object_class->set_property = ppg_ruler_set_property;
	g_type_class_add_private(object_class, sizeof(PpgRulerPrivate));

	widget_class = GTK_WIDGET_CLASS(klass);
#if GTK_CHECK_VERSION(2, 91, 0)
	widget_class->draw = ppg_ruler_draw;
	widget_class->get_preferred_height = ppg_ruler_get_preferred_height;
#else
	widget_class->expose_event = ppg_ruler_expose_event;
	widget_class->size_request = ppg_ruler_size_request;
#endif
	widget_class->motion_notify_event = ppg_ruler_motion_notify_event;
	widget_class->realize = ppg_ruler_realize;
	widget_class->size_allocate = ppg_ruler_size_allocate;
	widget_class->style_set = ppg_ruler_style_set;

	g_object_class_install_property(object_class,
	                                PROP_LOWER,
	                                g_param_spec_double("lower",
	                                                    "lower",
	                                                    "lower",
	                                                    0,
	                                                    G_MAXDOUBLE,
	                                                    0,
	                                                    G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_UPPER,
	                                g_param_spec_double("upper",
	                                                    "upper",
	                                                    "upper",
	                                                    0,
	                                                    G_MAXDOUBLE,
	                                                    0,
	                                                    G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_POSITION,
	                                g_param_spec_double("position",
	                                                    "position",
	                                                    "position",
	                                                    0,
	                                                    G_MAXDOUBLE,
	                                                    0,
	                                                    G_PARAM_READWRITE));
}


/**
 * ppg_ruler_init:
 * @ruler: (in): A #PpgRuler.
 *
 * Initializes the newly created #PpgRuler instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_ruler_init (PpgRuler *ruler)
{
	PpgRulerPrivate *priv;

	priv = ruler->priv =
		G_TYPE_INSTANCE_GET_PRIVATE(ruler, PPG_TYPE_RULER, PpgRulerPrivate);

	gtk_widget_add_events(GTK_WIDGET(ruler), GDK_POINTER_MOTION_MASK);

	priv->font_desc = pango_font_description_new();
	pango_font_description_set_family_static(priv->font_desc, "Monospace");
	pango_font_description_set_size(priv->font_desc, 8 * PANGO_SCALE);
}

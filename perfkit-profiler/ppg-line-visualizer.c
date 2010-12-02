/* ppg-line-visualizer.c
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

#include "ppg-color.h"
#include "ppg-line-visualizer.h"

G_DEFINE_TYPE(PpgLineVisualizer, ppg_line_visualizer, PPG_TYPE_VISUALIZER)

typedef struct
{
	gint      key;
	PpgModel *model;
	gchar    *title;
	GdkColor  color;
	gboolean  fill;
} Line;

struct _PpgLineVisualizerPrivate
{
	GArray       *lines;
	ClutterActor *actor;
	guint         paint_handler;
	guint         resize_handler;

	gboolean      range_set;
	gdouble       range_lower;
	gdouble       range_upper;

	gdouble       last_time; /* time of last sample */
};

void
ppg_line_visualizer_set_range (PpgLineVisualizer *visualizer,
                               gdouble            lower,
                               gdouble            upper)
{
	PpgLineVisualizerPrivate *priv;

	g_return_if_fail(PPG_IS_LINE_VISUALIZER(visualizer));

	priv = visualizer->priv;

	priv->range_lower = lower;
	priv->range_upper = upper;
	priv->range_set = TRUE;
}

static void
model_changed_cb (PpgModel          *model,
                  PpgLineVisualizer *line)
{
	PpgVisualizer *visualizer = (PpgVisualizer *)line;
	PpgLineVisualizerPrivate *priv = line->priv;
	gdouble last;

	last = ppg_model_get_last_time(model);
	ppg_visualizer_queue_draw_fast(visualizer, priv->last_time, last);
	priv->last_time = last;
}

void
ppg_line_visualizer_append (PpgLineVisualizer *visualizer,
                            const gchar       *name,
                            GdkColor          *color,
                            gboolean           fill,
                            PpgModel          *model,
                            gint               key)
{
	PpgLineVisualizerPrivate *priv;
	Line line = { 0 };

	g_return_if_fail(PPG_IS_LINE_VISUALIZER(visualizer));

	priv = visualizer->priv;

	if (color) {
		line.color = *color;
	} else {
		gdk_color_parse("#000", &line.color);
	}

	line.title = g_strdup(name);
	line.model = model;
	line.key = key;
	line.fill = fill;

	g_array_append_val(priv->lines, line);

	g_signal_connect(model, "changed", G_CALLBACK(model_changed_cb),
	                 visualizer);
}

static ClutterActor*
ppg_line_visualizer_get_actor (PpgVisualizer *visualizer)
{
	g_return_val_if_fail(PPG_IS_LINE_VISUALIZER(visualizer), NULL);
	return PPG_LINE_VISUALIZER(visualizer)->priv->actor;
}

static void
ppg_line_visualizer_resize_surface (PpgLineVisualizer *visualizer)
{
	PpgLineVisualizerPrivate *priv;
	gfloat width;
	gfloat height;

	g_return_if_fail(PPG_IS_LINE_VISUALIZER(visualizer));

	priv = visualizer->priv;

	g_object_get(priv->actor,
	             "height", &height,
	             "width", &width,
	             NULL);

	g_object_set(priv->actor,
	             "surface-height", (gint)height,
	             "surface-width", (gint)width,
	             NULL);

	ppg_visualizer_queue_draw(PPG_VISUALIZER(visualizer));
}

static gboolean
ppg_line_visualizer_resize_timeout (gpointer user_data)
{
	PpgLineVisualizer *visualizer = (PpgLineVisualizer *)user_data;

	g_return_val_if_fail(PPG_IS_LINE_VISUALIZER(visualizer), FALSE);

	visualizer->priv->resize_handler = 0;
	ppg_line_visualizer_resize_surface(visualizer);

	return FALSE;
}

static void
ppg_line_visualizer_queue_resize (PpgLineVisualizer *visualizer)
{
	PpgLineVisualizerPrivate *priv;

	g_return_if_fail(PPG_IS_LINE_VISUALIZER(visualizer));

	priv = visualizer->priv;

	if (!priv->resize_handler) {
		priv->resize_handler =
			g_timeout_add(0, ppg_line_visualizer_resize_timeout, visualizer);
	}
}

static void
ppg_line_visualizer_notify_allocation (ClutterActor *actor,
                                       GParamSpec *pspec,
                                       PpgLineVisualizer *visualizer)
{
	ppg_line_visualizer_queue_resize(visualizer);
}

static inline gdouble
get_x_offset (gdouble begin,
              gdouble end,
              gdouble width,
              gdouble value)
{
	return (value - begin) / (end - begin) * width;
}

static inline gdouble
get_y_offset (gdouble lower,
              gdouble upper,
              gdouble height,
              gdouble value)
{
	return height - ((value - lower) / (upper - lower) * height);
}

static gdouble
get_value (PpgModel     *model,
           PpgModelIter *iter,
           gint          key)
{
	GValue value = { 0 };

	ppg_model_get_value(model, iter, key, &value);
	switch (value.g_type) {
	case G_TYPE_DOUBLE:
		return g_value_get_double(&value);
	case G_TYPE_FLOAT:
		return g_value_get_float(&value);
	case G_TYPE_INT:
		return g_value_get_int(&value);
	case G_TYPE_UINT:
		return g_value_get_uint(&value);
	case G_TYPE_LONG:
		return g_value_get_long(&value);
	case G_TYPE_ULONG:
		return g_value_get_ulong(&value);
	default:
		g_critical("Uknown value type: %s", g_type_name(value.g_type));
		g_assert_not_reached();
	}

	return 0.0;
}

static void
ppg_line_visualizer_draw_fast (PpgVisualizer *visualizer,
                               gdouble        begin,
                               gdouble        end)
{
	PpgLineVisualizerPrivate *priv;
	PpgModelIter iter;
	Line *line;
	PpgColorIter color;
	cairo_t *cr;
	gfloat height;
	gfloat width;
	gdouble x;
	gdouble y;
	gdouble last_x = 0;
	gdouble last_y = 0;
	gdouble real_begin;
	gdouble real_end;
	gdouble lower = 0;
	gdouble upper = 0;
	gdouble tl;
	gdouble tu;
	gdouble val = 0;
	gint i;

	g_return_if_fail(PPG_IS_LINE_VISUALIZER(visualizer));

	priv = PPG_LINE_VISUALIZER(visualizer)->priv;

#if 0
	clutter_cairo_texture_clear(CLUTTER_CAIRO_TEXTURE(priv->actor));
#endif

	/*
	 * Get the bounds we need to draw.
	 */
	g_object_get(visualizer,
	             "begin", &real_begin,
	             "end", &real_end,
	             NULL);
	g_object_get(priv->actor,
	             "width", &width,
	             "height", &height,
	             NULL);

	if (end < real_begin || begin > real_end) {
		return;
	}

	cr = clutter_cairo_texture_create(CLUTTER_CAIRO_TEXTURE(priv->actor));

	/*
	 * Clear the range we are drawing.
	 */
	cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
	cairo_rectangle(cr,
	                get_x_offset(real_begin, real_end, width, begin), 0,
	                get_x_offset(real_begin, real_end, width, end), height);
	cairo_clip_preserve(cr);
	cairo_fill(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

#if 0
	cairo_rectangle(cr, 0, 0, width, height);
	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_fill(cr);
#endif

	/*
	 * Create the smallest range containing all lines.
	 */
	if (priv->range_set) {
		lower = priv->range_lower;
		upper = priv->range_upper;
	} else {
		for (i = 0; i < priv->lines->len; i++) {
			line = &g_array_index(priv->lines, Line, i);
			ppg_model_get_range(line->model, line->key, &tl, &tu);
			lower = MIN(lower, tl);
			upper = MAX(upper, tu);
		}
	}

	ppg_color_iter_init(&color);

	cairo_set_line_width(cr, 1.0);

	for (i = 0; i < priv->lines->len; i++) {
		line = &g_array_index(priv->lines, Line, i);
		cairo_move_to(cr, 0, height);
		gdk_cairo_set_source_color(cr, &color.color);

		if (!ppg_model_get_iter_at(line->model, &iter, begin, end, PPG_RESOLUTION_FULL)) {
			goto next;
		}

		last_x = get_x_offset(real_begin, real_end, width, iter.time);
		last_y = get_y_offset(lower, upper, height,
		                      get_value(line->model, &iter, line->key));
		cairo_move_to(cr, last_x, last_y);

		/*
		 * Can't do anything with only a single data point.
		 */
		if (!ppg_model_iter_next(line->model, &iter)) {
			goto next;
		}

		do {
			val = get_value(line->model, &iter, line->key);
			x = get_x_offset(real_begin, real_end, width, iter.time);
			y = get_y_offset(lower, upper, height, val);
#if 0
			cairo_line_to(cr, x, y);
#else
			cairo_curve_to(cr,
			               last_x + ((x - last_x) / 2.0),
			               last_y,
			               last_x + ((x - last_x) / 2.0),
			               y, x, y);
#endif
			last_x = x;
			last_y = y;
		} while (ppg_model_iter_next(line->model, &iter));

		cairo_stroke(cr);

	  next:
		ppg_color_iter_next(&color);
	}

	cairo_destroy(cr);
}

static void
ppg_line_visualizer_draw (PpgVisualizer *visualizer)
{
	PpgLineVisualizer *line = (PpgLineVisualizer *)visualizer;
	PpgLineVisualizerPrivate *priv;
	gdouble begin;
	gdouble end;

	g_return_if_fail(PPG_IS_LINE_VISUALIZER(line));

	priv = line->priv;

	g_object_get(visualizer,
	             "begin", &begin,
	             "end", &end,
	             NULL);

	clutter_cairo_texture_clear(CLUTTER_CAIRO_TEXTURE(priv->actor));
	ppg_line_visualizer_draw_fast(visualizer, begin, end);
}

/**
 * ppg_line_visualizer_finalize:
 * @object: (in): A #PpgLineVisualizer.
 *
 * Finalizer for a #PpgLineVisualizer instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_line_visualizer_finalize (GObject *object)
{
	PpgLineVisualizerPrivate *priv = PPG_LINE_VISUALIZER(object)->priv;

	g_array_unref(priv->lines);

	G_OBJECT_CLASS(ppg_line_visualizer_parent_class)->finalize(object);
}

/**
 * ppg_line_visualizer_class_init:
 * @klass: (in): A #PpgLineVisualizerClass.
 *
 * Initializes the #PpgLineVisualizerClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_line_visualizer_class_init (PpgLineVisualizerClass *klass)
{
	GObjectClass *object_class;
	PpgVisualizerClass *visualizer_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_line_visualizer_finalize;
	g_type_class_add_private(object_class, sizeof(PpgLineVisualizerPrivate));

	visualizer_class = PPG_VISUALIZER_CLASS(klass);
	visualizer_class->get_actor = ppg_line_visualizer_get_actor;
	visualizer_class->draw = ppg_line_visualizer_draw;
	visualizer_class->draw_fast = ppg_line_visualizer_draw_fast;
}

/**
 * ppg_line_visualizer_init:
 * @visualizer: (in): A #PpgLineVisualizer.
 *
 * Initializes the newly created #PpgLineVisualizer instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_line_visualizer_init (PpgLineVisualizer *visualizer)
{
	PpgLineVisualizerPrivate *priv;

	priv = G_TYPE_INSTANCE_GET_PRIVATE(visualizer, PPG_TYPE_LINE_VISUALIZER,
	                                   PpgLineVisualizerPrivate);
	visualizer->priv = priv;

	priv->lines = g_array_new(FALSE, FALSE, sizeof(Line));

	priv->actor = g_object_new(CLUTTER_TYPE_CAIRO_TEXTURE,
	                           "surface-width", 1,
	                           "surface-height", 1,
	                           "natural-height", 45.0f,
	                           NULL);

	g_signal_connect_after(priv->actor, "notify::allocation",
	                       G_CALLBACK(ppg_line_visualizer_notify_allocation),
	                       visualizer);
}

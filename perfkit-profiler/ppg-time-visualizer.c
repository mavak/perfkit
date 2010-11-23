/* ppg-time-visualizer.c
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
#include "ppg-log.h"
#include "ppg-time-visualizer.h"

G_DEFINE_TYPE(PpgTimeVisualizer, ppg_time_visualizer, PPG_TYPE_VISUALIZER)

struct _PpgTimeVisualizerPrivate
{
	ClutterActor *actor;
	PpgModel     *model;
	guint         paint_handler;
	guint         resize_handler;
	gdouble       last_time; /* time of last sample */
};

static void
model_changed_cb (PpgModel          *model,
                  PpgTimeVisualizer *time_visualizer)
{
	PpgVisualizer *visualizer = (PpgVisualizer *)time_visualizer;
	PpgTimeVisualizerPrivate *priv = time_visualizer->priv;
	gdouble last;

	last = ppg_model_get_last_time(model);
	ppg_visualizer_queue_draw_fast(visualizer, priv->last_time, last);
	priv->last_time = last;
}

void
ppg_time_visualizer_set_model (PpgTimeVisualizer *visualizer,
                               PpgModel          *model)
{
	PpgTimeVisualizerPrivate *priv;

	g_return_if_fail(PPG_IS_TIME_VISUALIZER(visualizer));

	priv = visualizer->priv;
	priv->model = model;
	g_signal_connect(model, "changed", G_CALLBACK(model_changed_cb),
	                 visualizer);
}

static ClutterActor*
ppg_time_visualizer_get_actor (PpgVisualizer *visualizer)
{
	g_return_val_if_fail(PPG_IS_TIME_VISUALIZER(visualizer), NULL);

	return PPG_TIME_VISUALIZER(visualizer)->priv->actor;
}

static void
ppg_time_visualizer_resize_surface (PpgTimeVisualizer *visualizer)
{
	PpgTimeVisualizerPrivate *priv;
	gfloat width;
	gfloat height;

	ENTRY;

	g_return_if_fail(PPG_IS_TIME_VISUALIZER(visualizer));

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

	EXIT;
}

static gboolean
ppg_time_visualizer_resize_timeout (gpointer user_data)
{
	PpgTimeVisualizer *visualizer = (PpgTimeVisualizer *)user_data;

	ENTRY;

	g_return_val_if_fail(PPG_IS_TIME_VISUALIZER(visualizer), FALSE);

	visualizer->priv->resize_handler = 0;
	ppg_time_visualizer_resize_surface(visualizer);
	RETURN(FALSE);
}

static void
ppg_time_visualizer_queue_resize (PpgTimeVisualizer *visualizer)
{
	PpgTimeVisualizerPrivate *priv;

	g_return_if_fail(PPG_IS_TIME_VISUALIZER(visualizer));

	priv = visualizer->priv;

	if (!priv->resize_handler) {
		priv->resize_handler =
			g_timeout_add(0, ppg_time_visualizer_resize_timeout, visualizer);
	}
}

static void
ppg_time_visualizer_notify_allocation (ClutterActor      *actor,
                                       GParamSpec        *pspec,
                                       PpgTimeVisualizer *visualizer)
{
	ENTRY;
	ppg_time_visualizer_queue_resize(visualizer);
	EXIT;
}

static inline gdouble
get_x_offset (gdouble begin,
              gdouble end,
              gdouble width,
              gdouble value)
{
	return (value - begin) / (end - begin) * width;
}

static void
ppg_time_visualizer_draw_fast (PpgVisualizer *visualizer,
                               gdouble        begin,
                               gdouble        end)
{
	PpgTimeVisualizerPrivate *priv;
	PpgModelIter iter;
	cairo_t *cr;
	gdouble real_begin;
	gdouble real_end;
	gdouble x;
	gfloat height;
	gfloat width;

	ENTRY;

	g_return_if_fail(PPG_IS_TIME_VISUALIZER(visualizer));

	priv = PPG_TIME_VISUALIZER(visualizer)->priv;

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
#if 0
	cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
	cairo_rectangle(cr,
	                get_x_offset(real_begin, real_end, width, begin), 0,
	                get_x_offset(real_begin, real_end, width, end), height);
	cairo_clip_preserve(cr);
	cairo_fill(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
#endif

	/*
	 * Prepare the line format.
	 */
	cairo_set_line_width(cr, 1.0);
	cairo_set_source_rgb(cr, 0, 0, 0);

	/*
	 * Draw the lines.
	 */
	if (ppg_model_get_iter_at(priv->model, &iter, begin, end, PPG_RESOLUTION_FULL)) {
		do {
			x = get_x_offset(real_begin, real_end, width, iter.time);
			cairo_move_to(cr, x, 0);
			cairo_line_to(cr, x, height);
			cairo_stroke(cr);
		} while (ppg_model_iter_next(priv->model, &iter));
	}

	cairo_destroy(cr);

	EXIT;
}

static void
ppg_time_visualizer_draw (PpgVisualizer *visualizer)
{
	PpgTimeVisualizer *_time= (PpgTimeVisualizer *)visualizer;
	PpgTimeVisualizerPrivate *priv;
	gdouble begin;
	gdouble end;

	g_return_if_fail(PPG_IS_TIME_VISUALIZER(_time));

	priv = _time->priv;

	g_object_get(visualizer,
	             "begin", &begin,
	             "end", &end,
	             NULL);

	clutter_cairo_texture_clear(CLUTTER_CAIRO_TEXTURE(priv->actor));
	ppg_time_visualizer_draw_fast(visualizer, begin, end);
}

/**
 * ppg_time_visualizer_finalize:
 * @object: (in): A #PpgTimeVisualizer.
 *
 * Finalizer for a #PpgTimeVisualizer instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_time_visualizer_finalize (GObject *object)
{
	G_OBJECT_CLASS(ppg_time_visualizer_parent_class)->finalize(object);
}

/**
 * ppg_time_visualizer_class_init:
 * @klass: (in): A #PpgTimeVisualizerClass.
 *
 * Initializes the #PpgTimeVisualizerClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_time_visualizer_class_init (PpgTimeVisualizerClass *klass)
{
	GObjectClass *object_class;
	PpgVisualizerClass *visualizer_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_time_visualizer_finalize;
	g_type_class_add_private(object_class, sizeof(PpgTimeVisualizerPrivate));

	visualizer_class = PPG_VISUALIZER_CLASS(klass);
	visualizer_class->get_actor = ppg_time_visualizer_get_actor;
	visualizer_class->draw = ppg_time_visualizer_draw;
	visualizer_class->draw_fast = ppg_time_visualizer_draw_fast;
}

/**
 * ppg_time_visualizer_init:
 * @visualizer: (in): A #PpgTimeVisualizer.
 *
 * Initializes the newly created #PpgTimeVisualizer instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_time_visualizer_init (PpgTimeVisualizer *visualizer)
{
	PpgTimeVisualizerPrivate *priv;

	priv = G_TYPE_INSTANCE_GET_PRIVATE(visualizer, PPG_TYPE_TIME_VISUALIZER,
	                                   PpgTimeVisualizerPrivate);
	visualizer->priv = priv;

	priv->actor = g_object_new(CLUTTER_TYPE_CAIRO_TEXTURE,
	                           "surface-width", 1,
	                           "surface-height", 1,
	                           "natural-height", 45.0f,
	                           NULL);

	g_signal_connect_after(priv->actor, "notify::allocation",
	                       G_CALLBACK(ppg_time_visualizer_notify_allocation),
	                       visualizer);
}

/* ppg-time-span-visualizer.c
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
#include "ppg-model.h"
#include "ppg-time-span-visualizer.h"

G_DEFINE_TYPE(PpgTimeSpanVisualizer, ppg_time_span_visualizer,
              PPG_TYPE_VISUALIZER)

struct _PpgTimeSpanVisualizerPrivate
{
	ClutterActor         *actor;
	PangoFontDescription *font_desc;
	PpgModel             *model;
	guint                 paint_handler;
	guint                 resize_handler;
	gdouble               last_time; /* time of last sample */
	gint                  begin_key;
	gint                  end_key;
};

enum
{
	PROP_0,
	PROP_BEGIN,
	PROP_END,
	PROP_MODEL,
	PROP_SPAN_BEGIN_KEY,
	PROP_SPAN_END_KEY,
};

static void
model_changed_cb (PpgModel          *model,
                  PpgTimeSpanVisualizer *time_span_visualizer)
{
	PpgVisualizer *visualizer = (PpgVisualizer *)time_span_visualizer;
	PpgTimeSpanVisualizerPrivate *priv = time_span_visualizer->priv;
	gdouble last;

	last = ppg_model_get_last_time(model);
	ppg_visualizer_queue_draw_fast(visualizer, priv->last_time, last);
	priv->last_time = last;
}

void
ppg_time_span_visualizer_set_model (PpgTimeSpanVisualizer *visualizer,
                                    PpgModel          *model)
{
	PpgTimeSpanVisualizerPrivate *priv;

	g_return_if_fail(PPG_IS_TIME_SPAN_VISUALIZER(visualizer));

	priv = visualizer->priv;
	priv->model = model;
	g_signal_connect(model, "changed", G_CALLBACK(model_changed_cb),
	                 visualizer);
}

static ClutterActor*
ppg_time_span_visualizer_get_actor (PpgVisualizer *visualizer)
{
	g_return_val_if_fail(PPG_IS_TIME_SPAN_VISUALIZER(visualizer), NULL);

	return PPG_TIME_SPAN_VISUALIZER(visualizer)->priv->actor;
}

static void
ppg_time_span_visualizer_resize_surface (PpgTimeSpanVisualizer *visualizer)
{
	PpgTimeSpanVisualizerPrivate *priv;
	gfloat width;
	gfloat height;

	ENTRY;

	g_return_if_fail(PPG_IS_TIME_SPAN_VISUALIZER(visualizer));

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
ppg_time_span_visualizer_resize_timeout (gpointer user_data)
{
	PpgTimeSpanVisualizer *visualizer = (PpgTimeSpanVisualizer *)user_data;

	ENTRY;

	g_return_val_if_fail(PPG_IS_TIME_SPAN_VISUALIZER(visualizer), FALSE);

	visualizer->priv->resize_handler = 0;
	ppg_time_span_visualizer_resize_surface(visualizer);
	RETURN(FALSE);
}

static void
ppg_time_span_visualizer_queue_resize (PpgTimeSpanVisualizer *visualizer)
{
	PpgTimeSpanVisualizerPrivate *priv;

	g_return_if_fail(PPG_IS_TIME_SPAN_VISUALIZER(visualizer));

	priv = visualizer->priv;

	if (!priv->resize_handler) {
		priv->resize_handler =
			g_timeout_add(0, ppg_time_span_visualizer_resize_timeout, visualizer);
	}
}

static void
ppg_time_span_visualizer_notify_allocation (ClutterActor      *actor,
                                       GParamSpec        *pspec,
                                       PpgTimeSpanVisualizer *visualizer)
{
	ENTRY;
	ppg_time_span_visualizer_queue_resize(visualizer);
	EXIT;
}

static inline gdouble
get_x_offset (gdouble begin,
              gdouble end,
              gdouble width,
              gdouble value)
{
	value = (value - begin) / (end - begin) * width;
	return CLAMP(value, 0.0, width);
}

static void
ppg_time_span_visualizer_draw_fast (PpgVisualizer *visualizer,
                                    gdouble        begin,
                                    gdouble        end)
{
	PpgTimeSpanVisualizerPrivate *priv;
	PpgModelIter iter;
	PangoLayout *layout;
	cairo_t *cr;
	cairo_pattern_t *p;
	gdouble real_begin;
	gdouble real_end;
	gdouble x1;
	gdouble x2;
	gfloat height;
	gfloat width;
	GValue value = { 0 };
	gchar text[32];
	gdouble diff;
	gint w;
	gint h;

	ENTRY;

	g_return_if_fail(PPG_IS_TIME_SPAN_VISUALIZER(visualizer));

	priv = PPG_TIME_SPAN_VISUALIZER(visualizer)->priv;

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
	layout = pango_cairo_create_layout(cr);
	pango_layout_set_font_description(layout, priv->font_desc);
	p = cairo_pattern_create_linear(0, 0, 0, height);
	cairo_pattern_add_color_stop_rgba(p, 0, 1, 1, 1, 0.4);
	cairo_pattern_add_color_stop_rgba(p, .61803, 1, 1, 1, 0.0);

	/*
	 * Clear the range we are drawing.
	 */
#if 1
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

	/*
	 * Draw the lines.
	 */
	if (ppg_model_get_iter_at(priv->model, &iter, begin, end, PPG_RESOLUTION_FULL)) {
		do {
			/*
			 * Get values.
			 */
			if (!ppg_model_get_value(priv->model, &iter, priv->begin_key, &value)) {
				g_value_unset(&value);
				continue;
			}
			x1 = g_value_get_double(&value);
			g_value_unset(&value);

			if (!ppg_model_get_value(priv->model, &iter, priv->end_key, &value)) {
				g_value_unset(&value);
				continue;
			}
			x2 = g_value_get_double(&value);
			g_value_unset(&value);

			/*
			 * Real time difference in msec.
			 */
			diff = (x2 - x1) * 1000.0;

			/*
			 * Get X offsets.
			 */
			x1 = get_x_offset(real_begin, real_end, width, x1);
			x2 = get_x_offset(real_begin, real_end, width, x2);

			/*
			 * Draw the block.
			 */
			cairo_rectangle(cr, x1, 0, x2 - x1, height);
			cairo_set_source_rgb(cr, 0.9607, 0.4745, 0);
			cairo_fill(cr);

			/*
			 * Draw the time span text if we can. Don't even try if we don't
			 * have at least 10 pixels.
			 */
			if ((x2 - x1) > 10.0) {
				g_snprintf(text, sizeof text, "%.0f msec", diff);
				pango_layout_set_text(layout, text, -1);
				pango_layout_get_pixel_size(layout, &w, &h);
				if ((x2 - x1) >= w) {
					cairo_set_source_rgb(cr, 0, 0, 0);
					cairo_move_to(cr, x1, height - h);
					pango_cairo_show_layout(cr, layout);
				}
			}

			/*
			 * Draw highlight.
			 */
			cairo_set_source(cr, p);
			cairo_rectangle(cr, x1, 0, x2 - x1, height);
			cairo_fill(cr);
		} while (ppg_model_iter_next(priv->model, &iter));
	}

	g_object_unref(layout);
	cairo_pattern_destroy(p);
	cairo_destroy(cr);

	EXIT;
}

static void
ppg_time_span_visualizer_draw (PpgVisualizer *visualizer)
{
	PpgTimeSpanVisualizer *_time= (PpgTimeSpanVisualizer *)visualizer;
	PpgTimeSpanVisualizerPrivate *priv;
	gdouble begin;
	gdouble end;

	g_return_if_fail(PPG_IS_TIME_SPAN_VISUALIZER(_time));

	priv = _time->priv;

	g_object_get(visualizer,
	             "begin", &begin,
	             "end", &end,
	             NULL);

	clutter_cairo_texture_clear(CLUTTER_CAIRO_TEXTURE(priv->actor));
	ppg_time_span_visualizer_draw_fast(visualizer, begin, end);
}

/**
 * ppg_time_span_visualizer_finalize:
 * @object: (in): A #PpgTimeSpanVisualizer.
 *
 * Finalizer for a #PpgTimeSpanVisualizer instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_time_span_visualizer_finalize (GObject *object)
{
	PpgTimeSpanVisualizerPrivate *priv = PPG_TIME_SPAN_VISUALIZER(object)->priv;

	pango_font_description_free(priv->font_desc);

	G_OBJECT_CLASS(ppg_time_span_visualizer_parent_class)->finalize(object);
}

/**
 * ppg_time_span_visualizer_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
static void
ppg_time_span_visualizer_set_property (GObject      *object,
                                       guint         prop_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
	PpgTimeSpanVisualizer *visualizer = PPG_TIME_SPAN_VISUALIZER(object);

	switch (prop_id) {
	case PROP_SPAN_BEGIN_KEY:
		visualizer->priv->begin_key = g_value_get_int(value);
		break;
	case PROP_SPAN_END_KEY:
		visualizer->priv->end_key = g_value_get_int(value);
		break;
	case PROP_MODEL:
		ppg_time_span_visualizer_set_model(visualizer, g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * ppg_time_span_visualizer_class_init:
 * @klass: (in): A #PpgTimeSpanVisualizerClass.
 *
 * Initializes the #PpgTimeSpanVisualizerClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_time_span_visualizer_class_init (PpgTimeSpanVisualizerClass *klass)
{
	GObjectClass *object_class;
	PpgVisualizerClass *visualizer_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_time_span_visualizer_finalize;
	object_class->set_property = ppg_time_span_visualizer_set_property;
	g_type_class_add_private(object_class, sizeof(PpgTimeSpanVisualizerPrivate));

	visualizer_class = PPG_VISUALIZER_CLASS(klass);
	visualizer_class->get_actor = ppg_time_span_visualizer_get_actor;
	visualizer_class->draw = ppg_time_span_visualizer_draw;
	visualizer_class->draw_fast = ppg_time_span_visualizer_draw_fast;

	g_object_class_install_property(object_class,
	                                PROP_MODEL,
	                                g_param_spec_object("model",
	                                                    "model",
	                                                    "model",
	                                                    PPG_TYPE_MODEL,
	                                                    G_PARAM_WRITABLE));

	g_object_class_install_property(object_class,
	                                PROP_SPAN_BEGIN_KEY,
	                                g_param_spec_int("span-begin-key",
	                                                 "span-begin-key",
	                                                 "span-begin-key",
	                                                 0,
	                                                 G_MAXINT,
	                                                 0,
	                                                 G_PARAM_WRITABLE));

	g_object_class_install_property(object_class,
	                                PROP_SPAN_END_KEY,
	                                g_param_spec_int("span-end-key",
	                                                 "span-end-key",
	                                                 "span-end-key",
	                                                 0,
	                                                 G_MAXINT,
	                                                 0,
	                                                 G_PARAM_WRITABLE));
}

/**
 * ppg_time_span_visualizer_init:
 * @visualizer: (in): A #PpgTimeSpanVisualizer.
 *
 * Initializes the newly created #PpgTimeSpanVisualizer instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_time_span_visualizer_init (PpgTimeSpanVisualizer *visualizer)
{
	PpgTimeSpanVisualizerPrivate *priv;

	priv = G_TYPE_INSTANCE_GET_PRIVATE(visualizer, PPG_TYPE_TIME_SPAN_VISUALIZER,
	                                   PpgTimeSpanVisualizerPrivate);
	visualizer->priv = priv;

	priv->actor = g_object_new(CLUTTER_TYPE_CAIRO_TEXTURE,
	                           "surface-width", 1,
	                           "surface-height", 1,
	                           "natural-height", 45.0f,
	                           NULL);

	priv->font_desc = pango_font_description_new();
	pango_font_description_set_family_static(priv->font_desc, "Monospace");
	pango_font_description_set_size(priv->font_desc, PANGO_SCALE * 8);

	g_signal_connect_after(priv->actor, "notify::allocation",
	                       G_CALLBACK(ppg_time_span_visualizer_notify_allocation),
	                       visualizer);
}

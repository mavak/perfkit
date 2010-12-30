/* ppg-visualizer-simple.c
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

#include "ppg-renderer.h"
#include "ppg-util.h"
#include "ppg-visualizer-simple.h"


G_DEFINE_TYPE(PpgVisualizerSimple, ppg_visualizer_simple, PPG_TYPE_VISUALIZER)


struct _PpgVisualizerSimplePrivate
{
	PpgRenderer *renderer;
	guint        invalidate_handler;
};


enum
{
	PROP_0,
	PROP_RENDERER,
};


/**
 * ppg_visualizer_simple_invalidate:
 * @simple: (in): A #PpgVisualizerSimple.
 *
 * Handles an "invalidate" request from a renderer. The requested area
 * is invalidated in the visualizer forcing a redraw.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_visualizer_simple_invalidate (PpgVisualizerSimple *simple,
                                  gdouble              begin_time,
                                  gdouble              end_time,
                                  PpgRenderer         *renderer)
{
	PpgVisualizer *visualizer = (PpgVisualizer *)simple;
	ppg_visualizer_queue_draw_time_span(visualizer, begin_time, end_time);
}


/**
 * ppg_visualizer_simple_set_renderer:
 * @simple: (in): A #PpgVisualizerSimple.
 * @renderer: (in): A #PpgRenderer.
 *
 * Sets the renderer for the visualizer.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_visualizer_simple_set_renderer (PpgVisualizerSimple *simple,
                                    PpgRenderer         *renderer)
{
	g_return_if_fail(PPG_IS_VISUALIZER_SIMPLE(simple));
	simple->priv->renderer = g_object_ref_sink(renderer);
	simple->priv->invalidate_handler =
		g_signal_connect_swapped(renderer, "invalidate",
		                         G_CALLBACK(ppg_visualizer_simple_invalidate),
		                         simple);
}


/**
 * ppg_visualizer_simple_draw:
 * @simple: (in): A #PpgVisualizerSimple.
 *
 * Handle a draw for the visualizer using our configured renderer.
 *
 * Returns: None.
 * Side effects: None.
 */
static PpgTask*
ppg_visualizer_simple_draw (PpgVisualizer   *visualizer,
                            cairo_surface_t *surface,
                            gdouble          begin_time,
                            gdouble          end_time,
                            gdouble          x,
                            gdouble          y,
                            gdouble          width,
                            gdouble          height)
{
	PpgVisualizerSimple *simple = (PpgVisualizerSimple *)visualizer;

	g_return_val_if_fail(PPG_IS_VISUALIZER_SIMPLE(simple), NULL);
	g_return_val_if_fail(simple->priv->renderer, NULL);

	return ppg_renderer_draw(simple->priv->renderer, surface,
	                         begin_time, end_time,
	                         x, y, width, height);
}


/**
 * ppg_visualizer_simple_dispose:
 * @object: (in): A #GObject.
 *
 * Dispose callback for @object.  This method releases references held
 * by the #GObject instance.
 *
 * Returns: None.
 * Side effects: Plenty.
 */
static void
ppg_visualizer_simple_dispose (GObject *object)
{
	PpgVisualizerSimplePrivate *priv = PPG_VISUALIZER_SIMPLE(object)->priv;

	ppg_clear_object(&priv->renderer);
	ppg_clear_source(&priv->invalidate_handler);

	G_OBJECT_CLASS(ppg_visualizer_simple_parent_class)->dispose(object);
}


/**
 * ppg_visualizer_simple_finalize:
 * @object: (in): A #PpgVisualizerSimple.
 *
 * Finalizer for a #PpgVisualizerSimple instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_visualizer_simple_finalize (GObject *object)
{
	G_OBJECT_CLASS(ppg_visualizer_simple_parent_class)->finalize(object);
}


/**
 * ppg_visualizer_simple_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
static void
ppg_visualizer_simple_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
	PpgVisualizerSimple *simple = PPG_VISUALIZER_SIMPLE(object);

	switch (prop_id) {
	case PROP_RENDERER:
		ppg_visualizer_simple_set_renderer(simple, g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}


/**
 * ppg_visualizer_simple_class_init:
 * @klass: (in): A #PpgVisualizerSimpleClass.
 *
 * Initializes the #PpgVisualizerSimpleClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_visualizer_simple_class_init (PpgVisualizerSimpleClass *klass)
{
	GObjectClass *object_class;
	PpgVisualizerClass *visualizer_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->dispose = ppg_visualizer_simple_dispose;
	object_class->finalize = ppg_visualizer_simple_finalize;
	object_class->set_property = ppg_visualizer_simple_set_property;
	g_type_class_add_private(object_class, sizeof(PpgVisualizerSimplePrivate));

	visualizer_class = PPG_VISUALIZER_CLASS(klass);
	visualizer_class->draw = ppg_visualizer_simple_draw;

	g_object_class_install_property(object_class,
	                                PROP_RENDERER,
	                                g_param_spec_object("renderer",
	                                                    "renderer",
	                                                    "renderer",
	                                                    PPG_TYPE_RENDERER,
	                                                    G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}


/**
 * ppg_visualizer_simple_init:
 * @simple: (in): A #PpgVisualizerSimple.
 *
 * Initializes the newly created #PpgVisualizerSimple instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_visualizer_simple_init (PpgVisualizerSimple *simple)
{
	simple->priv = G_TYPE_INSTANCE_GET_PRIVATE(simple,
	                                           PPG_TYPE_VISUALIZER_SIMPLE,
	                                           PpgVisualizerSimplePrivate);
}

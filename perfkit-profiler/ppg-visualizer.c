/* ppg-visualizer.c
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

#include "ppg-log.h"
#include "ppg-visualizer.h"

G_DEFINE_ABSTRACT_TYPE(PpgVisualizer, ppg_visualizer, G_TYPE_INITIALLY_UNOWNED)

struct _PpgVisualizerPrivate
{
	gchar   *title;
	gchar   *name;
	gdouble  begin;
	gdouble  end;
	guint    draw_handler;
};

enum
{
	PROP_0,
	PROP_BEGIN,
	PROP_END,
	PROP_NAME,
	PROP_TITLE,
};

ClutterActor*
ppg_visualizer_get_actor (PpgVisualizer *visualizer)
{
	return PPG_VISUALIZER_GET_CLASS(visualizer)->get_actor(visualizer);
}

static gboolean
ppg_visualizer_draw_timeout (gpointer data)
{
	PpgVisualizer *visualizer = (PpgVisualizer *)data;
	PpgVisualizerPrivate *priv;
	PpgVisualizerClass *klass;

	g_return_val_if_fail(PPG_IS_VISUALIZER(visualizer), FALSE);

	priv = visualizer->priv;
	klass = PPG_VISUALIZER_GET_CLASS(visualizer);

	priv->draw_handler = 0;

	if (klass->draw) {
		klass->draw(visualizer);
	}

	return FALSE;
}

void
ppg_visualizer_queue_draw (PpgVisualizer *visualizer)
{
	PpgVisualizerPrivate *priv;

	g_return_if_fail(PPG_IS_VISUALIZER(visualizer));

	priv = visualizer->priv;

	if (!priv->draw_handler) {
		priv->draw_handler =
			g_timeout_add(0, ppg_visualizer_draw_timeout, visualizer);
	}
}

static void
ppg_visualizer_set_begin (PpgVisualizer *visualizer,
                          gdouble        begin)
{
	g_return_if_fail(PPG_IS_VISUALIZER(visualizer));

	ENTRY;
	visualizer->priv->begin = begin;
	ppg_visualizer_queue_draw(visualizer);
	EXIT;
}

static void
ppg_visualizer_set_end (PpgVisualizer *visualizer,
                        gdouble        end)
{
	g_return_if_fail(PPG_IS_VISUALIZER(visualizer));

	ENTRY;
	visualizer->priv->end = end;
	ppg_visualizer_queue_draw(visualizer);
	EXIT;
}

static void
ppg_visualizer_finalize (GObject *object)
{
	PpgVisualizer *visualizer = PPG_VISUALIZER(object);
	PpgVisualizerPrivate *priv = visualizer->priv;

	ENTRY;

	g_free(priv->title);

	G_OBJECT_CLASS(ppg_visualizer_parent_class)->dispose(object);

	EXIT;
}

/**
 * ppg_visualizer_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (out): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Get a given #GObject property.
 */
static void
ppg_visualizer_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
	PpgVisualizer *visualizer = PPG_VISUALIZER(object);

	switch (prop_id) {
	case PROP_BEGIN:
		g_value_set_double(value, visualizer->priv->begin);
		break;
	case PROP_END:
		g_value_set_double(value, visualizer->priv->end);
		break;
	case PROP_NAME:
		g_value_set_string(value, visualizer->priv->name);
		break;
	case PROP_TITLE:
		g_value_set_string(value, visualizer->priv->title);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * ppg_visualizer_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
static void
ppg_visualizer_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
	PpgVisualizer *visualizer = PPG_VISUALIZER(object);

	switch (prop_id) {
	case PROP_BEGIN:
		ppg_visualizer_set_begin(visualizer, g_value_get_double(value));
		break;
	case PROP_END:
		ppg_visualizer_set_end(visualizer, g_value_get_double(value));
		break;
	case PROP_NAME:
		/* construct only */
		visualizer->priv->name = g_value_dup_string(value);
		break;
	case PROP_TITLE:
		/* construct only */
		visualizer->priv->title = g_value_dup_string(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * ppg_visualizer_class_init:
 * @klass: (in): A #PpgVisualizerClass.
 *
 * Initializes the #PpgVisualizerClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_visualizer_class_init (PpgVisualizerClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_visualizer_finalize;
	object_class->get_property = ppg_visualizer_get_property;
	object_class->set_property = ppg_visualizer_set_property;
	g_type_class_add_private(object_class, sizeof(PpgVisualizerPrivate));

	g_object_class_install_property(object_class,
	                                PROP_TITLE,
	                                g_param_spec_string("title",
	                                                    "title",
	                                                    "title",
	                                                    NULL,
	                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property(object_class,
	                                PROP_NAME,
	                                g_param_spec_string("name",
	                                                    "name",
	                                                    "name",
	                                                    NULL,
	                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property(object_class,
	                                PROP_BEGIN,
	                                g_param_spec_double("begin",
	                                                    "begin",
	                                                    "begin",
	                                                    0,
	                                                    G_MAXDOUBLE,
	                                                    0,
	                                                    G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_END,
	                                g_param_spec_double("end",
	                                                    "end",
	                                                    "end",
	                                                    0,
	                                                    G_MAXDOUBLE,
	                                                    0,
	                                                    G_PARAM_READWRITE));
}

/**
 * ppg_visualizer_init:
 * @visualizer: (in): A #PpgVisualizer.
 *
 * Initializes the newly created #PpgVisualizer instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_visualizer_init (PpgVisualizer *visualizer)
{
	PpgVisualizerPrivate *priv;

	priv = G_TYPE_INSTANCE_GET_PRIVATE(visualizer, PPG_TYPE_VISUALIZER,
	                                   PpgVisualizerPrivate);
	visualizer->priv = priv;
}

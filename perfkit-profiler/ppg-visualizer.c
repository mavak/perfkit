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
#include "ppg-prefs.h"
#include "ppg-task-render.h"
#include "ppg-util.h"
#include "ppg-visualizer.h"


G_DEFINE_ABSTRACT_TYPE(PpgVisualizer, ppg_visualizer, GOO_TYPE_CANVAS_IMAGE)


struct _PpgVisualizerPrivate
{
	cairo_surface_t *surface;
	gchar           *name;
	gchar           *title;
	PpgTask         *task;
	gdouble          begin_time;
	gdouble          end_time;
	gdouble          draw_begin_time;
	gdouble          draw_end_time;
	gdouble          natural_height;
	gboolean         frozen;
	gboolean         important;
	guint            frame_limit;
	guint            draw_handler;
	guint            resize_handler;
};


enum
{
	PROP_0,

	PROP_BEGIN_TIME,
	PROP_END_TIME,
	PROP_FRAME_LIMIT,
	PROP_IS_IMPORTANT,
	PROP_NAME,
	PROP_NATURAL_HEIGHT,
	PROP_TITLE,
};


/**
 * ppg_visualizer_task_notify_state:
 * @visualizer: (in): A #PpgVisualizer.
 * @pspec: (in): A #GParamSpec.
 * @task: (in): A #PpgTask.
 *
 * Handle the "notify::state" signal from @task. Update the visualizer
 * pattern if necessary.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_visualizer_task_notify_state (PpgVisualizer *visualizer,
                                  GParamSpec    *pspec,
                                  PpgTask       *task)
{
	PpgVisualizerPrivate *priv;
	cairo_surface_t *surface;
	PpgTaskState state;
	cairo_t *cr;
	gdouble begin_time;
	gdouble height;
	gdouble total_width;
	gdouble span;
	gdouble width;
	gdouble x;
	gdouble y;

	g_return_if_fail(PPG_IS_VISUALIZER(visualizer));
	g_return_if_fail(PPG_IS_TASK(task));
	g_return_if_fail(visualizer->priv->surface);

	priv = visualizer->priv;

	/*
	 * We don't own the reference, so safe to just drop our pointer. Using
	 * GObjects weak pointers here would be a lot of maintenance pain.
	 */
	if (priv->task == task) {
		priv->task = NULL;
	}

	g_object_get(task,
	             "state", &state,
	             "surface", &surface,
	             NULL);

	if (state == PPG_TASK_SUCCESS) {
		g_object_get(task,
		             "begin-time", &begin_time,
		             "height", &height,
		             "width", &width,
		             "x", &x,
		             "y", &y,
		             NULL);

		span = priv->end_time - priv->begin_time;
		g_object_get(visualizer, "width", &total_width, NULL);
		x = (begin_time - priv->begin_time) / span * total_width;

		cr = cairo_create(priv->surface);
		cairo_set_source_surface(cr, surface, x, y);
		if (cairo_status(cr) != 0) {
			cairo_destroy(cr);
			GOTO(failure);
		}

		/*
		 * Clip the range of the draw.
		 */
		cairo_rectangle(cr, x, y, width, height);
		cairo_clip_preserve(cr);

		/*
		 * Clear the draw area.
		 */
		cairo_save(cr);
		cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
		cairo_fill_preserve(cr);
		cairo_restore(cr);

		/*
		 * Fill in the rendered image.
		 */
		cairo_fill(cr);
		cairo_destroy(cr);

		goo_canvas_item_request_update(GOO_CANVAS_ITEM(visualizer));
	}

  failure:
	/*
	 * Release our surface that was allocated for the draw request.
	 */
	if ((state & PPG_TASK_FINISHED_MASK) != 0) {
		cairo_surface_destroy(surface);
	}
}


/**
 * ppg_visualizer_draw_timeout:
 * @visualizer: (in): A #PpgVisualizer.
 *
 * A GSourceFunc to start a new drawing task.
 *
 * Returns: %FALSE always.
 * Side effects: None.
 */
static gboolean
ppg_visualizer_draw_timeout (gpointer data)
{
	PpgVisualizer *visualizer = (PpgVisualizer *)data;
	PpgVisualizerPrivate *priv;
	cairo_surface_t *surface = NULL;
	gpointer instance;
	GooCanvas *canvas;
	GdkWindow *window;
	gdouble begin_time;
	gdouble end_time;
	gdouble height;
	gdouble span;
	gdouble width;
	gdouble x;

	g_return_val_if_fail(PPG_IS_VISUALIZER(visualizer), FALSE);

	priv = visualizer->priv;

	/*
	 * Cancel any active tasks. The task will lose its reference after
	 * the task scheduler completes; so we can just NULL it out.
	 */
	if ((instance = priv->task)) {
		priv->task = NULL;
		ppg_task_cancel(instance);
	}

	/*
	 * Make sure we have a time range to even render.
	 */
	if (priv->begin_time == 0.0 && priv->end_time == 0.0) {
		goto cleanup;
	}

	/*
	 * Get the time range for the render.
	 */
	begin_time = CLAMP(priv->draw_begin_time, 0.0, priv->end_time);
	end_time = CLAMP(priv->draw_end_time, 0.0, priv->end_time);
	if (begin_time == 0.0 && end_time == 0.0) {
		begin_time = priv->begin_time;
		end_time = priv->end_time;
	}

	/*
	 * Get the area for the render. This could probably be optimized to
	 * remove the division by keeping a ratio of (time range / width).
	 */
	g_object_get(visualizer, "height", &height, "width", &width, NULL);
	span = priv->end_time - priv->begin_time;
	x = (begin_time - priv->begin_time) / span * width;
	width = ((end_time - priv->begin_time) / span * width) - x;

	/*
	 * Create a surface for the rendering.
	 */
	canvas = goo_canvas_item_get_canvas(GOO_CANVAS_ITEM(visualizer));
	window = gtk_widget_get_window(GTK_WIDGET(canvas));
	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
	                                     width, height);

	/*
	 * Create the task to do the rendering.
	 */
	priv->task = PPG_VISUALIZER_GET_CLASS(visualizer)->
		draw(visualizer,
	         cairo_surface_reference(surface),
	         begin_time, end_time,
	         0, 0, width, height);
	g_signal_connect_swapped(priv->task, "notify::state",
	                         G_CALLBACK(ppg_visualizer_task_notify_state),
	                         visualizer);
	ppg_task_schedule(priv->task);

  cleanup:
	if (surface) {
		cairo_surface_destroy(surface);
	}
	priv->draw_handler = 0;
	priv->draw_begin_time = 0.0;
	priv->draw_end_time = 0.0;

	return FALSE;
}


/**
 * ppg_visualizer_queue_draw_time_span:
 * @visualizer: (in): A #PpgVisualizer.
 * @begin_time: (in): A #gdouble contianing the beggining time.
 * @end_time: (in): A #gdouble contianing the ending time.
 *
 * Queues a draw for a particular time span. If @begin_time and #end_time
 * are 0.0, then the entire visibile area will be drawn.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_visualizer_queue_draw_time_span (PpgVisualizer *visualizer,
                                     gdouble        begin_time,
                                     gdouble        end_time,
                                     gboolean       now)
{
	PpgVisualizerPrivate *priv;
	guint msec;

	g_return_if_fail(PPG_IS_VISUALIZER(visualizer));
	g_return_if_fail(begin_time >= 0.0);
	g_return_if_fail(end_time >= 0.0);

	priv = visualizer->priv;

	if (begin_time == 0.0) {
		begin_time = priv->begin_time;
	}

	if (end_time == 0.0) {
		end_time = priv->end_time;
	}

	if (!priv->frozen) {
		if (now) {
			priv->draw_begin_time = begin_time;
			priv->draw_end_time = end_time;
			ppg_visualizer_draw_timeout(visualizer);
		} else if (!priv->draw_handler) {
			msec = 1000 / priv->frame_limit;
			priv->draw_begin_time = begin_time;
			priv->draw_end_time = end_time;
			priv->draw_handler = g_timeout_add(msec,
			                                   ppg_visualizer_draw_timeout,
			                                   visualizer);
		} else {
			priv->draw_begin_time = MIN(priv->draw_begin_time, begin_time);
			priv->draw_end_time = MAX(priv->draw_end_time, end_time);
		}
	}
}


/**
 * ppg_visualizer_queue_draw:
 * @visualizer: (in): A #PpgVisualizer.
 *
 * Queues a draw request for the entire visible area of the visualizer.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_visualizer_queue_draw (PpgVisualizer *visualizer)
{
	g_return_if_fail(PPG_IS_VISUALIZER(visualizer));
	ppg_visualizer_queue_draw_time_span(visualizer, 0.0, 0.0, TRUE);
}


/**
 * ppg_visualizer_resize_timeout:
 * @visualizer: (in): A #PpgVisualizer.
 *
 * A GSourceFunc to handle a resize request. The surface of the visualizer
 * is resized and a draw request is queued.
 *
 * Returns: %FALSE always
 * Side effects: None.
 */
static gboolean
ppg_visualizer_resize_timeout (gpointer data)
{
	PpgVisualizer *visualizer = (PpgVisualizer *)data;
	PpgVisualizerPrivate *priv;
	cairo_pattern_t *pattern;
	GdkWindow *window;
	GooCanvas *canvas;
	gdouble width;
	gdouble height;

	g_return_val_if_fail(PPG_IS_VISUALIZER(visualizer), FALSE);

	priv = visualizer->priv;

	/*
	 * Remove existing surface.
	 */
	if (priv->surface) {
		g_object_set(visualizer, "pattern", NULL, NULL);
		cairo_surface_destroy(priv->surface);
		priv->surface = NULL;
	}

	/*
	 * Create new surface matching new size allocation.
	 */
	g_object_get(visualizer, "height", &height, "width", &width, NULL);
	canvas = goo_canvas_item_get_canvas(GOO_CANVAS_ITEM(visualizer));
	window = gtk_widget_get_window(GTK_WIDGET(canvas));
	priv->surface = gdk_window_create_similar_surface(window,
	                                                  CAIRO_CONTENT_COLOR_ALPHA,
	                                                  width, height);

	/*
	 * Create a new pattern for drawing the item.
	 */
	pattern = cairo_pattern_create_for_surface(priv->surface);
	g_object_set(visualizer, "pattern", pattern, NULL);
	cairo_pattern_destroy(pattern);
	ppg_visualizer_queue_draw(visualizer);

	priv->resize_handler = 0;
	return FALSE;
}


/**
 * ppg_visualizer_queue_resize:
 * @visualizer: (in): A #PpgVisualizer.
 *
 * Queues a request to resize the pattern the visualizer uses to render.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_visualizer_queue_resize (PpgVisualizer *visualizer)
{
	PpgVisualizerPrivate *priv;

	g_return_if_fail(PPG_IS_VISUALIZER(visualizer));

	priv = visualizer->priv;

	if (!priv->resize_handler) {
		priv->resize_handler = g_timeout_add(0, ppg_visualizer_resize_timeout,
		                                     visualizer);
	}
}


/**
 * ppg_visualizer_set_begin_time:
 * @visualizer: (in): A #PpgVisualizer.
 * @begin_time: (in): A #gdouble containing the new begin time.
 *
 * Sets the begin_time for the visualizer. A new draw request is queued.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_visualizer_set_begin_time (PpgVisualizer *visualizer,
                               gdouble        begin_time)
{
	g_return_if_fail(PPG_IS_VISUALIZER(visualizer));
	g_return_if_fail(begin_time >= 0.0);

	visualizer->priv->begin_time = begin_time;
	ppg_visualizer_queue_draw(visualizer);
}


/**
 * ppg_visualizer_set_end_time:
 * @visualizer: (in): A #PpgVisualizer.
 * @end_time: (in): A #gdouble containing the new end time.
 *
 * Sets the end_time for the visualizer. A new draw request is queued.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_visualizer_set_end_time (PpgVisualizer *visualizer,
                             gdouble        end_time)
{
	g_return_if_fail(PPG_IS_VISUALIZER(visualizer));
	g_return_if_fail(end_time >= 0.0);

	visualizer->priv->end_time = end_time;
	ppg_visualizer_queue_draw(visualizer);
}


/**
 * ppg_visualizer_set_is_important:
 * @visualizer: (in): A #PpgVisualizer.
 *
 * Sets if this visualizer is important. Important visualizers can be shown
 * when an instrument is collaposed into its tiny view.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_visualizer_set_is_important (PpgVisualizer *visualizer,
                                 gboolean       important)
{
	g_return_if_fail(PPG_IS_VISUALIZER(visualizer));

	visualizer->priv->important = important;
	g_object_notify(G_OBJECT(visualizer), "is-important");
}


/**
 * ppg_visualizer_set_time:
 * @visualizer: (in): A #PpgVisualizer.
 * @begin_time: (in): A #gdouble containing the new begin time.
 * @end_time: (in): A #gdouble containing the new end time.
 *
 * Sets both the begin_time and end_time for the visualizer. A new draw
 * request is queued.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_visualizer_set_time (PpgVisualizer *visualizer,
                         gdouble        begin_time,
                         gdouble        end_time)
{
	PpgVisualizerPrivate *priv;

	g_return_if_fail(PPG_IS_VISUALIZER(visualizer));
	g_return_if_fail(begin_time >= 0.0);
	g_return_if_fail(end_time >= begin_time);

	priv = visualizer->priv;

	if ((priv->begin_time != begin_time) || (priv->end_time != end_time)) {
		priv->begin_time = begin_time;
		priv->end_time = end_time;
		ppg_visualizer_queue_draw(visualizer);
	}
}


/**
 * ppg_visualizer_get_natural_height:
 * @visualizer: (in): A #PpgVisualizer.
 *
 * Gets the natural height of the visualizer. This is used to calculate
 * the height needed within the instrument view.
 *
 * Returns: A #gdouble.
 * Side effects: None.
 */
gdouble
ppg_visualizer_get_natural_height (PpgVisualizer *visualizer)
{
	g_return_val_if_fail(PPG_IS_VISUALIZER(visualizer), 0.0);
	return visualizer->priv->natural_height;
}


/**
 * ppg_visualizer_get_is_important:
 * @visualizer: (in): A #PpgVisualizer.
 *
 * Retrieves if this visualizer is important. See
 * ppg_visualizer_set_is_important() for more information.
 *
 * Returns: %TRUE if the visualizer is important.
 * Side effects: None.
 */
gboolean
ppg_visualizer_get_is_important (PpgVisualizer *visualizer)
{
	g_return_val_if_fail(PPG_IS_VISUALIZER(visualizer), FALSE);
	return visualizer->priv->important;
}


/**
 * ppg_visualizer_set_natural_height:
 * @visualizer: (in): A #PpgVisualizer.
 * @natural_height: (in): The natural height of the visualizer.
 *
 * Sets the natural height for the visualizer. This is what most
 * implementations will want to modify if they need to adjust their
 * height based on the content. The zoom level in the instrument
 * will be multiplied against this value to get the effective
 * height for the visualizer.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_visualizer_set_natural_height (PpgVisualizer *visualizer,
                                   gdouble        natural_height)
{
	PpgVisualizerPrivate *priv;

	g_return_if_fail(PPG_IS_VISUALIZER(visualizer));

	priv = visualizer->priv;

	priv->natural_height = natural_height;
	g_object_notify(G_OBJECT(visualizer), "natural-height");
}


/**
 * ppg_visualizer_set_name:
 * @visualizer: (in): A #PpgVisualizer.
 * @name: (in): A string containing the visualizer name.
 *
 * Sets the name of the visualizer. This is mostly for internal purposes.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_visualizer_set_name (PpgVisualizer *visualizer,
                         const gchar   *name)
{
	g_return_if_fail(PPG_IS_VISUALIZER(visualizer));
	g_return_if_fail(visualizer->priv->name == NULL);

	visualizer->priv->name = g_strdup(name);
	g_object_notify(G_OBJECT(visualizer), "name");
}


/**
 * ppg_visualizer_set_title:
 * @visualizer: (in): A #PpgVisualizer.
 * @title: (in): A string containing the visualizer title.
 *
 * Sets the title for the visualizer as viewd by the user.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_visualizer_set_title (PpgVisualizer *visualizer,
                          const gchar   *title)
{
	g_return_if_fail(PPG_IS_VISUALIZER(visualizer));
	g_return_if_fail(visualizer->priv->title == NULL);

	visualizer->priv->title = g_strdup(title);
	g_object_notify(G_OBJECT(visualizer), "title");
}


/**
 * ppg_visualizer_freeze:
 * @visualizer: (in): A #PpgVisualizer.
 *
 * Freezes the visualizer preventing it from drawing updates. Drawing will
 * continue when ppg_visualizer_thaw() is called.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_visualizer_freeze (PpgVisualizer *visualizer)
{
	PpgVisualizerPrivate *priv;

	g_return_if_fail(PPG_IS_VISUALIZER(visualizer));

	priv = visualizer->priv;

	priv->frozen = TRUE;
	if (priv->draw_handler) {
		g_source_remove(priv->draw_handler);
		priv->draw_handler = 0;
		priv->draw_begin_time = 0.0;
		priv->draw_end_time = 0.0;
	}
}


/**
 * ppg_visualizer_thaw:
 * @visualizer: (in): A #PpgVisualizer.
 *
 * Thaws a call to ppg_visualizer_freeze() and queues a new draw request of
 * the visualizer.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_visualizer_thaw (PpgVisualizer *visualizer)
{
	PpgVisualizerPrivate *priv;

	g_return_if_fail(PPG_IS_VISUALIZER(visualizer));

	priv = visualizer->priv;

	priv->frozen = FALSE;
	ppg_visualizer_queue_draw(visualizer);
}


/**
 * ppg_visualizer_get_name:
 * @visualizer: (in): A #PpgVisualizer.
 *
 * Retrieves the name of a visualizer.
 *
 * Returns: None.
 * Side effects: None.
 */
const gchar*
ppg_visualizer_get_name (PpgVisualizer *visualizer)
{
	g_return_val_if_fail(PPG_IS_VISUALIZER(visualizer), NULL);
	return visualizer->priv->name;
}


/**
 * ppg_visualizer_finalize:
 * @object: (in): A #PpgVisualizer.
 *
 * Finalizer for a #PpgVisualizer instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_visualizer_finalize (GObject *object)
{
	PpgVisualizerPrivate *priv = PPG_VISUALIZER(object)->priv;

	if (priv->surface) {
		cairo_surface_destroy(priv->surface);
		priv->surface = NULL;
	}

	g_free(priv->title);
	g_free(priv->name);

	G_OBJECT_CLASS(ppg_visualizer_parent_class)->finalize(object);
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
	case PROP_BEGIN_TIME:
		g_value_set_double(value, visualizer->priv->begin_time);
		break;
	case PROP_END_TIME:
		g_value_set_double(value, visualizer->priv->end_time);
		break;
	case PROP_IS_IMPORTANT:
		g_value_set_boolean(value, visualizer->priv->important);
		break;
	case PROP_NAME:
		g_value_set_string(value, ppg_visualizer_get_name(visualizer));
		break;
	case PROP_NATURAL_HEIGHT:
		g_value_set_double(value, visualizer->priv->natural_height);
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
	case PROP_BEGIN_TIME:
		ppg_visualizer_set_begin_time(visualizer, g_value_get_double(value));
		break;
	case PROP_END_TIME:
		ppg_visualizer_set_end_time(visualizer, g_value_get_double(value));
		break;
	case PROP_FRAME_LIMIT:
		visualizer->priv->frame_limit = g_value_get_int(value);
		break;
	case PROP_IS_IMPORTANT:
		ppg_visualizer_set_is_important(visualizer, g_value_get_boolean(value));
		break;
	case PROP_NAME:
		ppg_visualizer_set_name(visualizer, g_value_get_string(value));
		break;
	case PROP_NATURAL_HEIGHT:
		ppg_visualizer_set_natural_height(visualizer, g_value_get_double(value));
		break;
	case PROP_TITLE:
		ppg_visualizer_set_title(visualizer, g_value_get_string(value));
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
	                                PROP_BEGIN_TIME,
	                                g_param_spec_double("begin-time",
	                                                    "begin-time",
	                                                    "begin-time",
	                                                    0.0,
	                                                    G_MAXDOUBLE,
	                                                    0.0,
	                                                    G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_END_TIME,
	                                g_param_spec_double("end-time",
	                                                    "end-time",
	                                                    "end-time",
	                                                    0.0,
	                                                    G_MAXDOUBLE,
	                                                    0.0,
	                                                    G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_IS_IMPORTANT,
	                                g_param_spec_boolean("is-important",
	                                                     "is-important",
	                                                     "is-important",
	                                                     FALSE,
	                                                     G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_NAME,
	                                g_param_spec_string("name",
	                                                    "name",
	                                                    "name",
	                                                    NULL,
	                                                    G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_NATURAL_HEIGHT,
	                                g_param_spec_double("natural-height",
	                                                    "natural-height",
	                                                    "natural-height",
	                                                    0,
	                                                    G_MAXDOUBLE,
	                                                    25.0,
	                                                    G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_TITLE,
	                                g_param_spec_string("title",
	                                                    "title",
	                                                    "title",
	                                                    NULL,
	                                                    G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_FRAME_LIMIT,
	                                g_param_spec_int("frame-limit",
	                                                 "frame-limit",
	                                                 "frame-limit",
	                                                 1,
	                                                 G_MAXINT,
	                                                 2,
	                                                 G_PARAM_WRITABLE));
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
	GSettings *settings;

	visualizer->priv = G_TYPE_INSTANCE_GET_PRIVATE(visualizer,
	                                               PPG_TYPE_VISUALIZER,
	                                               PpgVisualizerPrivate);

	visualizer->priv->natural_height = 25.0;

	settings = ppg_prefs_get_window_settings();
	g_settings_bind(settings, "redraws-per-second",
	                visualizer, "frame-limit",
	                G_SETTINGS_BIND_GET);

	g_signal_connect(visualizer, "notify::width",
	                 G_CALLBACK(ppg_visualizer_queue_resize), NULL);
	g_signal_connect(visualizer, "notify::height",
	                 G_CALLBACK(ppg_visualizer_queue_resize), NULL);
}

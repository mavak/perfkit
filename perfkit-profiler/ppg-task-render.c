/* ppg-task-render.c
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

#include <gtk/gtk.h>

#include "ppg-log.h"
#include "ppg-task-render.h"
#include "ppg-util.h"


G_DEFINE_TYPE(PpgTaskRender, ppg_task_render, PPG_TYPE_TASK)


/*
 * XXX: Until pixman is fixed, the max threads we can do here is really
 *      just the Xorg thread. Otherwise craziness happens. Ugh.
 */
#define MAX_THREADS 1


struct _PpgTaskRenderPrivate
{
	gint sequence;
	cairo_surface_t *surface;
	gdouble begin_time;
	gdouble end_time;
	gdouble x;
	gdouble y;
	gdouble width;
	gdouble height;
};


enum
{
	PROP_0,

	PROP_BEGIN_TIME,
	PROP_END_TIME,
	PROP_HEIGHT,
	PROP_SURFACE,
	PROP_SEQUENCE,
	PROP_WIDTH,
	PROP_X,
	PROP_Y,
};


enum
{
	RENDER,
	LAST_SIGNAL
};


static GAsyncQueue *task_queue = NULL;
#if MAX_THREADS > 1
static GThread *task_threads[MAX_THREADS] = { NULL };
#endif


/*
 * Globals.
 */
static guint signals[LAST_SIGNAL] = { 0 };
static guint render_handler = 0;


/**
 * ppg_task_render_run:
 * @task: (in): A #PpgTaskRender.
 *
 * Render the task by allowing attached signals to process the request.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_task_render_run (PpgTask *task)
{
	g_signal_emit(task, signals[RENDER], 0);
	ppg_task_finish(task);
}


/**
 * ppg_task_render_compare:
 *
 * Compare func for comparing two tasks. This simply pushes the newest
 * task to the beginning of the queue.
 *
 * Returns: A qsort() style return value.
 * Side effects: None.
 */
static gint
ppg_task_render_compare (gconstpointer a,
                         gconstpointer b,
                         gpointer      user_data)
{
	PpgTaskRender *taska = (PpgTaskRender *)a;
	PpgTaskRender *taskb = (PpgTaskRender *)b;

	return taskb->priv->sequence - taska->priv->sequence;
}


#if MAX_THREADS == 1
static gboolean
ppg_task_render_timeout (gpointer data)
{
	PpgTask *task;

	while ((task = g_async_queue_try_pop(task_queue))) {
		ppg_task_run(task);
		g_object_unref(task);
	}
	render_handler = 0;
	return FALSE;
}
#endif


/**
 * ppg_task_render_schedule:
 * @task: (in): A #PpgTaskRender.
 *
 * Schedules a render task using our private task queue.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_task_render_schedule (PpgTask *task)
{
	g_async_queue_push_sorted(task_queue, g_object_ref_sink(task),
	                          ppg_task_render_compare,
	                          NULL);
#if MAX_THREADS == 1
	if (!render_handler) {
		g_timeout_add(0, ppg_task_render_timeout, NULL);
	}
#endif
}


/**
 * ppg_task_render_thread:
 * @data: (in): A #GAsyncQueue.
 *
 * Thread function to process tasks from the task queue.
 *
 * Returns: %NULL.
 * Side effects: None.
 */
#if MAX_THREADS > 1
static gpointer
ppg_task_render_thread (gpointer data)
{
	GAsyncQueue *queue = (GAsyncQueue *)data;
	PpgTask *task;

	while ((task = g_async_queue_pop(queue))) {
		ppg_task_run(task);
		g_object_unref(task);
	}
	return NULL;
}
#endif


/**
 * ppg_task_render_finalize:
 * @object: (in): A #PpgTaskRender.
 *
 * Finalizer for a #PpgTaskRender instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_task_render_finalize (GObject *object)
{
	ENTRY;

	/*
	 * Don't free surface, we do not own its reference. See issues
	 * in comments about pixman.
	 */

	G_OBJECT_CLASS(ppg_task_render_parent_class)->finalize(object);

	EXIT;
}


/**
 * ppg_task_render_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (out): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Get a given #GObject property.
 */
static void
ppg_task_render_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
	PpgTaskRender *render = PPG_TASK_RENDER(object);

	switch (prop_id) {
	case PROP_X:
		g_value_set_double(value, render->priv->x);
		break;
	case PROP_Y:
		g_value_set_double(value, render->priv->y);
		break;
	case PROP_WIDTH:
		g_value_set_double(value, render->priv->width);
		break;
	case PROP_HEIGHT:
		g_value_set_double(value, render->priv->height);
		break;
	case PROP_SEQUENCE:
		g_value_set_int(value, render->priv->sequence);
		break;
	case PROP_SURFACE:
		g_value_set_pointer(value, render->priv->surface);
		break;
	case PROP_BEGIN_TIME:
		g_value_set_double(value, render->priv->begin_time);
		break;
	case PROP_END_TIME:
		g_value_set_double(value, render->priv->end_time);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}


/**
 * ppg_task_render_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
static void
ppg_task_render_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
	PpgTaskRender *render = PPG_TASK_RENDER(object);

	switch (prop_id) {
	case PROP_X:
		render->priv->x = g_value_get_double(value);
		break;
	case PROP_Y:
		render->priv->y = g_value_get_double(value);
		break;
	case PROP_WIDTH:
		render->priv->width = g_value_get_double(value);
		break;
	case PROP_HEIGHT:
		render->priv->height = g_value_get_double(value);
		break;
	case PROP_SURFACE:
		render->priv->surface = g_value_get_pointer(value);
		break;
	case PROP_BEGIN_TIME:
		render->priv->begin_time = g_value_get_double(value);
		break;
	case PROP_END_TIME:
		render->priv->end_time = g_value_get_double(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}


/**
 * ppg_task_render_class_init:
 * @klass: (in): A #PpgTaskRenderClass.
 *
 * Initializes the #PpgTaskRenderClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_task_render_class_init (PpgTaskRenderClass *klass)
{
	GObjectClass *object_class;
	PpgTaskClass *task_class;
#if MAX_THREADS > 1
	guint n_threads;
	gint i;
#endif

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_task_render_finalize;
	object_class->get_property = ppg_task_render_get_property;
	object_class->set_property = ppg_task_render_set_property;
	g_type_class_add_private(object_class, sizeof(PpgTaskRenderPrivate));

	task_class = PPG_TASK_CLASS(klass);
	task_class->run = ppg_task_render_run;
	task_class->schedule = ppg_task_render_schedule;

	g_object_class_install_property(object_class,
	                                PROP_BEGIN_TIME,
	                                g_param_spec_double("begin-time",
	                                                    "begin-time",
	                                                    "begin-time",
	                                                    0,
	                                                    G_MAXDOUBLE,
	                                                    0,
	                                                    G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_END_TIME,
	                                g_param_spec_double("end-time",
	                                                    "end-time",
	                                                    "end-time",
	                                                    0,
	                                                    G_MAXDOUBLE,
	                                                    0,
	                                                    G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_X,
	                                g_param_spec_double("x",
	                                                    "x",
	                                                    "x",
	                                                    0,
	                                                    G_MAXDOUBLE,
	                                                    0,
	                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property(object_class,
	                                PROP_Y,
	                                g_param_spec_double("y",
	                                                    "y",
	                                                    "y",
	                                                    0,
	                                                    G_MAXDOUBLE,
	                                                    0,
	                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property(object_class,
	                                PROP_WIDTH,
	                                g_param_spec_double("width",
	                                                    "width",
	                                                    "width",
	                                                    0,
	                                                    G_MAXDOUBLE,
	                                                    0,
	                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property(object_class,
	                                PROP_HEIGHT,
	                                g_param_spec_double("height",
	                                                    "height",
	                                                    "height",
	                                                    0,
	                                                    G_MAXDOUBLE,
	                                                    0,
	                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property(object_class,
	                                PROP_SEQUENCE,
	                                g_param_spec_int("sequence",
	                                                 "sequence",
	                                                 "sequence",
	                                                 0,
	                                                 G_MAXINT,
	                                                 0,
	                                                 G_PARAM_READABLE));

	/**
	 * PpgTaskRender:surface:
	 * @surface: A #cairo_surface_t.
	 *
	 * The surface to render upon. Since pixman is not thread safe in reference
	 * counting, this property is a pointer type. The reference count of the
	 * surface is not changed. It is expected that the caller set the reference
	 * explicitly and free it from the same thread they referenced it from
	 * when the task has completed.
	 */
	g_object_class_install_property(object_class,
	                                PROP_SURFACE,
	                                g_param_spec_pointer("surface",
	                                                     "Surface",
	                                                     "The cairo surface",
	                                                     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	signals[RENDER] = g_signal_new("render",
	                               PPG_TYPE_TASK_RENDER,
	                               G_SIGNAL_RUN_LAST,
	                               G_STRUCT_OFFSET(PpgTaskRenderClass, render),
	                               NULL, NULL,
	                               g_cclosure_marshal_VOID__VOID,
	                               G_TYPE_NONE,
	                               0);

	task_queue = g_async_queue_new();

#if MAX_THREADS > 1
	/*
	 * Create rendering threads if we are not using the main loop for
	 * rendering.
	 */
	n_threads = MIN(ppg_get_num_cpus(), MAX_THREADS);
	for (i = 0; i < n_threads; i++) {
		task_threads[i] = g_thread_create(ppg_task_render_thread,
		                                  task_queue, TRUE, NULL);
	}
#endif
}


/**
 * ppg_task_render_init:
 * @render: (in): A #PpgTaskRender.
 *
 * Initializes the newly created #PpgTaskRender instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_task_render_init (PpgTaskRender *render)
{
	static guint sequence = 0;

	render->priv = G_TYPE_INSTANCE_GET_PRIVATE(render, PPG_TYPE_TASK_RENDER,
	                                           PpgTaskRenderPrivate);

	render->priv->sequence = sequence++;
	ppg_task_use_idle(PPG_TASK(render), TRUE);
}

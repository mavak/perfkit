/* ppg-task.c
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
#include "ppg-task.h"
#include "ppg-util.h"


G_DEFINE_ABSTRACT_TYPE(PpgTask, ppg_task, G_TYPE_INITIALLY_UNOWNED)


struct _PpgTaskPrivate
{
	PpgTaskState state;
	GError *error;
	GMutex *mutex;
	gboolean use_idle;
};


enum
{
	PROP_0,

	PROP_STATE,
	PROP_ERROR,
};


/*
 * Globals.
 */
static GThreadPool *threadpool = NULL;


/**
 * ppg_task_emit_notify_state_idle:
 * @task: (in): A #PpgTask.
 *
 * A GSourceFunc to notify the state has changed from the main loop.
 *
 * Returns: %FALSE always.
 * Side effects: None.
 */
static gboolean
ppg_task_emit_notify_state_idle (gpointer data)
{
	PpgTask *task = (PpgTask *)data;

	g_object_notify(G_OBJECT(task), "state");
	g_object_unref(task);
	return FALSE;
}


/**
 * ppg_task_emit_notify_state:
 * @task: (in): A #PpgTask.
 *
 * Emits the "notify::state" signal. If the task requested that this be done
 * from the main loop, the Gdk main thread will be acquired.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_task_emit_notify_state (PpgTask *task)
{
	if (task->priv->use_idle) {
		g_timeout_add(0, ppg_task_emit_notify_state_idle,
		              g_object_ref(task));
	} else {
		g_object_notify(G_OBJECT(task), "state");
	}
}


/**
 * ppg_task_cancel:
 * @task: (in): A #PpgTask.
 *
 * Cancels a PpgTask. If the task is not already running, it will not ever
 * be run.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_task_cancel (PpgTask *task)
{
	PpgTaskPrivate *priv;
	gboolean emit = FALSE;

	g_return_if_fail(PPG_IS_TASK(task));

	priv = task->priv;

	g_mutex_lock(priv->mutex);

	switch (priv->state) {
	case PPG_TASK_INITIAL:
	case PPG_TASK_RUNNING:
		priv->state = PPG_TASK_FAILED;
		g_set_error(&priv->error, PPG_TASK_ERROR, PPG_TASK_ERROR_CANCELLED,
		            "The given task was cancelled.");
		emit = TRUE;
		break;
	case PPG_TASK_FAILED:
	case PPG_TASK_SUCCESS:
	case PPG_TASK_FINISHED_MASK:
	default:
		break;
	}

	g_mutex_unlock(priv->mutex);

	if (emit) {
		ppg_task_emit_notify_state(task);
	}
}


/**
 * ppg_task_fail:
 * @task: (in): A #PpgTask.
 * @error: (in): A #GError or %NULL.
 *
 * Completes a task and sets it to failed with an error.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_task_fail (PpgTask      *task,
               const GError *error)
{
	PpgTaskPrivate *priv;
	gdouble emit = FALSE;

	g_return_if_fail(PPG_IS_TASK(task));

	priv = task->priv;

	g_mutex_lock(priv->mutex);

	switch (priv->state) {
	case PPG_TASK_INITIAL:
	case PPG_TASK_RUNNING:
		priv->error = g_error_copy(error);
		priv->state = PPG_TASK_FAILED;
		emit = TRUE;
		break;
	case PPG_TASK_FAILED:
	case PPG_TASK_SUCCESS:
		break;
	case PPG_TASK_FINISHED_MASK:
	default:
		g_assert_not_reached();
		return;
	}

	g_mutex_unlock(priv->mutex);

	if (emit) {
		ppg_task_emit_notify_state(task);
	}
}


/**
 * ppg_task_finish:
 * @task: (in): A #PpgTask.
 *
 * Completes the task successfully.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_task_finish (PpgTask *task)
{
	PpgTaskPrivate *priv;
	gboolean emit = FALSE;

	g_return_if_fail(PPG_IS_TASK(task));

	priv = task->priv;

	g_mutex_lock(priv->mutex);

	switch (priv->state) {
	case PPG_TASK_INITIAL:
	case PPG_TASK_RUNNING:
		priv->state = PPG_TASK_SUCCESS;
		emit = TRUE;
		break;
	case PPG_TASK_FAILED:
	case PPG_TASK_SUCCESS:
		break;
	case PPG_TASK_FINISHED_MASK:
	default:
		g_assert_not_reached();
		return;
	}

	g_mutex_unlock(priv->mutex);

	if (emit) {
		ppg_task_emit_notify_state(task);
	}
}


/**
 * ppg_task_run:
 * @task: (in): A #PpgTask.
 *
 * Runs the task (synchronously).
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_task_run (PpgTask *task)
{
	PpgTaskPrivate *priv;
	gboolean run = FALSE;

	task = g_object_ref(task);
	priv = task->priv;

	g_mutex_lock(priv->mutex);

	if (priv->state == PPG_TASK_INITIAL) {
		priv->state = PPG_TASK_RUNNING;
		run = TRUE;
	}

	g_mutex_unlock(priv->mutex);

	if (run) {
		if (PPG_TASK_GET_CLASS(task)->run) {
			PPG_TASK_GET_CLASS(task)->run(task);
		}
	}

	g_object_unref(task);
}


/**
 * ppg_task_schedule:
 * @task: (in): A #PpgTask.
 *
 * Schedules a task to be run asynchronously. By default, the task is
 * scheduled onto the default threadpool for tasks. The size of the
 * threadpool matches the number of cpus.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_task_schedule (PpgTask *task)
{
	PPG_TASK_GET_CLASS(task)->schedule(task);
}


/**
 * ppg_task_real_schedule:
 * @task: (in): A #PpgTask.
 *
 * Schedules a task to run on the default threadpool.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_task_real_schedule (PpgTask *task)
{
	g_thread_pool_push(threadpool, g_object_ref_sink(task), NULL);
}


/**
 * ppg_task_use_idle:
 * @task: (in): A #PpgTask.
 * @use_idle: (in): If state changes should propagate from main thread.
 *
 * Sets if the task should report state changes from the idle thread.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_task_use_idle (PpgTask  *task,
                   gboolean  use_idle)
{
	g_return_if_fail(PPG_IS_TASK(task));
	task->priv->use_idle = use_idle;
}


static void
ppg_task_thread (gpointer data,
                 gpointer user_data)
{
	PpgTask *task = (PpgTask *)data;

	ppg_task_run(task);
	g_object_unref(task);
}


/**
 * ppg_task_finalize:
 * @object: (in): A #PpgTask.
 *
 * Finalizer for a #PpgTask instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_task_finalize (GObject *object)
{
	PpgTaskPrivate *priv = PPG_TASK(object)->priv;

	ENTRY;

	g_clear_error(&priv->error);
	g_mutex_free(priv->mutex);

	G_OBJECT_CLASS(ppg_task_parent_class)->finalize(object);

	EXIT;
}


/**
 * ppg_task_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (out): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Get a given #GObject property.
 */
static void
ppg_task_get_property (GObject    *object,
                       guint       prop_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
	PpgTask *task = PPG_TASK(object);

	switch (prop_id) {
	case PROP_ERROR:
		g_value_set_boxed(value, task->priv->error);
		break;
	case PROP_STATE:
		g_value_set_enum(value, task->priv->state);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}


/**
 * ppg_task_class_init:
 * @klass: (in): A #PpgTaskClass.
 *
 * Initializes the #PpgTaskClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_task_class_init (PpgTaskClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_task_finalize;
	object_class->get_property = ppg_task_get_property;
	g_type_class_add_private(object_class, sizeof(PpgTaskPrivate));

	klass->schedule = ppg_task_real_schedule;

	g_object_class_install_property(object_class,
	                                PROP_STATE,
	                                g_param_spec_enum("state",
	                                                  "state",
	                                                  "state",
	                                                  PPG_TYPE_TASK_STATE,
	                                                  PPG_TASK_INITIAL,
	                                                  G_PARAM_READABLE));

	g_object_class_install_property(object_class,
	                                PROP_ERROR,
	                                g_param_spec_boxed("error",
	                                                   "error",
	                                                   "error",
	                                                   G_TYPE_ERROR,
	                                                   G_PARAM_READABLE));

	/*
	 * Create threadpool for generic tasks. Sized the same as the number
	 * of CPUs.
	 */
	threadpool = g_thread_pool_new(ppg_task_thread, NULL,
	                               ppg_get_num_cpus(),
	                               FALSE, NULL);
	if (!threadpool) {
		g_error("Failed to create task threadpool");
		g_assert(FALSE);
	}
}


/**
 * ppg_task_init:
 * @task: (in): A #PpgTask.
 *
 * Initializes the newly created #PpgTask instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_task_init (PpgTask *task)
{
	task->priv = G_TYPE_INSTANCE_GET_PRIVATE(task, PPG_TYPE_TASK, PpgTaskPrivate);
	task->priv->mutex = g_mutex_new();
}


/**
 * ppg_task_state_get_type:
 *
 * Retrieves the GType for PpgTaskState.
 *
 * Returns: A #GType.
 * Side effects: Registers #GType on first call.
 */
GType
ppg_task_state_get_type (void)
{
	static GType type_id = 0;
	static const GEnumValue values[] = {
		{ PPG_TASK_INITIAL, "PPG_TASK_INITIAL", "INITIAL" },
		{ PPG_TASK_RUNNING, "PPG_TASK_RUNNING", "RUNNING" },
		{ PPG_TASK_SUCCESS, "PPG_TASK_SUCCESS", "SUCCESS" },
		{ PPG_TASK_FAILED,  "PPG_TASK_FAILED",  "FAILED" },
		{ 0 }
	};

	if (G_UNLIKELY(!type_id)) {
		type_id = g_enum_register_static("PpgTaskState", values);
	}
	return type_id;
}


/**
 * ppg_task_error_quark:
 *
 * Returns the error quark for PpgTask.
 *
 * Returns: A GQuark.
 * Side effects: None.
 */
GQuark
ppg_task_error_quark (void)
{
	return g_quark_from_static_string("ppg-task-error-quark");
}

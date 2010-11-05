/* ppg-edit-channel-task.c
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

#include "ppg-edit-channel-task.h"

G_DEFINE_TYPE(PpgEditChannelTask, ppg_edit_channel_task, PPG_TYPE_SESSION_TASK)

struct _PpgEditChannelTaskPrivate
{
	gchar **args;
	gchar **env;
	gchar  *target;
	gchar  *working_dir;
	GPid    pid;

	gboolean target_done;
	gboolean args_done;
	gboolean env_done;
	gboolean dir_done;
	gboolean pid_done;
};

enum
{
	PROP_0,
	PROP_ARGS,
	PROP_ENV,
	PROP_PID,
	PROP_TARGET,
	PROP_WORKING_DIR,
};

static void
ppg_edit_channel_task_try_finish (PpgEditChannelTask *task)
{
	PpgEditChannelTaskPrivate *priv = task->priv;

	if (priv->target_done &&
	    priv->args_done &&
	    priv->dir_done &&
	    priv->env_done &&
	    priv->pid_done) {
		ppg_task_finish(PPG_TASK(task));
	}
}

static void
ppg_edit_channel_task_pid_set (GObject      *object,
                               GAsyncResult *result,
                               gpointer      user_data)
{
	PkConnection *conn = (PkConnection *)object;
	PpgTask *task = (PpgTask *)user_data;
	PpgEditChannelTaskPrivate *priv;
	GError *error = NULL;

	g_return_if_fail(PPG_IS_EDIT_CHANNEL_TASK(task));

	priv = PPG_EDIT_CHANNEL_TASK(task)->priv;

	if (!pk_connection_channel_set_pid_finish(conn, result, &error)) {
		ppg_task_finish_with_error(task, error);
	}

	priv->pid_done = TRUE;
	ppg_edit_channel_task_try_finish(PPG_EDIT_CHANNEL_TASK(task));
}

static void
ppg_edit_channel_task_target_set (GObject      *object,
                                  GAsyncResult *result,
                                  gpointer      user_data)
{
	PkConnection *conn = (PkConnection *)object;
	PpgTask *task = (PpgTask *)user_data;
	PpgEditChannelTaskPrivate *priv;
	GError *error = NULL;

	g_return_if_fail(PPG_IS_EDIT_CHANNEL_TASK(task));

	priv = PPG_EDIT_CHANNEL_TASK(task)->priv;

	if (!pk_connection_channel_set_target_finish(conn, result, &error)) {
		ppg_task_finish_with_error(task, error);
	}

	priv->target_done = TRUE;
	ppg_edit_channel_task_try_finish(PPG_EDIT_CHANNEL_TASK(task));
}

static void
ppg_edit_channel_task_working_dir_set (GObject      *object,
                                       GAsyncResult *result,
                                       gpointer      user_data)
{
	PkConnection *conn = (PkConnection *)object;
	PpgTask *task = (PpgTask *)user_data;
	PpgEditChannelTaskPrivate *priv;
	GError *error = NULL;

	g_return_if_fail(PPG_IS_EDIT_CHANNEL_TASK(task));

	priv = PPG_EDIT_CHANNEL_TASK(task)->priv;

	if (!pk_connection_channel_set_working_dir_finish(conn, result, &error)) {
		ppg_task_finish_with_error(task, error);
	}

	priv->dir_done = TRUE;
	ppg_edit_channel_task_try_finish(PPG_EDIT_CHANNEL_TASK(task));
}

static void
ppg_edit_channel_task_args_set (GObject      *object,
                                GAsyncResult *result,
                                gpointer      user_data)
{
	PkConnection *conn = (PkConnection *)object;
	PpgTask *task = (PpgTask *)user_data;
	PpgEditChannelTaskPrivate *priv;
	GError *error = NULL;

	g_return_if_fail(PPG_IS_EDIT_CHANNEL_TASK(task));

	priv = PPG_EDIT_CHANNEL_TASK(task)->priv;

	if (!pk_connection_channel_set_args_finish(conn, result, &error)) {
		ppg_task_finish_with_error(task, error);
	}

	priv->args_done = TRUE;
	ppg_edit_channel_task_try_finish(PPG_EDIT_CHANNEL_TASK(task));
}

static void
ppg_edit_channel_task_env_set (GObject      *object,
                               GAsyncResult *result,
                               gpointer      user_data)
{
	PkConnection *conn = (PkConnection *)object;
	PpgTask *task = (PpgTask *)user_data;
	PpgEditChannelTaskPrivate *priv;
	GError *error = NULL;

	g_return_if_fail(PPG_IS_EDIT_CHANNEL_TASK(task));

	priv = PPG_EDIT_CHANNEL_TASK(task)->priv;

	if (!pk_connection_channel_set_env_finish(conn, result, &error)) {
		ppg_task_finish_with_error(task, error);
	}

	priv->env_done = TRUE;
	ppg_edit_channel_task_try_finish(PPG_EDIT_CHANNEL_TASK(task));
}

static void
ppg_edit_channel_task_run (PpgTask *task)
{
	PpgEditChannelTask *edit = (PpgEditChannelTask *)task;
	PpgEditChannelTaskPrivate *priv;
	PpgSession *session;
	PkConnection *conn;
	gint channel;

	g_return_if_fail(PPG_IS_EDIT_CHANNEL_TASK(edit));

	priv = edit->priv;

	session = ppg_session_task_get_session(PPG_SESSION_TASK(task));
	if (!session) {
		ppg_task_finish_with_error(task, NULL);
		return;
	}

	g_object_get(session,
	             "channel", &channel,
	             "connection", &conn,
	             NULL);

	if (!conn || channel < 0) {
		ppg_task_finish_with_error(task, NULL);
		return;
	}

	pk_connection_channel_set_target_async(conn, channel, priv->target, NULL,
	                                       ppg_edit_channel_task_target_set,
	                                       task);

	if (priv->args) {
		pk_connection_channel_set_args_async(conn, channel,
		                                     (const gchar **)priv->args, NULL,
		                                     ppg_edit_channel_task_args_set,
		                                     task);
	} else {
		priv->args_done = TRUE;
	}

	if (priv->env) {
		pk_connection_channel_set_env_async(conn, channel,
		                                    (const gchar **)priv->env, NULL,
		                                    ppg_edit_channel_task_env_set,
		                                    task);
	} else {
		priv->env_done = TRUE;
	}

	if (priv->working_dir) {
		pk_connection_channel_set_working_dir_async(conn, channel,
		                                            priv->working_dir, NULL,
		                                            ppg_edit_channel_task_working_dir_set,
		                                            task);
	} else {
		priv->dir_done = TRUE;
	}

	if (priv->pid) {
		pk_connection_channel_set_pid_async(conn, channel, priv->pid, NULL,
		                                    ppg_edit_channel_task_pid_set,
		                                    task);
	} else {
		priv->pid_done = TRUE;
	}

	ppg_edit_channel_task_try_finish(edit);

	g_object_set(session,
	             "args", priv->args,
	             "env", priv->env,
	             "target", priv->target,
	             "pid", priv->pid,
	             NULL);

	g_object_unref(conn);
}

static void
ppg_edit_channel_task_set_target (PpgEditChannelTask *task,
                                  const gchar        *target)
{
	PpgEditChannelTaskPrivate *priv;

	g_return_if_fail(PPG_IS_EDIT_CHANNEL_TASK(task));

	priv = task->priv;
	g_free(priv->target);
	priv->target = g_strdup(target);
}

static void
ppg_edit_channel_task_set_working_dir (PpgEditChannelTask *task,
                                       const gchar        *working_dir)
{
	PpgEditChannelTaskPrivate *priv;

	g_return_if_fail(PPG_IS_EDIT_CHANNEL_TASK(task));

	priv = task->priv;
	g_free(priv->working_dir);
	priv->working_dir = g_strdup(working_dir);
}

static void
ppg_edit_channel_task_set_args (PpgEditChannelTask  *task,
                                gchar              **args)
{
	PpgEditChannelTaskPrivate *priv;

	g_return_if_fail(PPG_IS_EDIT_CHANNEL_TASK(task));

	priv = task->priv;
	g_strfreev(priv->args);
	priv->args = g_strdupv(args);
}

static void
ppg_edit_channel_task_set_env (PpgEditChannelTask  *task,
                               gchar **env)
{
	PpgEditChannelTaskPrivate *priv;

	g_return_if_fail(PPG_IS_EDIT_CHANNEL_TASK(task));

	priv = task->priv;
	g_strfreev(priv->env);
	priv->env = g_strdupv(env);
}

static void	
ppg_edit_channel_task_set_pid (PpgEditChannelTask *task,
                               GPid pid)
{
	PpgEditChannelTaskPrivate *priv;

	g_return_if_fail(PPG_IS_EDIT_CHANNEL_TASK(task));

	priv = task->priv;
	priv->pid = pid;
}

/**
 * ppg_edit_channel_task_finalize:
 * @object: (in): A #PpgEditChannelTask.
 *
 * Finalizer for a #PpgEditChannelTask instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_edit_channel_task_finalize (GObject *object)
{
	G_OBJECT_CLASS(ppg_edit_channel_task_parent_class)->finalize(object);
}

/**
 * ppg_edit_channel_task_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
static void
ppg_edit_channel_task_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
	PpgEditChannelTask *task = PPG_EDIT_CHANNEL_TASK(object);

	switch (prop_id) {
	case PROP_TARGET:
		ppg_edit_channel_task_set_target(task, g_value_get_string(value));
		break;
	case PROP_WORKING_DIR:
		ppg_edit_channel_task_set_working_dir(task, g_value_get_string(value));
		break;
	case PROP_ARGS:
		ppg_edit_channel_task_set_args(task, g_value_get_boxed(value));
		break;
	case PROP_ENV:
		ppg_edit_channel_task_set_env(task, g_value_get_boxed(value));
		break;
	case PROP_PID:
		ppg_edit_channel_task_set_pid(task, g_value_get_uint(value));
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * ppg_edit_channel_task_class_init:
 * @klass: (in): A #PpgEditChannelTaskClass.
 *
 * Initializes the #PpgEditChannelTaskClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_edit_channel_task_class_init (PpgEditChannelTaskClass *klass)
{
	GObjectClass *object_class;
	PpgTaskClass *task_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_edit_channel_task_finalize;
	object_class->set_property = ppg_edit_channel_task_set_property;
	g_type_class_add_private(object_class, sizeof(PpgEditChannelTaskPrivate));

	task_class = PPG_TASK_CLASS(klass);
	task_class->run = ppg_edit_channel_task_run;

	g_object_class_install_property(object_class,
	                                PROP_TARGET,
	                                g_param_spec_string("target",
	                                                    "target",
	                                                    "target",
	                                                    NULL,
	                                                    G_PARAM_WRITABLE));

	g_object_class_install_property(object_class,
	                                PROP_WORKING_DIR,
	                                g_param_spec_string("working-dir",
	                                                    "working-dir",
	                                                    "working-dir",
	                                                    NULL,
	                                                    G_PARAM_WRITABLE));

	g_object_class_install_property(object_class,
	                                PROP_ARGS,
	                                g_param_spec_boxed("args",
	                                                   "args",
	                                                   "args",
	                                                   G_TYPE_STRV,
	                                                   G_PARAM_WRITABLE));

	g_object_class_install_property(object_class,
	                                PROP_ENV,
	                                g_param_spec_boxed("env",
	                                                   "env",
	                                                   "env",
	                                                   G_TYPE_STRV,
	                                                   G_PARAM_WRITABLE));

	g_object_class_install_property(object_class,
	                                PROP_PID,
	                                g_param_spec_uint("pid",
	                                                  "pid",
	                                                  "pid",
	                                                  0,
	                                                  G_MAXSHORT,
	                                                  0,
	                                                  G_PARAM_WRITABLE));
}

/**
 * ppg_edit_channel_task_init:
 * @task: (in): A #PpgEditChannelTask.
 *
 * Initializes the newly created #PpgEditChannelTask instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_edit_channel_task_init (PpgEditChannelTask *task)
{
	task->priv = G_TYPE_INSTANCE_GET_PRIVATE(task, PPG_TYPE_EDIT_CHANNEL_TASK,
	                                         PpgEditChannelTaskPrivate);
}

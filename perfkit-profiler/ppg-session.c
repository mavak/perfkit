/* ppg-session.c
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

#include <perfkit/perfkit.h>

#include "ppg-clock-source.h"
#include "ppg-instrument.h"
#include "ppg-log.h"
#include "ppg-session.h"
#include "ppg-util.h"


struct _PpgSessionPrivate
{
	PkConnection    *connection;
	PpgSessionState  state;
	PpgClockSource  *clock;

	struct {
		gchar   **args;
		gint      channel;
		gchar   **env;
		GPid      pid;
		gchar    *target;
		gchar    *working_dir;
		GTimeVal  start_tv;
	} channel;
};


enum
{
	PROP_0,

	PROP_ARGS,
	PROP_CHANNEL,
	PROP_CONNECTION,
	PROP_ELAPSED,
	PROP_ENV,
	PROP_PID,
	PROP_STATE,
	PROP_TARGET,
	PROP_WORKING_DIR,

	PROP_LAST
};


enum
{
	INSTRUMENT_ADDED,

	LAST_SIGNAL
};


G_DEFINE_TYPE(PpgSession, ppg_session, G_TYPE_INITIALLY_UNOWNED)


/*
 * Globals.
 */
static GParamSpec *pspecs[PROP_LAST] = { 0 };
static guint       signals[LAST_SIGNAL] = { 0 };


/**
 * ppg_session_set_state:
 * @session: (in): A #PpgSession.
 * @state: (in): A #PpgSessionState.
 *
 * Set the state of the session.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_set_state (PpgSession      *session,
                       PpgSessionState  state)
{
	PpgSessionPrivate *priv;

	g_return_if_fail(PPG_IS_SESSION(session));
	g_return_if_fail(state >= PPG_SESSION_INITIAL);
	g_return_if_fail(state <= PPG_SESSION_FAILED);

	priv = session->priv;

	if (priv->state != state) {
		priv->state = state;
		g_object_notify_by_pspec(G_OBJECT(session), pspecs[PROP_STATE]);
	}
}


/**
 * ppg_session_clock_sync:
 * @session: (in): A #PpgSession.
 * @clock_: (in): A #PpgClockSource.
 *
 * Notify a synchronization of the clock.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_clock_sync (PpgSession     *session,
                        PpgClockSource *clock_)
{
	g_object_notify_by_pspec(G_OBJECT(session), pspecs[PROP_ELAPSED]);
}


static void
ppg_session_report_error (PpgSession   *session,
                          const GError *error)
{
	ENTRY;
	EXIT;
}


static void
ppg_session_add_channel_cb (GObject      *object,
                            GAsyncResult *result,
                            gpointer      user_data)
{
	PpgSessionPrivate *priv;
	PkConnection *connection = (PkConnection *)object;
	PpgSession *session = (PpgSession *)user_data;
	GError *error = NULL;

	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(PPG_IS_SESSION(session));

	priv = session->priv;

	if (!pk_connection_manager_add_channel_finish(connection, result,
	                                              &priv->channel.channel,
	                                              &error)) {
		ppg_session_report_error(session, error);
		ppg_session_set_state(session, PPG_SESSION_FAILED);
		g_clear_error(&error);
		return;
	}

	ppg_session_set_state(session, PPG_SESSION_READY);
}


static void
ppg_session_set_connected (PpgSession *session)
{
	PpgSessionPrivate *priv;
	PpgClockSource *clock_;

	g_return_if_fail(PPG_IS_SESSION(session));

	priv = session->priv;

	/*
	 * Setup clock-sync source.
	 */
	clock_ = g_object_new(PPG_TYPE_CLOCK_SOURCE,
	                      "connection", priv->connection,
	                      NULL);
	g_signal_connect_swapped(clock_, "clock-sync",
	                         G_CALLBACK(ppg_session_clock_sync),
	                         session);
	priv->clock = g_object_ref_sink(clock_);

	/*
	 * Create a channel for our session.
	 */
	pk_connection_manager_add_channel_async(priv->connection, NULL,
	                                        ppg_session_add_channel_cb,
	                                        session);
}


static void
ppg_session_connect_cb (GObject      *object,
                        GAsyncResult *result,
                        gpointer      user_data)
{
	PkConnection *connection = (PkConnection *)object;
	PpgSession *session = (PpgSession *)user_data;
	GError *error = NULL;

	ENTRY;

	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(PPG_IS_SESSION(session));

	if (!pk_connection_connect_finish(connection, result, &error)) {
		ppg_session_report_error(session, error);
		g_clear_error(&error);
		GOTO(failure);
	}

	ppg_session_set_connected(session);

  failure:
	g_object_unref(session);
	EXIT;
}


/**
 * ppg_session_set_connection:
 * @session: (in): A #PpgSession.
 * @connection: (in): A #PkConnection.
 *
 * Set the connection for the session. This may only be called once
 * for a given session.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_set_connection (PpgSession   *session,
                            PkConnection *connection)
{
	PpgSessionPrivate *priv;

	g_return_if_fail(PPG_IS_SESSION(session));
	g_return_if_fail(session->priv->connection == NULL);

	priv = session->priv;

	priv->connection = g_object_ref(connection);

	if (pk_connection_is_connected(connection)) {
		ppg_session_set_connected(session);
	} else {
		pk_connection_connect_async(connection, NULL,
		                            ppg_session_connect_cb,
		                            g_object_ref(session));
	}
}


static void
ppg_session_set_pid (PpgSession *session,
                     GPid        pid)
{
	PpgSessionPrivate *priv;

	g_return_if_fail(PPG_IS_SESSION(session));

	priv = session->priv;

	priv->channel.pid = pid;
	g_object_notify_by_pspec(G_OBJECT(session), pspecs[PROP_PID]);
}


static void
ppg_session_set_args_cb (GObject      *object,
                         GAsyncResult *result,
                         gpointer      user_data)
{
	PpgSessionPrivate *priv;
	PkConnection *connection = (PkConnection *)object;
	PpgSession *session = (PpgSession *)user_data;
	GError *error = NULL;

	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(PPG_IS_SESSION(session));

	priv = session->priv;

	if (!pk_connection_channel_set_args_finish(connection, result, &error)) {
		ppg_session_report_error(session, error);
		g_clear_error(&error);
		GOTO(failure);
	}

  failure:
	g_object_unref(session);
}


static void
ppg_session_set_args (PpgSession  *session,
                      gchar      **args)
{
	PpgSessionPrivate *priv;

	g_return_if_fail(PPG_IS_SESSION(session));

	priv = session->priv;

	g_strfreev(priv->channel.args);
	priv->channel.args = g_strdupv(args);
	pk_connection_channel_set_args_async(priv->connection,
	                                     priv->channel.channel,
	                                     (const gchar **)args,
	                                     NULL,
	                                     ppg_session_set_args_cb,
	                                     g_object_ref(session));
	g_object_notify(G_OBJECT(session), "args");
}


static void
ppg_session_set_env_cb (GObject      *object,
                        GAsyncResult *result,
                        gpointer      user_data)
{
	PpgSessionPrivate *priv;
	PkConnection *connection = (PkConnection *)object;
	PpgSession *session = (PpgSession *)user_data;
	GError *error = NULL;

	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(PPG_IS_SESSION(session));

	priv = session->priv;

	if (!pk_connection_channel_set_env_finish(connection, result, &error)) {
		ppg_session_report_error(session, error);
		g_clear_error(&error);
		GOTO(failure);
	}

  failure:
	g_object_unref(session);
}


static void
ppg_session_set_env (PpgSession  *session,
                      gchar      **env)
{
	PpgSessionPrivate *priv;

	g_return_if_fail(PPG_IS_SESSION(session));

	priv = session->priv;

	g_strfreev(priv->channel.env);
	priv->channel.env = g_strdupv(env);
	pk_connection_channel_set_env_async(priv->connection,
	                                    priv->channel.channel,
	                                    (const gchar **)env,
	                                    NULL,
	                                    ppg_session_set_env_cb,
	                                    g_object_ref(session));
	g_object_notify(G_OBJECT(session), "env");
}


static void
ppg_session_set_target_cb (GObject      *object,
                           GAsyncResult *result,
                           gpointer      user_data)
{
	PpgSessionPrivate *priv;
	PkConnection *connection = (PkConnection *)object;
	PpgSession *session = (PpgSession *)user_data;
	GError *error = NULL;

	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(PPG_IS_SESSION(session));

	priv = session->priv;

	if (!pk_connection_channel_set_target_finish(connection, result, &error)) {
		ppg_session_report_error(session, error);
		g_clear_error(&error);
		GOTO(failure);
	}

  failure:
	g_object_unref(session);
}


static void
ppg_session_set_target (PpgSession  *session,
                        const gchar *target)
{
	PpgSessionPrivate *priv;

	g_return_if_fail(PPG_IS_SESSION(session));

	priv = session->priv;

	g_free(priv->channel.target);
	priv->channel.target = g_strdup(target);
	pk_connection_channel_set_target_async(priv->connection,
	                                       priv->channel.channel,
	                                       target,
	                                       NULL,
	                                       ppg_session_set_target_cb,
	                                       g_object_ref(session));
	g_object_notify(G_OBJECT(session), "target");
}


static void
ppg_session_set_working_dir_cb (GObject      *object,
                                GAsyncResult *result,
                                gpointer      user_data)
{
	PpgSessionPrivate *priv;
	PkConnection *connection = (PkConnection *)object;
	PpgSession *session = (PpgSession *)user_data;
	GError *error = NULL;

	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(PPG_IS_SESSION(session));

	priv = session->priv;

	if (!pk_connection_channel_set_working_dir_finish(connection,
	                                                  result,
	                                                  &error)) {
		ppg_session_report_error(session, error);
		g_clear_error(&error);
		GOTO(failure);
	}

  failure:
	g_object_unref(session);
}


static void
ppg_session_set_working_dir (PpgSession  *session,
                             const gchar *working_dir)
{
	PpgSessionPrivate *priv;

	g_return_if_fail(PPG_IS_SESSION(session));

	priv = session->priv;

	g_free(priv->channel.working_dir);
	priv->channel.working_dir = g_strdup(working_dir);
	pk_connection_channel_set_working_dir_async(priv->connection,
	                                            priv->channel.channel,
	                                            working_dir,
	                                            NULL,
	                                            ppg_session_set_working_dir_cb,
	                                            g_object_ref(session));
	g_object_notify(G_OBJECT(session), "working-dir");
}


static void
ppg_session_start_cb (GObject      *object,
                      GAsyncResult *result,
                      gpointer      user_data)
{
	PpgSessionPrivate *priv;
	PkConnection *connection = (PkConnection *)object;
	PpgSession *session = (PpgSession *)user_data;
	GError *error = NULL;

	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(PPG_IS_SESSION(session));

	priv = session->priv;

	if (!pk_connection_channel_start_finish(connection, result,
	                                        &priv->channel.start_tv,
	                                        &error)) {
		ppg_session_report_error(session, error);
		g_clear_error(&error);
		GOTO(failure);
	}

	ppg_session_set_state(session, PPG_SESSION_STARTED);
	ppg_clock_source_start(priv->clock, &priv->channel.start_tv);

  failure:
	g_object_unref(session);
}


/**
 * ppg_session_start:
 * @session: (in): A #PpgSession.
 *
 * Starts the session on the agent.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_session_start (PpgSession *session)
{
	PpgSessionPrivate *priv;

	g_return_if_fail(PPG_IS_SESSION(session));

	priv = session->priv;

	switch (priv->state) {
	case PPG_SESSION_INITIAL:
	case PPG_SESSION_READY:
	case PPG_SESSION_STOPPED:
		pk_connection_channel_start_async(priv->connection,
		                                  priv->channel.channel,
		                                  NULL,
		                                  ppg_session_start_cb,
		                                  g_object_ref(session));
		break;
	case PPG_SESSION_FAILED:
	case PPG_SESSION_MUTED:
	case PPG_SESSION_STARTED:
		break;
	default:
		g_assert_not_reached();
		return;
	}
}


static void
ppg_session_stop_cb (GObject      *object,
                     GAsyncResult *result,
                     gpointer      user_data)
{
	PkConnection *connection = (PkConnection *)object;
	PpgSession *session = (PpgSession *)user_data;
	GError *error = NULL;

	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(PPG_IS_SESSION(session));

	if (!pk_connection_channel_stop_finish(connection, result, &error)) {
		ppg_session_report_error(session, error);
		g_clear_error(&error);
		GOTO(failure);
	}

	ppg_session_set_state(session, PPG_SESSION_STOPPED);
	ppg_clock_source_stop(session->priv->clock);

  failure:
	g_object_unref(session);
}


void
ppg_session_stop (PpgSession *session)
{
	PpgSessionPrivate *priv;

	g_return_if_fail(PPG_IS_SESSION(session));

	priv = session->priv;

	switch (priv->state) {
	case PPG_SESSION_INITIAL:
	case PPG_SESSION_READY:
		break;
	case PPG_SESSION_FAILED:
	case PPG_SESSION_MUTED:
	case PPG_SESSION_STARTED:
	case PPG_SESSION_STOPPED:
		pk_connection_channel_stop_async(priv->connection,
		                                 priv->channel.channel,
		                                 NULL,
		                                 ppg_session_stop_cb,
		                                 g_object_ref(session));
		break;
	default:
		g_assert_not_reached();
		return;
	}
}


/**
 * ppg_session_get_elapsed:
 * @session: (in): A #PpgSession.
 *
 * Retrieves the amount of time elapsed since the session was started in
 * seconds. The microseconds are provided as a decimal.
 *
 * Returns: The time as a #gdouble.
 * Side effects: None.
 */
gdouble
ppg_session_get_elapsed (PpgSession *session)
{
	g_return_val_if_fail(PPG_IS_SESSION(session), 0.0);
	return ppg_clock_source_get_elapsed(session->priv->clock);
}


PpgSessionState
ppg_session_get_state (PpgSession *session)
{
	g_return_val_if_fail(PPG_IS_SESSION(session), 0);
	return session->priv->state;
}


gboolean
ppg_session_load (PpgSession   *session,
                  const gchar  *filename,
                  GError      **error)
{
	return TRUE;
}


void
ppg_session_add_instrument (PpgSession    *session,
                            PpgInstrument *instrument)
{
	g_signal_emit(session, signals[INSTRUMENT_ADDED], 0, instrument);
}


gdouble
ppg_session_get_started_at (PpgSession *session)
{
	g_return_val_if_fail(PPG_IS_SESSION(session), 0.0);
	return ppg_get_time(&session->priv->channel.start_tv);
}


PkModel*
ppg_session_create_model (PpgSession *session)
{
	PpgSessionPrivate *priv;
	PkModel *model;

	g_return_val_if_fail(PPG_IS_SESSION(session), NULL);

	priv = session->priv;

	/*
	 * TODO: Eventually, use an mmap'able data model.
	 */
	model = g_object_new(PK_TYPE_MODEL_MEMORY, NULL);

	return model;
}


/**
 * ppg_session_dispose:
 * @object: (in): A #GObject.
 *
 * Dispose callback for @object.  This method releases references held
 * by the #GObject instance.
 *
 * Returns: None.
 * Side effects: Plenty.
 */
static void
ppg_session_dispose (GObject *object)
{
	PpgSessionPrivate *priv = PPG_SESSION(object)->priv;
	gpointer instance;

	ENTRY;

	if ((instance = priv->connection)) {
		priv->connection = NULL;
		g_object_unref(instance);
	}

	G_OBJECT_CLASS(ppg_session_parent_class)->dispose(object);

	EXIT;
}


/**
 * ppg_session_finalize:
 * @object: (in): A #PpgSession.
 *
 * Finalizer for a #PpgSession instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_finalize (GObject *object)
{
	PpgSessionPrivate *priv = PPG_SESSION(object)->priv;

	ENTRY;

	ppg_clear_object(&priv->clock);

	G_OBJECT_CLASS(ppg_session_parent_class)->finalize(object);

	EXIT;
}


/**
 * ppg_session_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (out): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Get a given #GObject property.
 */
static void
ppg_session_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
	PpgSession *session = PPG_SESSION(object);

	switch (prop_id) {
	case PROP_ARGS:
		g_value_set_boxed(value, session->priv->channel.args);
		break;
	case PROP_CHANNEL:
		g_value_set_int(value, session->priv->channel.channel);
		break;
	case PROP_CONNECTION:
		g_value_set_object(value, session->priv->connection);
		break;
	case PROP_ELAPSED:
		g_value_set_double(value, ppg_session_get_elapsed(session));
		break;
	case PROP_ENV:
		g_value_set_boxed(value, session->priv->channel.env);
		break;
	case PROP_PID:
		g_value_set_uint(value, session->priv->channel.pid);
		break;
	case PROP_STATE:
		g_value_set_enum(value, session->priv->state);
		break;
	case PROP_TARGET:
		g_value_set_string(value, session->priv->channel.target);
		break;
	case PROP_WORKING_DIR:
		g_value_set_string(value, session->priv->channel.working_dir);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}


/**
 * ppg_session_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
static void
ppg_session_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
	PpgSession *session = PPG_SESSION(object);

	switch (prop_id) {
	case PROP_ARGS:
		ppg_session_set_args(session, g_value_get_boxed(value));
		break;
	case PROP_ENV:
		ppg_session_set_env(session, g_value_get_boxed(value));
		break;
	case PROP_TARGET:
		ppg_session_set_target(session, g_value_get_string(value));
		break;
	case PROP_WORKING_DIR:
		ppg_session_set_working_dir(session, g_value_get_string(value));
		break;
	case PROP_CONNECTION:
		ppg_session_set_connection(session, g_value_get_object(value));
		break;
	case PROP_PID:
		ppg_session_set_pid(session, g_value_get_uint(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}


/**
 * ppg_session_class_init:
 * @klass: (in): A #PpgSessionClass.
 *
 * Initializes the #PpgSessionClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_class_init (PpgSessionClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->dispose = ppg_session_dispose;
	object_class->finalize = ppg_session_finalize;
	object_class->get_property = ppg_session_get_property;
	object_class->set_property = ppg_session_set_property;
	g_type_class_add_private(object_class, sizeof(PpgSessionPrivate));

	pspecs[PROP_ARGS] = g_param_spec_boxed("args",
	                                       "Args",
	                                       "The target executables arguments",
	                                       G_TYPE_STRV,
	                                       G_PARAM_READWRITE);
	g_object_class_install_property(object_class, PROP_ARGS, pspecs[PROP_ARGS]);

	pspecs[PROP_CHANNEL] = g_param_spec_int("channel",
	                                        "Channel",
	                                        "The channel for the session",
	                                        0,
	                                        G_MAXINT,
	                                        0,
	                                        G_PARAM_READABLE);
	g_object_class_install_property(object_class, PROP_CHANNEL, pspecs[PROP_CHANNEL]);

	pspecs[PROP_CONNECTION] = g_param_spec_object("connection",
	                                              "Connection",
	                                              "The connection to a perfkit-agent",
	                                              PK_TYPE_CONNECTION,
	                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
	g_object_class_install_property(object_class, PROP_CONNECTION, pspecs[PROP_CONNECTION]);

	pspecs[PROP_ENV] = g_param_spec_boxed("env",
	                                      "Env",
	                                      "The targets environment",
	                                      G_TYPE_STRV,
	                                      G_PARAM_READWRITE);
	g_object_class_install_property(object_class, PROP_ENV, pspecs[PROP_ENV]);

	/**
	 * PpgSession:elapsed:
	 *
	 * The "elapsed" property contains the amount of time that has elapsed
	 * since the session was started.
	 */
	pspecs[PROP_ELAPSED] = g_param_spec_double("elapsed",
	                                           "ElapsedTime",
	                                           "The amount of time elapsed in the session",
	                                           0.0,
	                                           G_MAXDOUBLE,
	                                           0.0,
	                                           G_PARAM_READABLE);
	g_object_class_install_property(object_class, PROP_ELAPSED, pspecs[PROP_ELAPSED]);

	pspecs[PROP_PID] = g_param_spec_uint("pid",
	                                     "Pid",
	                                     "The pid of the target process",
	                                     0,
	                                     G_MAXUINT,
	                                     0,
	                                     G_PARAM_READWRITE);
	g_object_class_install_property(object_class, PROP_PID, pspecs[PROP_PID]);

	pspecs[PROP_STATE] = g_param_spec_enum("state",
	                                       "State",
	                                       "The state of the session",
	                                       PPG_TYPE_SESSION_STATE,
	                                       PPG_SESSION_INITIAL,
	                                       G_PARAM_READABLE);
	g_object_class_install_property(object_class, PROP_STATE, pspecs[PROP_STATE]);

	pspecs[PROP_TARGET] = g_param_spec_string("target",
	                                          "Target",
	                                          "The target executable to launch",
	                                          NULL,
	                                          G_PARAM_READWRITE);
	g_object_class_install_property(object_class, PROP_TARGET, pspecs[PROP_TARGET]);

	pspecs[PROP_WORKING_DIR] = g_param_spec_string("working-dir",
	                                               "WorkingDir",
	                                               "The directory to run the executable from",
	                                               NULL,
	                                               G_PARAM_READWRITE);
	g_object_class_install_property(object_class, PROP_WORKING_DIR, pspecs[PROP_WORKING_DIR]);

	signals[INSTRUMENT_ADDED] = g_signal_new("instrument-added",
	                                         PPG_TYPE_SESSION,
	                                         G_SIGNAL_RUN_FIRST,
	                                         0,
	                                         NULL,
	                                         NULL,
	                                         g_cclosure_marshal_VOID__OBJECT,
	                                         G_TYPE_NONE,
	                                         1,
	                                         PPG_TYPE_INSTRUMENT);
}


/**
 * ppg_session_init:
 * @session: (in): A #PpgSession.
 *
 * Initializes the newly created #PpgSession instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_init (PpgSession *session)
{
	session->priv = G_TYPE_INSTANCE_GET_PRIVATE(session, PPG_TYPE_SESSION,
	                                            PpgSessionPrivate);
	session->priv->channel.channel = -1;
	ppg_session_set_state(session, PPG_SESSION_INITIAL);
}


GType
ppg_session_state_get_type (void)
{
	static gsize initialized = FALSE;
	static GType type_id;
	static const GEnumValue values[] = {
		{ PPG_SESSION_INITIAL, "PPG_SESSION_INITIAL", "INITIAL" },
		{ PPG_SESSION_READY,   "PPG_SESSION_READY",   "READY" },
		{ PPG_SESSION_STOPPED, "PPG_SESSION_STOPPED", "STOPPED" },
		{ PPG_SESSION_FAILED,  "PPG_SESSION_FAILED",  "FAILED" },
		{ PPG_SESSION_MUTED,   "PPG_SESSION_MUTED",   "MUTED" },
		{ 0 }
	};

	if (g_once_init_enter(&initialized)) {
		type_id = g_enum_register_static("PpgSessionState", values);
		g_once_init_leave(&initialized, TRUE);
	}

	return type_id;
}

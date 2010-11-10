/* pka-channel.c
 *
 * Copyright (C) 2009 Christian Hergert
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "Channel"

#include <fcntl.h>
#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <stdlib.h>
#include <unistd.h>

#include "pka-channel.h"
#include "pka-log.h"
#include "pka-source.h"
#include "pka-util.h"

#define IO_SUCCESS(_f) (G_IO_STATUS_NORMAL == (_f))
#define ENSURE_STATE(_c, _s, _l)                                            \
    G_STMT_START {                                                          \
    	if ((_c)->priv->state != (PKA_CHANNEL_##_s)) {                      \
    		g_set_error(error, PKA_CHANNEL_ERROR, PKA_CHANNEL_ERROR_STATE,  \
                        "Channel is not in the " #_s " state.");            \
            GOTO(_l);                                                       \
        }                                                                   \
    } G_STMT_END

#define RETURN_FIELD(_ch, _ft, _fi)                                         \
    G_STMT_START {                                                          \
        PkaChannelPrivate *priv;                                            \
        _ft ret;                                                            \
        g_return_val_if_fail(PKA_IS_CHANNEL((_ch)), 0);                     \
        ENTRY;                                                              \
        priv = (_ch)->priv;                                                 \
        g_mutex_lock(priv->mutex);                                          \
        ret = priv->_fi;                                                    \
        g_mutex_unlock(priv->mutex);                                        \
        RETURN(ret);                                                        \
    } G_STMT_END

#define RETURN_FIELD_COPY(_ch, _cf, _fi)                                    \
    G_STMT_START {                                                          \
        PkaChannelPrivate *priv;                                            \
        gpointer ret;                                                       \
        g_return_val_if_fail(PKA_IS_CHANNEL((_ch)), NULL);                  \
        ENTRY;                                                              \
        priv = (_ch)->priv;                                                 \
        g_mutex_lock(priv->mutex);                                          \
        ret = _cf(priv->_fi);                                               \
        g_mutex_unlock(priv->mutex);                                        \
        RETURN(ret);                                                        \
    } G_STMT_END

#define AUTHORIZE_IOCTL(_c, _i, _t, _l)                                     \
    G_STMT_START {                                                          \
        /* TODO: handle target (_t) */                                      \
        if (!pka_context_is_authorized(_c, PKA_IOCTL_##_i)) {               \
        	g_set_error(error, PKA_CONTEXT_ERROR,                           \
                        PKA_CONTEXT_ERROR_NOT_AUTHORIZED,                   \
                        "Insufficient permissions for operation.");         \
            GOTO(_l);                                                       \
        }                                                                   \
    } G_STMT_END

#define SET_FIELD_COPY(_ch, _fr, _cp, _t)                                   \
    G_STMT_START {                                                          \
        PkaChannelPrivate *priv;                                            \
        gboolean ret = FALSE;                                               \
        ENTRY;                                                              \
        priv = _ch->priv;                                                   \
        AUTHORIZE_IOCTL(context, MODIFY_CHANNEL, _ch, failed);              \
        g_mutex_lock(priv->mutex);                                          \
        ENSURE_STATE(_ch, READY, unlock);                                   \
        DEBUG(Channel, "Setting " #_t " of channel %d on behalf of "        \
              "context %d.", priv->id, pka_context_get_id(context));        \
        _fr(priv->_t);                                                      \
        priv->_t = _cp(_t);                                                 \
        ret = TRUE;                                                         \
      unlock:                                                               \
      	g_mutex_unlock(priv->mutex);                                        \
      failed:                                                               \
      	RETURN(ret);                                                        \
	} G_STMT_END

/**
 * SECTION:pka-channel
 * @title: PkaChannel
 * @short_description: Data source aggregation and process management
 *
 * #PkaChannel encapsulates the logic for spawning a new inferior process or
 * attaching to an existing one.  #PkaSource<!-- -->'s are added to the
 * channel to provide instrumentation into the inferior.
 *
 * #PkaChannel is designed to be thread-safe.  Therefore, accessor methods
 * such as pka_channel_get_target() return a copy of their current value as
 * opposed to the private copy which could be freed before the caller receives
 * the value.
 */

G_DEFINE_TYPE (PkaChannel, pka_channel, G_TYPE_OBJECT)

/*
 * Internal methods used for management of samples and manifests.
 */
extern void     pka_source_notify_started         (PkaSource       *source,
                                                   PkaSpawnInfo    *spawn_info);
extern void     pka_source_notify_stopped         (PkaSource       *source);
extern void     pka_source_notify_muted           (PkaSource       *source);
extern void     pka_source_notify_unmuted         (PkaSource       *source);
extern void     pka_source_notify_reset           (PkaSource       *source);
extern gboolean pka_source_set_channel            (PkaSource       *source,
                                                   PkaChannel      *channel);

struct _PkaChannelPrivate
{
	gint          id;         /* Monotonic id for the channel. */
	GMutex       *mutex;
	guint         state;      /* Current channel state. */
	GPtrArray    *sources;    /* Array of data sources. */
	GPid          pid;         /* Inferior pid */
	gboolean      pid_set;     /* Pid was attached, not spawned */
	gchar        *working_dir; /* Inferior working directory */
	gchar        *target;      /* Inferior executable */
	gchar       **env;         /* Key=Value environment */
	gchar       **args;        /* Target arguments */
	gboolean      kill_pid;    /* Should inferior be killed upon stop */
	gint          exit_status; /* The inferiors exit status */
	GTimeVal      created_at;  /* When the channel was created */
	GIOChannel   *stdin;
	GIOChannel   *stdout;
	GIOChannel   *stderr;
};

enum
{
	SOURCE_ADDED,
	SOURCE_REMOVED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

/**
 * pka_channel_new:
 *
 * Creates a new instance of #PkaChannel.
 *
 * Return value: the newly created #PkaChannel.
 * Side effects: None.
 */
PkaChannel*
pka_channel_new (void)
{
	ENTRY;
	RETURN(g_object_new(PKA_TYPE_CHANNEL, NULL));
}

/**
 * pka_channel_get_id:
 * @channel: A #PkaChannel
 *
 * Retrieves the channel id.
 *
 * Returns: A guint.
 * Side effects: None.
 */
gint
pka_channel_get_id (PkaChannel *channel) /* IN */
{
	g_return_val_if_fail(PKA_IS_CHANNEL(channel), G_MININT);
	return channel->priv->id;
}

/**
 * pka_channel_get_target:
 * @channel: A #PkaChannel
 *
 * Retrieves the "target" executable for the channel.  If the channel is to
 * connect to an existing process this is %NULL.
 *
 * Returns: A string containing the target.
 *   The value should be freed with g_free().
 * Side effects: None.
 */
gchar*
pka_channel_get_target (PkaChannel *channel) /* IN */
{
	RETURN_FIELD_COPY(channel, g_strdup, target);
}

/**
 * pka_channel_set_target:
 * @channel: A #PkaChannel.
 * @context: A #PkaContext.
 * @target: the target executable.
 * @error: A location for a #GError, or %NULL.
 *
 * Sets the target of the #PkaChannel.  If the context does not have
 * permissions, this operation will fail.  The channel also must not
 * have been started.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_channel_set_target (PkaChannel   *channel, /* IN */
                        PkaContext   *context, /* IN */
                        const gchar  *target,  /* IN */
                        GError      **error)   /* OUT */
{
	SET_FIELD_COPY(channel, g_free, g_strdup, target);
}

/**
 * pka_channel_get_working_dir:
 * @channel: A #PkaChannel
 *
 * Retrieves the "working-dir" property for the channel.  This is the directory
 * the process should be spawned within.
 *
 * Returns: A string containing the working directory.
 *   The value should be freed with g_free().
 * Side effects: None.
 */
gchar*
pka_channel_get_working_dir (PkaChannel *channel) /* IN */
{
	RETURN_FIELD_COPY(channel, g_strdup, working_dir);
}

/**
 * pka_channel_set_working_dir:
 * @channel: A #PkaChannel.
 * @context: A #PkaContext.
 * @working_dir: the working directory for the target.
 * @error: A location for a #GError, or %NULL.
 *
 * Sets the working directory of the inferior process.  If the context
 * does not have permissions, this operation will fail.  Also, this may
 * only be called before the channel has started.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_channel_set_working_dir (PkaChannel   *channel,     /* IN */
                             PkaContext   *context,     /* IN */
                             const gchar  *working_dir, /* IN */
                             GError      **error)       /* OUT */
{
	SET_FIELD_COPY(channel, g_free, g_strdup, working_dir);
}

/**
 * pka_channel_get_args:
 * @channel: A #PkaChannel
 *
 * Retrieves the "args" property.
 *
 * Returns: A string array containing the arguments to the process.  This
 *   value should be freed with g_strfreev().
 * Side effects: None.
 */
gchar**
pka_channel_get_args (PkaChannel *channel) /* IN */
{
	RETURN_FIELD_COPY(channel, g_strdupv, args);
}

/**
 * pka_channel_set_args:
 * @channel: A #PkaChannel.
 * @context: A #PkaContext.
 * @args: The target arguments.
 * @error: A location for a #GError, or %NULL.
 *
 * Sets the arguments for the target.  If the context does not have
 * permissions, this operation will fail.  Also, this may only be called
 * before the session has started.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_channel_set_args (PkaChannel  *channel, /* IN */
                      PkaContext  *context, /* IN */
                      gchar      **args,    /* IN */
                      GError     **error)   /* OUT */
{
	SET_FIELD_COPY(channel, g_strfreev, g_strdupv, args);
}

/**
 * pka_channel_get_env:
 * @channel: A #PkaChannel
 *
 * Retrieves the "env" property.
 *
 * Returns: A string array containing the environment variables to set before
 *   the process has started.  The value should be freed with g_free().
 * Side effects: None.
 */
gchar**
pka_channel_get_env (PkaChannel *channel) /* IN */
{
	RETURN_FIELD_COPY(channel, g_strdupv, env);
}

/**
 * pka_channel_set_env:
 * @channel: A #PkaChannel.
 * @context: A #PkaContext.
 * @env: A #GStrv containing the environment.
 * @error: A location for a #GError, or %NULL.
 *
 * Sets the environment of the target process.  @args should be a #GStrv
 * containing "KEY=VALUE" strings for the environment.  If the context
 * does not have permissions, this operation will fail.  Also, this may
 * only be called before the channel has started.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_channel_set_env (PkaChannel  *channel, /* IN */
                     PkaContext  *context, /* IN */
                     gchar      **env,     /* IN */
                     GError     **error)   /* OUT */
{
	SET_FIELD_COPY(channel, g_strfreev, g_strdupv, env);
}

/**
 * pka_channel_get_pid:
 * @channel: A #PkaChannel
 *
 * Retrieves the pid of the process.  If an existing process is to be monitored
 * this is the pid of that process.  If a new process was to be spawned, after
 * the process has been spawned this will contain its process id.
 *
 * Returns: The process id of the monitored proces.
 *
 * Side effects: None.
 */
GPid
pka_channel_get_pid (PkaChannel *channel)
{
	RETURN_FIELD(channel, GPid, pid);
}

/**
 * pka_channel_set_pid:
 * @channel: A #PkaChannel.
 * @context: A #PkaContext.
 * @pid: A #GPid.
 * @error: A location for #GError, or %NULL.
 *
 * Sets the pid of the process of which to attach.  If set, the channel will
 * not spawn a process, but instead instruct the sources to attach to the
 * existing process.  If the context does not have permissions, this
 * operation will fail.  Also, this may only be called before the channel has
 * been started.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_channel_set_pid (PkaChannel  *channel, /* IN */
                     PkaContext  *context, /* IN */
                     GPid         pid,     /* IN */
                     GError     **error)   /* OUT */
{
	PkaChannelPrivate *priv;
	gboolean ret = FALSE;

	ENTRY;
	priv = channel->priv;
	AUTHORIZE_IOCTL(context, MODIFY_CHANNEL, channel, failed);
	g_mutex_lock(priv->mutex);
	ENSURE_STATE(channel, READY, unlock);
	INFO(Channel, "Setting pid of channel %d to %d on behalf of context %d.",
	     priv->id, pid, pka_context_get_id(context));
	priv->pid = pid;
	priv->pid_set = TRUE;
	g_free(priv->target);
	priv->target = NULL;
	ret = TRUE;
  unlock:
	g_mutex_unlock(priv->mutex);
  failed:
	RETURN(ret);
}

/**
 * pka_channel_get_pid_set:
 * @channel: A #PkaChannel.
 *
 * Determines if the pid was set manually, as opposed to set when the inferior
 * process was spawned.  %TRUE indicates that the channel is attaching to an
 * existing process.
 *
 * Returns: %TRUE if the pid was set; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_channel_get_pid_set (PkaChannel *channel) /* IN */
{
    RETURN_FIELD(channel, gboolean, pid_set);
}

/**
 * pka_channel_get_kill_pid:
 * @channel: A #PkaChannel.
 *
 * Retrieves if the inferior process should be killed if the channel is stopped
 * before it has exited.  This value is ignored if the process was not spawned
 * by Perfkit.
 *
 * Returns: %TRUE if the process will be killed upon stopping the channel;
 *   otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_channel_get_kill_pid (PkaChannel *channel) /* IN */
{
	RETURN_FIELD(channel, gboolean, kill_pid);
}

/**
 * pka_channel_set_kill_pid:
 * @channel: A #PkaChannel.
 * @context: A #PkaContext.
 * @kill_pid: If the inferior should be killed when stopping.
 * @error: A location for a #GError, or %NULL.
 *
 * Sets the "kill_pid" property.  If set, any process spawned by Perfkit will
 * be killed when the channel is stopped.  This is the default.  If the context
 * does not have permissions, this operation will fail.
 *
 * This may be called at any time.  If the process has already been stopped,
 * this will do nothing.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_channel_set_kill_pid (PkaChannel  *channel,  /* IN */
                          PkaContext  *context,  /* IN */
                          gboolean     kill_pid, /* IN */
                          GError     **error)    /* OUT */
{
	PkaChannelPrivate *priv;
	gboolean ret = FALSE;

	ENTRY;
	priv = channel->priv;
	AUTHORIZE_IOCTL(context, MODIFY_CHANNEL, channel, failed);
	g_mutex_lock(priv->mutex);
	DEBUG(Channel, "Setting kill_pid of channel %d to %s on behalf of "
	               "context %d.", priv->id, kill_pid ? "TRUE" : "FALSE",
	               pka_context_get_id(context));
	priv->kill_pid = kill_pid;
	ret = TRUE;
	g_mutex_unlock(priv->mutex);
  failed:
	RETURN(ret);
}

/**
 * pka_channel_get_exit_status:
 * @channel: A #PkaChannel.
 * @error: A location for a #GError, or %NULL.
 *
 * Retrieves the exit status of the inferior process.  The exit status is
 * only accurate if the process was spawned by @channel and has exited.
 *
 * Returns: The exit status of @channels<!-- -->'s inferior.
 * Side effects: None.
 */
gboolean
pka_channel_get_exit_status (PkaChannel  *channel,     /* IN */
                             gint        *exit_status, /* OUT */
                             GError     **error)       /* OUT */
{
	PkaChannelPrivate *priv;
	gboolean ret = FALSE;

	g_return_val_if_fail(PKA_IS_CHANNEL(channel), FALSE);
	g_return_val_if_fail(exit_status != NULL, FALSE);

	ENTRY;
	priv = channel->priv;
	g_mutex_lock(priv->mutex);
	if (priv->state != PKA_CHANNEL_STOPPED) {
		g_set_error(error, PKA_CHANNEL_ERROR, PKA_CHANNEL_ERROR_STATE,
		            "The process has not exited yet.");
		GOTO(failed);
	}
	if (priv->pid_set) {
		g_set_error(error, PKA_CHANNEL_ERROR, PKA_CHANNEL_ERROR_STATE,
		            "The exit status may only be retrieved from processes "
		            "spawned by Perfkit.");
		GOTO(failed);
	}
	*exit_status = priv->exit_status;
	ret = TRUE;
  failed:
	g_mutex_unlock(priv->mutex);
	RETURN(ret);
}

/**
 * pka_channel_add_source:
 * @channel: A #PkaChannel.
 *
 * Adds an existing source to the channel.  If the channel has already been
 * started, the source will be started immediately.  The source must not have
 * been previous added to another channel or this will fail.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_channel_add_source (PkaChannel  *channel, /* IN */
                        PkaContext  *context, /* IN */
                        PkaSource   *source,  /* IN */
                        GError     **error)   /* OUT */
{
	PkaChannelPrivate *priv;
	gboolean ret = FALSE;

	g_return_val_if_fail(PKA_IS_CHANNEL(channel), FALSE);
	g_return_val_if_fail(context != NULL, FALSE);
	g_return_val_if_fail(PKA_IS_SOURCE(source), FALSE);

	ENTRY;
	priv = channel->priv;
	AUTHORIZE_IOCTL(context, MODIFY_CHANNEL, channel, unauthorized);
	g_mutex_lock(priv->mutex);
	if (!pka_source_set_channel(source, channel)) {
		g_set_error(error, PKA_CHANNEL_ERROR, PKA_CHANNEL_ERROR_STATE,
		            "Source already attached to a channel.");
		GOTO(failed);
	}
	/*
	 * TODO: Figure out how to handle circular reference count.
	 */
	g_ptr_array_add(priv->sources, g_object_ref(source));
	ret = TRUE;
  failed:
	g_mutex_unlock(priv->mutex);
	if (ret) {
		g_signal_emit(channel, signals[SOURCE_ADDED], 0, source);
	}
  unauthorized:
	RETURN(ret);
}

/**
 * pka_channel_inferior_exited:
 * @pid: A #GPid.
 * @status: The inferior exit status.
 * @channel: A #PkaChannel.
 *
 * Callback up on the child process exiting.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_channel_inferior_exited (GPid        pid,     /* IN */
                             gint        status,  /* IN */
                             PkaChannel *channel) /* IN */
{
	PkaChannelPrivate *priv;

	g_return_if_fail(PKA_IS_CHANNEL(channel));

	ENTRY;
	INFO(Channel, "Process %d exited with status %d.", (gint)pid, status);
	priv = channel->priv;
	g_mutex_lock(priv->mutex);
	priv->exit_status = status;
	g_mutex_unlock(priv->mutex);
	pka_channel_stop(channel, pka_context_default(), NULL);
	g_object_unref(channel);
	EXIT;
}

/**
 * pka_channel_init_spawn_info_locked:
 * @channel: A #PkaChannel.
 * @spawn_info: A #PkaSpawnInfo.
 * @error: A location for a #GError, or %NULL.
 *
 * Initializes @spawn_info with the current data within @channel.  The sources
 * are notified and allowed to modify the spawn info if needed.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
static gboolean
pka_channel_init_spawn_info_locked (PkaChannel    *channel,    /* IN */
                                    PkaSpawnInfo  *spawn_info, /* IN */
                                    GError       **error)      /* OUT */
{
	PkaChannelPrivate *priv;
	PkaSource *source;
	gboolean ret = TRUE;
	gint i;

	g_return_val_if_fail(PKA_IS_CHANNEL(channel), FALSE);
	g_return_val_if_fail(spawn_info != NULL, FALSE);

	ENTRY;
	priv = channel->priv;
	memset(spawn_info, 0, sizeof(*spawn_info));
	spawn_info->target = g_strdup(priv->target);
	spawn_info->working_dir = g_strdup(priv->working_dir);
	spawn_info->env = g_strdupv(priv->env);
	spawn_info->args = g_strdupv(priv->args);
	spawn_info->pid = priv->pid;
	for (i = 0; i < priv->sources->len; i++) {
		source = g_ptr_array_index(priv->sources, i);
		if (!pka_source_modify_spawn_info(source, spawn_info, error)) {
			ret = FALSE;
			GOTO(error);
		}
	}
  error:
	RETURN(ret);
}

/**
 * pka_channel_destroy_spawn_info:
 * @channel: A #PkaChannel.
 * @spawn_info: A #PkaSpawnInfo.
 *
 * Destroys the previously initialized #PkaSpawnInfo.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_channel_destroy_spawn_info (PkaChannel   *channel,    /* IN */
                                PkaSpawnInfo *spawn_info) /* IN */
{
	ENTRY;
	g_free(spawn_info->target);
	g_free(spawn_info->working_dir);
	g_strfreev(spawn_info->args);
	g_strfreev(spawn_info->env);
	EXIT;
}

/**
 * pka_channel_stdout_cb:
 * @channel: (in): A #PkaChannel.
 *
 * Handle data available to read on the "stdout" #GIOChannel @io.
 *
 * NOTE:
 *   Data is currently read and immediately discarded until we have
 *   determined how we want to pass this on to clients.
 *
 * Returns: %FALSE if the source should be removed.
 * Side effects: None.
 */
static gboolean
pka_channel_stdio_cb (GIOChannel   *io,
                      GIOCondition  cond,
                      gboolean      is_stderr,
                      gpointer      user_data)
{
	PkaChannel *channel = (PkaChannel *)user_data;
	PkaChannelPrivate *priv;
	GError *error = NULL;
	gchar buf[1024];
	gsize n_bytes;

	ENTRY;

	g_return_val_if_fail(PKA_IS_CHANNEL(channel), FALSE);

	priv = channel->priv;

	while (IO_SUCCESS(g_io_channel_read_chars(io, buf, sizeof buf, &n_bytes, &error))) {
		/*
		 * FIXME: We should be sending the data to any subscribing client
		 *        here instead of immediately discarding it.
		 */
	}

	if (error) {
		INFO(Channel, "Error reading %s from inferior: %s",
		     is_stderr ? "stderr" : "stdout",
		     error->message);
		g_clear_error(&error);
	}

	RETURN(priv->state != PKA_CHANNEL_STOPPED);
}

/**
 * pka_channel_stdout_cb:
 * @channel: (in): A #PkaChannel.
 *
 * Handle data available to read on the "stdout" #GIOChannel @io.
 *
 * NOTE:
 *   Data is currently read and immediately discarded until we have
 *   determined how we want to pass this on to clients.
 *
 * Returns: %FALSE if the source should be removed.
 * Side effects: None.
 */
static gboolean
pka_channel_stdout_cb (GIOChannel   *io,
                       GIOCondition  cond,
                       gpointer      user_data)
{
	ENTRY;
	RETURN(pka_channel_stdio_cb (io, cond, FALSE, user_data));
}

/**
 * pka_channel_stderr_cb:
 * @channel: (in): A #PkaChannel.
 *
 * Handle data available to read on the "stderr" #GIOChannel @io.
 *
 * NOTE:
 *   Data is currently read and immediately discarded until we have
 *   determined how we want to pass this on to clients.
 *
 * Returns: %FALSE if the source should be removed.
 * Side effects: None.
 */
static gboolean
pka_channel_stderr_cb (GIOChannel   *io,
                       GIOCondition  cond,
                       gpointer      user_data)
{
	ENTRY;
	RETURN(pka_channel_stdio_cb (io, cond, TRUE, user_data));
}

/**
 * pka_channel_start:
 * @channel: A #PkaChannel
 * @error: A location for a #GError or %NULL
 *
 * Attempts to start the channel.  If the channel was successfully started
 * the attached #PkaSource<!-- -->'s will be notified to start creating
 * samples.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 *
 * Side effects:
 *   The channels state-machine is altered.
 *   Data sources are notified to start sending manifests and samples.
 */
gboolean
pka_channel_start (PkaChannel  *channel, /* IN */
                   PkaContext  *context, /* IN */
                   GError     **error)   /* OUT */
{
	PkaChannelPrivate *priv;
	PkaSpawnInfo spawn_info = { 0 };
	gint standard_input;
	gint standard_output;
	gint standard_error;
	gboolean ret = FALSE;
	GError *local_error = NULL;
	gchar **argv = NULL;
	gchar *argv_str;
	gchar *env_str;
	gint len, i;

	ENTRY;

	g_return_val_if_fail(PKA_IS_CHANNEL(channel), FALSE);

	priv = channel->priv;
	g_mutex_lock(priv->mutex);
	/*
	 * If we are in the stopped state, go ahead and reset everything.
	 */
	if (priv->state == PKA_CHANNEL_STOPPED) {
		g_ptr_array_foreach(priv->sources,
		                    (GFunc)pka_source_notify_reset,
		                    NULL);
		priv->state = PKA_CHANNEL_READY;
	}
	/*
	 * Verify we are in good state.
	 */
	ENSURE_STATE(channel, READY, unlock);
	INFO(Channel, "Starting channel %d on behalf of context %d.",
	     priv->id, pka_context_get_id(context));

	/*
	 * Allow sources to modify the spawn information.
	 */
	if (!pka_channel_init_spawn_info_locked(channel, &spawn_info, error)) {
		GOTO(unlock);
	}

	/*
	 * Spawn the inferior process only if necessary.
	 */
	if ((!spawn_info.pid) && (spawn_info.target)) {
		/*
		 * Build the spawned argv. Copy in arguments if needed.
		 */
		if (spawn_info.args) {
			len = g_strv_length(spawn_info.args);
			argv = g_new0(gchar*,  len + 2);
			for (i = 0; i < len; i++) {
				argv[i + 1] = g_strdup(spawn_info.args[i]);
			}
		} else {
			argv = g_new0(gchar*, 2);
		}

		/*
		 * Set the executable target.
		 */
		argv[0] = g_strdup(spawn_info.target);

		/*
		 * Determine the working directory.
		 */
		if (pka_str_empty0(spawn_info.working_dir)) {
			g_free(spawn_info.working_dir);
			spawn_info.working_dir = g_strdup(g_get_tmp_dir());
		}

		/*
		 * Log the channel arguments.
		 */
		argv_str = spawn_info.args ? g_strjoinv(", ", spawn_info.args)
		                           : g_strdup("");
		env_str = spawn_info.env ? g_strjoinv(", ", spawn_info.env)
		                         : g_strdup("");
		INFO(Channel, "Attempting to spawn channel %d on behalf of context %d.",
		     priv->id, pka_context_get_id(context));
		INFO(Channel, "             Target = %s",
		     spawn_info.target ? spawn_info.target : "");
		INFO(Channel, "          Arguments = [%s]", argv_str);
		INFO(Channel, "        Environment = [%s]", env_str);
		INFO(Channel, "  Working Directory = %s", spawn_info.working_dir);
		g_free(argv_str);
		g_free(env_str);

		/*
		 * Attempt to spawn the process.
		 */
		if (!g_spawn_async_with_pipes(spawn_info.working_dir,
		                              argv,
		                              spawn_info.env,
		                              G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
		                              NULL,
		                              NULL,
		                              &priv->pid,
		                              &standard_input,
		                              &standard_output,
		                              &standard_error,
		                              &local_error))
		{
			priv->state = PKA_CHANNEL_FAILED;
			WARNING(Channel, "Error starting channel %d: %s",
			        priv->id, local_error->message);
			g_propagate_error(error, local_error);
			GOTO(unlock);
		}

		/*
		 * Make our file descriptors non-blocking.
		 */
		fcntl(standard_input, F_SETFL, O_NONBLOCK);
		fcntl(standard_output, F_SETFL, O_NONBLOCK);
		fcntl(standard_error, F_SETFL, O_NONBLOCK);

		/*
		 * Setup GIOChannel's for the particular FDs.
		 */
		priv->stdin = g_io_channel_unix_new(standard_input);
		priv->stdout = g_io_channel_unix_new(standard_output);
		priv->stderr = g_io_channel_unix_new(standard_error);
		g_io_add_watch(priv->stdout, G_IO_IN | G_IO_HUP,
		               pka_channel_stdout_cb, channel);
		g_io_add_watch(priv->stderr, G_IO_IN | G_IO_HUP,
		               pka_channel_stderr_cb, channel);

		/*
		 * Register callback upon child exit.
		 */
		g_child_watch_add(priv->pid,
		                  (GChildWatchFunc)pka_channel_inferior_exited,
		                  g_object_ref(channel));

		spawn_info.pid = priv->pid;
		INFO(Channel, "Channel %d spawned process %d.", priv->id, priv->pid);
	}

	priv->state = PKA_CHANNEL_RUNNING;
	ret = TRUE;

  unlock:
  	g_mutex_unlock(priv->mutex);
  	if (ret) {
		/*
		 * Notify the included data channels of the inferior starting up.
		 */
		g_ptr_array_foreach(priv->sources,
		                    (GFunc)pka_source_notify_started,
		                    &spawn_info);
	}
	pka_channel_destroy_spawn_info(channel, &spawn_info);
	g_strfreev(argv);
	RETURN(ret);
}

/**
 * pka_channel_stop:
 * @channel: A #PkaChannel.
 * @context: A #PkaContext.
 * @error: A location for a #GError or %NULL.
 *
 * Attempts to stop the channel.  If successful, the attached
 * #PkaSource<!-- -->'s will be notified that they should stop sending
 * samples.  After the sources have been notified if @killpid is set
 * the process will be terminated.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 *
 * Side effects:
 *   The state machine is altered.
 *   Data sources are notified to stop.
 *   The process is terminated if @killpid is set.
 */
gboolean
pka_channel_stop (PkaChannel  *channel, /* IN */
                  PkaContext  *context, /* IN */
                  GError     **error)   /* OUT */
{
	PkaChannelPrivate *priv;
	gboolean ret = TRUE;

	ENTRY;

	g_return_val_if_fail(PKA_IS_CHANNEL(channel), FALSE);

	priv = channel->priv;
	g_mutex_lock(priv->mutex);
	switch (priv->state) {
	CASE(PKA_CHANNEL_RUNNING);
	CASE(PKA_CHANNEL_MUTED);
		INFO(Channel, "Stopping channel %d on behalf of context %d.",
		     priv->id, pka_context_get_id(context));
		priv->state = PKA_CHANNEL_STOPPED;

		/*
		 * Notify sources of channel stopping.
		 */
		g_ptr_array_foreach(priv->sources,
		                    (GFunc)pka_source_notify_stopped,
		                    NULL);

		/*
		 * Kill the process only if settings permit and we spawned the
		 * process outself.
		 */
		if ((priv->kill_pid) && (!priv->pid_set) && (priv->pid)) {
			INFO(Channel, "Channel %d killing process %d on behalf of context %d.",
			     priv->id, (gint)priv->pid, pka_context_get_id(context));
			kill(priv->pid, SIGKILL);
		}

		/*
		 * Close the file descriptors.
		 */
		g_io_channel_unref(priv->stdin);
		g_io_channel_unref(priv->stdout);
		g_io_channel_unref(priv->stderr);
		priv->stdin = NULL;
		priv->stdout = NULL;
		priv->stderr = NULL;

		BREAK;
	CASE(PKA_CHANNEL_READY);
		ret = FALSE;
		g_set_error(error, PKA_CHANNEL_ERROR, PKA_CHANNEL_ERROR_STATE,
		            "Cannot stop a channel that has not yet been started.");
		BREAK;
	CASE(PKA_CHANNEL_STOPPED);
	CASE(PKA_CHANNEL_FAILED);
		BREAK;
	default:
		g_warn_if_reached();
	}
	g_mutex_unlock(priv->mutex);
	RETURN(ret);
}

/**
 * pka_channel_mute:
 * @channel: A #PkaChannel.
 * @error: A location for a #GError, or %NULL.
 *
 * Attempts to mute the channel.  If successful, the attached
 * #PkaSource<!-- -->'s are notified to mute as well.  Any samples delivered
 * while muted are silently dropped.  Updated manifests, however, are stored
 * for future delivery when unpausing occurs.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 *
 * Side effects:
 *   The state machine is altered.
 *   Data sources are notified to mute.
 */
gboolean
pka_channel_mute (PkaChannel  *channel, /* IN */
                  PkaContext  *context, /* IN */
                  GError     **error)   /* IN */
{
	PkaChannelPrivate *priv;
	gboolean ret = FALSE;

	ENTRY;

	g_return_val_if_fail(PKA_IS_CHANNEL(channel), FALSE);

	priv = channel->priv;
	g_mutex_lock(priv->mutex);
	switch (priv->state) {
	CASE(PKA_CHANNEL_READY);
		g_set_error(error, PKA_CHANNEL_ERROR, PKA_CHANNEL_ERROR_STATE,
		            _("Cannot mute channel; not yet started."));
		BREAK;
	CASE(PKA_CHANNEL_STOPPED);
		g_set_error(error, PKA_CHANNEL_ERROR, PKA_CHANNEL_ERROR_STATE,
		            _("Cannot mute channel; channel stopped."));
		BREAK;
	CASE(PKA_CHANNEL_FAILED);
		g_set_error(error, PKA_CHANNEL_ERROR, PKA_CHANNEL_ERROR_STATE,
		            _("Cannot mute channel; channel failed to start."));
		BREAK;
	CASE(PKA_CHANNEL_MUTED);
		ret = TRUE;
		BREAK;
	CASE(PKA_CHANNEL_RUNNING);
		priv->state = PKA_CHANNEL_MUTED;
		INFO(Channel, "Muting channel %d on behalf of context %d.",
		     priv->id, pka_context_get_id(context));

		/*
		 * Notify sources that we have muted.
		 */
		g_ptr_array_foreach(priv->sources,
		                    (GFunc)pka_source_notify_muted,
		                    NULL);
		ret = TRUE;
		BREAK;
	default:
		g_warn_if_reached();
	}
	g_mutex_unlock(priv->mutex);
	RETURN(ret);
}

/**
 * pka_channel_unmute:
 * @channel: A #PkaChannel.
 * @error: A location for a #GError or %NULL.
 *
 * Attempts to unmute a #PkaChannel.  If successful; the
 * #PkaSource<!-- -->'s will be notified to unmute and continue sending
 * samples.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 *
 * Side effects:
 *   The state machine is altered.
 *   Data sources are notified to resume.
 */
gboolean
pka_channel_unmute (PkaChannel  *channel, /* IN */
                    PkaContext  *context, /* IN */
                    GError     **error)   /* OUT */
{
	PkaChannelPrivate *priv;
	gboolean ret = FALSE;

	ENTRY;

	g_return_val_if_fail(PKA_IS_CHANNEL(channel), FALSE);

	priv = channel->priv;

	g_mutex_lock(priv->mutex);
	switch (priv->state) {
	CASE(PKA_CHANNEL_MUTED);
		INFO(Channel, "Unpausing channel %d on behalf of context %d.",
		     priv->id, pka_context_get_id(context));
		priv->state = PKA_CHANNEL_RUNNING;

		/*
		 * Enable sources.
		 */
		g_ptr_array_foreach(priv->sources,
		                    (GFunc)pka_source_notify_unmuted,
		                    NULL);

		ret = TRUE;
		BREAK;
	CASE(PKA_CHANNEL_READY);
	CASE(PKA_CHANNEL_RUNNING);
	CASE(PKA_CHANNEL_STOPPED);
	CASE(PKA_CHANNEL_FAILED);
		g_set_error(error, PKA_CHANNEL_ERROR, PKA_CHANNEL_ERROR_STATE,
		            "Channel %d is not muted.", priv->id);
		BREAK;
	default:
		g_warn_if_reached();
	}
	g_mutex_unlock(priv->mutex);

	RETURN(ret);
}

/**
 * pka_channel_get_state:
 * @channel: A #PkaChannel.
 *
 * Retrieves the current state of the channel.
 *
 * Returns: The channel state.
 * Side effects: None.
 */
PkaChannelState
pka_channel_get_state (PkaChannel *channel) /* IN */
{
	PkaChannelPrivate *priv;
	PkaChannelState state;

	ENTRY;

	g_return_val_if_fail(PKA_IS_CHANNEL(channel), 0);

	priv = channel->priv;

	g_mutex_lock(priv->mutex);
	state = priv->state;
	g_mutex_unlock(priv->mutex);

	RETURN(state);
}

/**
 * pka_channel_get_sources:
 * @channel: A #PkaChannel.
 *
 * Retrieves the list of sources for the channel.
 *
 * Returns: A #GList containing the sources. The list and elemnts are owned
 *   by the caller.  The elements should be freed with g_object_unref() and
 *   the list freed with g_list_free().
 * Side effects: None.
 */
GList*
pka_channel_get_sources (PkaChannel *channel) /* IN */
{
	PkaChannelPrivate *priv;
	PkaSource *source;
	GList *sources = NULL;
	gint i;

	ENTRY;

	g_return_val_if_fail(PKA_IS_CHANNEL(channel), NULL);

	priv = channel->priv;

	g_mutex_lock(priv->mutex);
	for (i = 0; i < priv->sources->len; i++) {
		source = g_ptr_array_index(priv->sources, i);
		sources = g_list_prepend(sources, g_object_ref(source));
	}
	g_mutex_unlock(priv->mutex);

	RETURN(sources);
}

/**
 * pka_channel_get_created_at:
 * @channel: (in): A #PkaChannel.
 * @tv: (out): A #GTimeVal.
 *
 * Retrieves the time that @channel was created and stores it in @tv.
 *
 * Returns: None.
 * Side effects: @tv is set.
 */
void
pka_channel_get_created_at (PkaChannel *channel,
                            GTimeVal   *tv)
{
	PkaChannelPrivate *priv;

	ENTRY;

	g_return_if_fail(PKA_IS_CHANNEL(channel));

	priv = channel->priv;
	*tv = priv->created_at;

	EXIT;
}

/**
 * pka_channel_compare:
 * @a: (type Pka.Channel): A #PkaChannel.
 * @b: (type Pka.Channel): A #PkaChannel.
 *
 * Standard qsort() style compare function.
 *
 * Returns: Less than zero if a precedes b. Greater than zero if b precedes a.
 *   Otherwise, zero.
 * Side effects: None.
 */
gint
pka_channel_compare (gconstpointer a,
                     gconstpointer b)
{
	return ((PkaChannel *)a)->priv->id - ((PkaChannel *)b)->priv->id;
}

/**
 * pka_channel_finalize:
 * @object: A #PkaChannel.
 *
 * Finalizes a #PkaChannel and frees resources.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_channel_finalize (GObject *object)
{
	PkaChannelPrivate *priv = PKA_CHANNEL(object)->priv;

	ENTRY;

	g_free(priv->target);
	g_free(priv->working_dir);
	g_strfreev(priv->args);
	g_strfreev(priv->env);
	g_ptr_array_foreach(priv->sources, (GFunc)g_object_unref, NULL);
	g_ptr_array_free(priv->sources, TRUE);
	g_mutex_free(priv->mutex);
	G_OBJECT_CLASS(pka_channel_parent_class)->finalize(object);

	EXIT;
}

/**
 * pka_channel_class_init:
 * @klass: A #PkaChannelClass.
 *
 * Initializes the #PkaChannelClass.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_channel_class_init (PkaChannelClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pka_channel_finalize;
	g_type_class_add_private(object_class, sizeof(PkaChannelPrivate));

	signals[SOURCE_ADDED] = g_signal_new("source-added",
	                                     PKA_TYPE_CHANNEL,
	                                     G_SIGNAL_RUN_FIRST,
	                                     0, NULL, NULL,
	                                     g_cclosure_marshal_VOID__OBJECT,
	                                     G_TYPE_NONE, 1, PKA_TYPE_SOURCE);

	signals[SOURCE_REMOVED] = g_signal_new("source-removed",
	                                       PKA_TYPE_CHANNEL,
	                                       G_SIGNAL_RUN_FIRST,
	                                       0, NULL, NULL,
	                                       g_cclosure_marshal_VOID__OBJECT,
	                                       G_TYPE_NONE, 1, PKA_TYPE_SOURCE);
}

/**
 * pka_channel_init:
 * @channel: A #PkaChannel.
 *
 * Initializes a newly created #PkaChannel instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_channel_init (PkaChannel *channel)
{
	static gint id_seq = 0;
	PkaChannelPrivate *priv;

	ENTRY;

	channel->priv = G_TYPE_INSTANCE_GET_PRIVATE(channel, PKA_TYPE_CHANNEL,
	                                            PkaChannelPrivate);
	priv = channel->priv;

	priv->sources = g_ptr_array_new();
	priv->mutex = g_mutex_new();
	priv->id = g_atomic_int_exchange_and_add(&id_seq, 1);
	priv->state = PKA_CHANNEL_READY;
	priv->kill_pid = TRUE;
	priv->working_dir = g_strdup(g_get_tmp_dir());
	g_get_current_time(&priv->created_at);

	EXIT;
}

/**
 * pka_channel_error_quark:
 *
 * Retrieves the #PkaChannel error domain #GQuark.
 *
 * Returns: A #GQuark.
 * Side effects: None.
 */
GQuark
pka_channel_error_quark (void)
{
	return g_quark_from_static_string("pka-channel-error-quark");
}

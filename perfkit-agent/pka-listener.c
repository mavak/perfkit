/* pka-listener.c
 *
 * Copyright 2010 Christian Hergert <chris@dronelabs.com>
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

#ifdef G_LOG_DOMAIN
#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "Listener"

#include "pka-channel.h"
#include "pka-listener.h"
#include "pka-listener-closures.h"
#include "pka-log.h"
#include "pka-manager.h"
#include "pka-source.h"
#include "pka-subscription.h"
#include "pka-version.h"

/**
 * SECTION:pka-listener
 * @title: PkaListener
 * @short_description: 
 *
 * Section overview.
 */

#define DEFAULT_CONTEXT (pka_context_default())

#define RESULT_IS_VALID(_t)                                         \
    g_simple_async_result_is_valid(                                 \
            G_ASYNC_RESULT((result)),                               \
            G_OBJECT((listener)),                                   \
            pka_listener_##_t##_async)

#define GET_RESULT_POINTER(_t, _r)                                  \
    ((_t *)g_simple_async_result_get_op_res_gpointer(               \
        G_SIMPLE_ASYNC_RESULT((_r))))

#define GET_RESULT_INT(_r)                                          \
    ((gint)g_simple_async_result_get_op_res_gssize(                 \
        G_SIMPLE_ASYNC_RESULT((_r))))

G_DEFINE_ABSTRACT_TYPE(PkaListener, pka_listener, G_TYPE_OBJECT)

/**
 * pka_listener_listen:
 * @listener: A #PkaListener.
 * @error: A location for a #GError, or %NULL.
 *
 * Starts the listener so that it may start accepting connections.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
pka_listener_listen (PkaListener  *listener, /* IN */
                     GError      **error)    /* OUT */
{
	gboolean ret;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);

	ENTRY;
	ret = PKA_LISTENER_GET_CLASS(listener)->listen(listener, error);
	RETURN(ret);
}

/**
 * pka_listener_close:
 * @listener: A #PkaListener.
 *
 * Immediately closes the listener connections.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_close (PkaListener *listener) /* IN */
{
	g_return_if_fail(PKA_IS_LISTENER(listener));
	PKA_LISTENER_GET_CLASS(listener)->close(listener);
}

#if 0
static void
pka_listener_channel_mute_cb (GObject      *listener,    /* IN */
                              GAsyncResult *result,      /* IN */
                              gpointer      user_data)   /* IN */
{
	GSimpleAsyncResult *real_result;

	g_return_if_fail(PKA_IS_LISTENER(listener));
	g_return_if_fail(RESULT_IS_VALID(channel_mute));

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	g_simple_async_result_set_op_res_gpointer(real_result, result);
	g_simple_async_result_complete(real_result);
	EXIT;
}
#endif

/**
 * pk_connection_channel_mute_async:
 * @connection: A #PkConnection.
 * @channel: A #gint.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "channel_mute_async" RPC.  @callback
 * MUST call pka_listener_channel_mute_finish().
 *
 * Notifies @channel to silently drop manifest and sample updates until
 * unmute() is called.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_channel_mute_async (PkaListener           *listener,    /* IN */
                                 gint                   channel,     /* IN */
                                 GCancellable          *cancellable, /* IN */
                                 GAsyncReadyCallback    callback,    /* IN */
                                 gpointer               user_data)   /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_channel_mute_async);
// TEMP TO TEST RPC RESULTS
	g_simple_async_result_complete(result);
	g_object_unref(result);
#if 0
	pka_channel_mute_async(instance,
	                       NULL,
	                       pka_listener_channel_mute_cb,
	                       result);
#endif
	EXIT;
}

/**
 * pk_connection_channel_mute_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "channel_mute_finish" RPC.
 *
 * Notifies @channel to silently drop manifest and sample updates until
 * unmute() is called.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_channel_mute_finish (PkaListener    *listener, /* IN */
                                  GAsyncResult   *result,   /* IN */
                                  GError        **error)    /* OUT */
{
	ENTRY;
// TEMP TO TEST RPC RESULTS
	RETURN(TRUE);
#if 0
	GSimpleAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	ret = pka_channel_mute_finish(instance,
	                              real_result,
	                              error);
	RETURN(ret);
#endif
}

/**
 * pk_connection_channel_get_args_async:
 * @connection: A #PkConnection.
 * @channel: A #gint.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "channel_get_args_async" RPC.  @callback
 * MUST call pka_listener_channel_get_args_finish().
 *
 * Retrieves the arguments for target.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_channel_get_args_async (PkaListener           *listener,    /* IN */
                                     gint                   channel,     /* IN */
                                     GCancellable          *cancellable, /* IN */
                                     GAsyncReadyCallback    callback,    /* IN */
                                     gpointer               user_data)   /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_channel_get_args_async);
	g_simple_async_result_set_op_res_gssize(result, channel);
	g_simple_async_result_complete(result);
	g_object_unref(result);
	EXIT;
}

/**
 * pk_connection_channel_get_args_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @args: A #gchar.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "channel_get_args_finish" RPC.
 *
 * Retrieves the arguments for target.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_channel_get_args_finish (PkaListener    *listener, /* IN */
                                      GAsyncResult   *result,   /* IN */
                                      gchar        ***args,     /* OUT */
                                      GError        **error)    /* OUT */
{
	PkaChannel *channel;
	gboolean ret = FALSE;
	gint channel_id;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(channel_get_args), FALSE);
	g_return_val_if_fail(args != NULL, FALSE);

	ENTRY;
	channel_id = GET_RESULT_INT(result);
	if (!pka_manager_find_channel(DEFAULT_CONTEXT, channel_id, &channel, error)) {
		GOTO(failed);
	}
	*args = pka_channel_get_args(channel);
	g_object_unref(channel);
	ret = TRUE;
  failed:
	RETURN(ret);
}

/**
 * pk_connection_channel_get_env_async:
 * @connection: A #PkConnection.
 * @channel: A #gint.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "channel_get_env_async" RPC.  @callback
 * MUST call pka_listener_channel_get_env_finish().
 *
 * Retrieves the environment for spawning the target process.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_channel_get_env_async (PkaListener           *listener,    /* IN */
                                    gint                   channel,     /* IN */
                                    GCancellable          *cancellable, /* IN */
                                    GAsyncReadyCallback    callback,    /* IN */
                                    gpointer               user_data)   /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_channel_get_env_async);
	g_simple_async_result_set_op_res_gssize(result, channel);
	g_simple_async_result_complete(result);
	g_object_unref(result);
	EXIT;
}

/**
 * pk_connection_channel_get_env_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @env: A #gchar.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "channel_get_env_finish" RPC.
 *
 * Retrieves the environment for spawning the target process.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_channel_get_env_finish (PkaListener    *listener, /* IN */
                                     GAsyncResult   *result,   /* IN */
                                     gchar        ***env,      /* OUT */
                                     GError        **error)    /* OUT */
{
	PkaChannel *channel;
	gboolean ret = FALSE;
	gint channel_id;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(channel_get_env), FALSE);
	g_return_val_if_fail(env != NULL, FALSE);

	ENTRY;
	channel_id = GET_RESULT_INT(result);
	if (!pka_manager_find_channel(DEFAULT_CONTEXT, channel_id, &channel, error)) {
		GOTO(failed);
	}
	*env = pka_channel_get_env(channel);
	g_object_unref(channel);
	ret = TRUE;
  failed:
	RETURN(ret);
}

/**
 * pk_connection_channel_get_exit_status_async:
 * @connection: A #PkConnection.
 * @channel: A #gint.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "channel_get_exit_status_async" RPC.  @callback
 * MUST call pka_listener_channel_get_exit_status_finish().
 *
 * Retrieves the exit status of the process.  This is only set after the
 * process has exited.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_channel_get_exit_status_async (PkaListener           *listener,    /* IN */
                                            gint                   channel,     /* IN */
                                            GCancellable          *cancellable, /* IN */
                                            GAsyncReadyCallback    callback,    /* IN */
                                            gpointer               user_data)   /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_channel_get_exit_status_async);
	g_simple_async_result_set_op_res_gssize(result, channel);
	g_simple_async_result_complete(result);
	g_object_unref(result);
	EXIT;
}

/**
 * pk_connection_channel_get_exit_status_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @exit_status: A #gint.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "channel_get_exit_status_finish" RPC.
 *
 * Retrieves the exit status of the process.  This is only set after the
 * process has exited.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_channel_get_exit_status_finish (PkaListener    *listener,    /* IN */
                                             GAsyncResult   *result,      /* IN */
                                             gint           *exit_status, /* OUT */
                                             GError        **error)       /* OUT */
{
	PkaChannel *channel;
	gboolean ret = FALSE;
	gint channel_id;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(channel_get_exit_status), FALSE);
	g_return_val_if_fail(exit_status != NULL, FALSE);

	ENTRY;
	channel_id = GET_RESULT_INT(result);
	if (!pka_manager_find_channel(DEFAULT_CONTEXT, channel_id, &channel, error)) {
		GOTO(failed);
	}
	if (!pka_channel_get_exit_status(channel, exit_status, error)) {
		g_object_unref(channel);
		GOTO(failed);
	}
	ret = TRUE;
	g_object_unref(channel);
  failed:
	RETURN(ret);
}

/**
 * pk_connection_channel_get_kill_pid_async:
 * @connection: A #PkConnection.
 * @channel: A #gint.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "channel_get_kill_pid_async" RPC.  @callback
 * MUST call pka_listener_channel_get_kill_pid_finish().
 *
 * Retrieves if the process should be killed when the channel is stopped.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_channel_get_kill_pid_async (PkaListener           *listener,    /* IN */
                                         gint                   channel,     /* IN */
                                         GCancellable          *cancellable, /* IN */
                                         GAsyncReadyCallback    callback,    /* IN */
                                         gpointer               user_data)   /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_channel_get_kill_pid_async);
	g_simple_async_result_set_op_res_gssize(result, channel);
	g_simple_async_result_complete(result);
	g_object_unref(result);
	EXIT;
}

/**
 * pk_connection_channel_get_kill_pid_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @kill_pid: A #gboolean.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "channel_get_kill_pid_finish" RPC.
 *
 * Retrieves if the process should be killed when the channel is stopped.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_channel_get_kill_pid_finish (PkaListener    *listener, /* IN */
                                          GAsyncResult   *result,   /* IN */
                                          gboolean       *kill_pid, /* OUT */
                                          GError        **error)    /* OUT */
{
	PkaChannel *channel;
	gboolean ret = FALSE;
	gint channel_id;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(channel_get_kill_pid), FALSE);
	g_return_val_if_fail(kill_pid != NULL, FALSE);

	ENTRY;
	channel_id = GET_RESULT_INT(result);
	if (!pka_manager_find_channel(DEFAULT_CONTEXT, channel_id, &channel, error)) {
		GOTO(failed);
	}
	*kill_pid = pka_channel_get_kill_pid(channel);
	g_object_unref(channel);
	ret = TRUE;
  failed:
	RETURN(ret);
}

/**
 * pk_connection_channel_get_pid_async:
 * @connection: A #PkConnection.
 * @channel: A #gint.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "channel_get_pid_async" RPC.  @callback
 * MUST call pka_listener_channel_get_pid_finish().
 *
 * Retrieves the process pid of the target process.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_channel_get_pid_async (PkaListener           *listener,    /* IN */
                                    gint                   channel,     /* IN */
                                    GCancellable          *cancellable, /* IN */
                                    GAsyncReadyCallback    callback,    /* IN */
                                    gpointer               user_data)   /* IN */
{
	ChannelGetPidCall *call;
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_channel_get_pid_async);
	call = ChannelGetPidCall_Create();
	call->channel = channel;
	g_simple_async_result_set_op_res_gpointer(
			result, call, (GDestroyNotify)ChannelGetPidCall_Free);
	g_simple_async_result_complete(result);
	g_object_unref(result);
	EXIT;
}

/**
 * pk_connection_channel_get_pid_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @pid: A #gint.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "channel_get_pid_finish" RPC.
 *
 * Retrieves the process pid of the target process.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_channel_get_pid_finish (PkaListener    *listener, /* IN */
                                     GAsyncResult   *result,   /* IN */
                                     gint           *pid,      /* OUT */
                                     GError        **error)    /* OUT */
{
	PkaChannel *channel;
	ChannelGetPidCall *call;
	gboolean ret = FALSE;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(channel_get_pid), FALSE);

	ENTRY;
	call = GET_RESULT_POINTER(ChannelGetPidCall, result);
	if (!pka_manager_find_channel(DEFAULT_CONTEXT, call->channel,
	                              &channel, error)) {
		GOTO(failed);
	}
	*pid = pka_channel_get_pid(channel);
	ret = TRUE;
	g_object_unref(channel);
  failed:
  	//g_object_unref(result);
	RETURN(ret);
}

#if 0
static void
pka_listener_channel_get_pid_set_cb (GObject      *listener,    /* IN */
                                     GAsyncResult *result,      /* IN */
                                     gpointer      user_data)   /* IN */
{
	GSimpleAsyncResult *real_result;

	g_return_if_fail(PKA_IS_LISTENER(listener));
	g_return_if_fail(RESULT_IS_VALID(channel_get_pid_set));

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	g_simple_async_result_set_op_res_gpointer(real_result, result);
	g_simple_async_result_complete(real_result);
	EXIT;
}
#endif

/**
 * pk_connection_channel_get_pid_set_async:
 * @connection: A #PkConnection.
 * @channel: A #gint.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "channel_get_pid_set_async" RPC.  @callback
 * MUST call pka_listener_channel_get_pid_set_finish().
 *
 * Retrieves if the "pid" property was set manually.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_channel_get_pid_set_async (PkaListener           *listener,    /* IN */
                                        gint                   channel,     /* IN */
                                        GCancellable          *cancellable, /* IN */
                                        GAsyncReadyCallback    callback,    /* IN */
                                        gpointer               user_data)   /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_channel_get_pid_set_async);
// TEMP TO TEST RPC RESULTS
	g_simple_async_result_complete(result);
	g_object_unref(result);
#if 0
	pka_channel_get_pid_set_async(instance,
	                              NULL,
	                              pka_listener_channel_get_pid_set_cb,
	                              result);
#endif
	EXIT;
}

/**
 * pk_connection_channel_get_pid_set_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @pid_set: A #gboolean.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "channel_get_pid_set_finish" RPC.
 *
 * Retrieves if the "pid" property was set manually.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_channel_get_pid_set_finish (PkaListener    *listener, /* IN */
                                         GAsyncResult   *result,   /* IN */
                                         gboolean       *pid_set,  /* OUT */
                                         GError        **error)    /* OUT */
{
	ENTRY;
// TEMP TO TEST RPC RESULTS
	RETURN(TRUE);
#if 0
	GSimpleAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	ret = pka_channel_get_pid_set_finish(instance,
	                                     real_result,
	                                     pid_set,
	                                     error);
	RETURN(ret);
#endif
}

#if 0
static void
pka_listener_channel_get_sources_cb (GObject      *listener,    /* IN */
                                     GAsyncResult *result,      /* IN */
                                     gpointer      user_data)   /* IN */
{
	GSimpleAsyncResult *real_result;

	g_return_if_fail(PKA_IS_LISTENER(listener));
	g_return_if_fail(RESULT_IS_VALID(channel_get_sources));

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	g_simple_async_result_set_op_res_gpointer(real_result, result);
	g_simple_async_result_complete(real_result);
	EXIT;
}
#endif

/**
 * pk_connection_channel_get_sources_async:
 * @connection: A #PkConnection.
 * @channel: A #gint.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "channel_get_sources_async" RPC.  @callback
 * MUST call pka_listener_channel_get_sources_finish().
 *
 * Retrieves the available sources.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_channel_get_sources_async (PkaListener           *listener,    /* IN */
                                        gint                   channel,     /* IN */
                                        GCancellable          *cancellable, /* IN */
                                        GAsyncReadyCallback    callback,    /* IN */
                                        gpointer               user_data)   /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_channel_get_sources_async);
// TEMP TO TEST RPC RESULTS
	g_simple_async_result_complete(result);
	g_object_unref(result);
#if 0
	pka_channel_get_sources_async(instance,
	                              NULL,
	                              pka_listener_channel_get_sources_cb,
	                              result);
#endif
	EXIT;
}

/**
 * pk_connection_channel_get_sources_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @sources: A #gint.
 * @sources_len: A #gsize.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "channel_get_sources_finish" RPC.
 *
 * Retrieves the available sources.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_channel_get_sources_finish (PkaListener    *listener,    /* IN */
                                         GAsyncResult   *result,      /* IN */
                                         gint          **sources,     /* OUT */
                                         gsize          *sources_len, /* OUT */
                                         GError        **error)       /* OUT */
{
	ENTRY;
// TEMP TO TEST RPC RESULTS
	RETURN(TRUE);
#if 0
	GSimpleAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	ret = pka_channel_get_sources_finish(instance,
	                                     real_result,
	                                     sources,
	                                     sources_len,
	                                     error);
	RETURN(ret);
#endif
}

#if 0
static void
pka_listener_channel_get_state_cb (GObject      *listener,    /* IN */
                                   GAsyncResult *result,      /* IN */
                                   gpointer      user_data)   /* IN */
{
	GSimpleAsyncResult *real_result;

	g_return_if_fail(PKA_IS_LISTENER(listener));
	g_return_if_fail(RESULT_IS_VALID(channel_get_state));

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	g_simple_async_result_set_op_res_gpointer(real_result, result);
	g_simple_async_result_complete(real_result);
	EXIT;
}
#endif

/**
 * pk_connection_channel_get_state_async:
 * @connection: A #PkConnection.
 * @channel: A #gint.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "channel_get_state_async" RPC.  @callback
 * MUST call pka_listener_channel_get_state_finish().
 *
 * Retrieves the current state of the channel.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_channel_get_state_async (PkaListener           *listener,    /* IN */
                                      gint                   channel,     /* IN */
                                      GCancellable          *cancellable, /* IN */
                                      GAsyncReadyCallback    callback,    /* IN */
                                      gpointer               user_data)   /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_channel_get_state_async);
// TEMP TO TEST RPC RESULTS
	g_simple_async_result_complete(result);
	g_object_unref(result);
#if 0
	pka_channel_get_state_async(instance,
	                            NULL,
	                            pka_listener_channel_get_state_cb,
	                            result);
#endif
	EXIT;
}

/**
 * pk_connection_channel_get_state_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @state: A #gint.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "channel_get_state_finish" RPC.
 *
 * Retrieves the current state of the channel.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_channel_get_state_finish (PkaListener    *listener, /* IN */
                                       GAsyncResult   *result,   /* IN */
                                       gint           *state,    /* OUT */
                                       GError        **error)    /* OUT */
{
	ENTRY;
// TEMP TO TEST RPC RESULTS
	RETURN(TRUE);
#if 0
	GSimpleAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	ret = pka_channel_get_state_finish(instance,
	                                   real_result,
	                                   state,
	                                   error);
	RETURN(ret);
#endif
}

#if 0
static void
pka_listener_channel_get_target_cb (GObject      *listener,    /* IN */
                                    GAsyncResult *result,      /* IN */
                                    gpointer      user_data)   /* IN */
{
	GSimpleAsyncResult *real_result;

	g_return_if_fail(PKA_IS_LISTENER(listener));
	g_return_if_fail(RESULT_IS_VALID(channel_get_target));

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	g_simple_async_result_set_op_res_gpointer(real_result, result);
	g_simple_async_result_complete(real_result);
	EXIT;
}
#endif

/**
 * pk_connection_channel_get_target_async:
 * @connection: A #PkConnection.
 * @channel: A #gint.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "channel_get_target_async" RPC.  @callback
 * MUST call pka_listener_channel_get_target_finish().
 *
 * Retrieves the channels target.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_channel_get_target_async (PkaListener           *listener,    /* IN */
                                       gint                   channel,     /* IN */
                                       GCancellable          *cancellable, /* IN */
                                       GAsyncReadyCallback    callback,    /* IN */
                                       gpointer               user_data)   /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_channel_get_target_async);
// TEMP TO TEST RPC RESULTS
	g_simple_async_result_complete(result);
	g_object_unref(result);
#if 0
	pka_channel_get_target_async(instance,
	                             NULL,
	                             pka_listener_channel_get_target_cb,
	                             result);
#endif
	EXIT;
}

/**
 * pk_connection_channel_get_target_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @target: A #gchar.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "channel_get_target_finish" RPC.
 *
 * Retrieves the channels target.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_channel_get_target_finish (PkaListener    *listener, /* IN */
                                        GAsyncResult   *result,   /* IN */
                                        gchar         **target,   /* OUT */
                                        GError        **error)    /* OUT */
{
	ENTRY;
// TEMP TO TEST RPC RESULTS
	RETURN(TRUE);
#if 0
	GSimpleAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	ret = pka_channel_get_target_finish(instance,
	                                    real_result,
	                                    target,
	                                    error);
	RETURN(ret);
#endif
}

#if 0
static void
pka_listener_channel_get_working_dir_cb (GObject      *listener,    /* IN */
                                         GAsyncResult *result,      /* IN */
                                         gpointer      user_data)   /* IN */
{
	GSimpleAsyncResult *real_result;

	g_return_if_fail(PKA_IS_LISTENER(listener));
	g_return_if_fail(RESULT_IS_VALID(channel_get_working_dir));

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	g_simple_async_result_set_op_res_gpointer(real_result, result);
	g_simple_async_result_complete(real_result);
	EXIT;
}
#endif

/**
 * pk_connection_channel_get_working_dir_async:
 * @connection: A #PkConnection.
 * @channel: A #gint.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "channel_get_working_dir_async" RPC.  @callback
 * MUST call pka_listener_channel_get_working_dir_finish().
 *
 * Retrieves the working directory of the target.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_channel_get_working_dir_async (PkaListener           *listener,    /* IN */
                                            gint                   channel,     /* IN */
                                            GCancellable          *cancellable, /* IN */
                                            GAsyncReadyCallback    callback,    /* IN */
                                            gpointer               user_data)   /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_channel_get_working_dir_async);
// TEMP TO TEST RPC RESULTS
	g_simple_async_result_complete(result);
	g_object_unref(result);
#if 0
	pka_channel_get_working_dir_async(instance,
	                                  NULL,
	                                  pka_listener_channel_get_working_dir_cb,
	                                  result);
#endif
	EXIT;
}

/**
 * pk_connection_channel_get_working_dir_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @working_dir: A #gchar.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "channel_get_working_dir_finish" RPC.
 *
 * Retrieves the working directory of the target.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_channel_get_working_dir_finish (PkaListener    *listener,    /* IN */
                                             GAsyncResult   *result,      /* IN */
                                             gchar         **working_dir, /* OUT */
                                             GError        **error)       /* OUT */
{
	ENTRY;
// TEMP TO TEST RPC RESULTS
	RETURN(TRUE);
#if 0
	GSimpleAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	ret = pka_channel_get_working_dir_finish(instance,
	                                         real_result,
	                                         working_dir,
	                                         error);
	RETURN(ret);
#endif
}

/**
 * pk_connection_channel_set_args_async:
 * @connection: A #PkConnection.
 * @channel: A #gint.
 * @args: A #gchar.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "channel_set_args_async" RPC.  @callback
 * MUST call pka_listener_channel_set_args_finish().
 *
 * Sets the targets arguments.  This may only be set before the channel
 * has started.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_channel_set_args_async (PkaListener           *listener,    /* IN */
                                     gint                   channel,     /* IN */
                                     gchar                **args,        /* IN */
                                     GCancellable          *cancellable, /* IN */
                                     GAsyncReadyCallback    callback,    /* IN */
                                     gpointer               user_data)   /* IN */
{
	GSimpleAsyncResult *result;
	ChannelSetArgsCall *call;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_channel_set_args_async);
	call = ChannelSetArgsCall_Create();
	call->channel = channel;
	call->args = g_strdupv(args);
	g_simple_async_result_set_op_res_gpointer(
			result, call, (GDestroyNotify)ChannelSetArgsCall_Free);
	g_simple_async_result_complete(result);
	g_object_unref(result);
	EXIT;
}

/**
 * pk_connection_channel_set_args_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "channel_set_args_finish" RPC.
 *
 * Sets the targets arguments.  This may only be set before the channel
 * has started.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_channel_set_args_finish (PkaListener    *listener, /* IN */
                                      GAsyncResult   *result,   /* IN */
                                      GError        **error)    /* OUT */
{
	PkaChannel *channel;
	ChannelSetArgsCall *call;
	gboolean ret = FALSE;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(channel_set_args), FALSE);

	ENTRY;
	call = GET_RESULT_POINTER(ChannelSetArgsCall, result);
	if (!pka_manager_find_channel(DEFAULT_CONTEXT, call->channel, &channel, error)) {
		GOTO(failed);
	}
	ret = pka_channel_set_args(channel, DEFAULT_CONTEXT, call->args, error);
	g_object_unref(channel);
  failed:
	RETURN(ret);
}

/**
 * pk_connection_channel_set_env_async:
 * @connection: A #PkConnection.
 * @channel: A #gint.
 * @env: A #gchar.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "channel_set_env_async" RPC.  @callback
 * MUST call pka_listener_channel_set_env_finish().
 *
 * Sets the environment of the target process.  This may only be set before
 * the channel has started.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_channel_set_env_async (PkaListener           *listener,    /* IN */
                                    gint                   channel,     /* IN */
                                    gchar                **env,         /* IN */
                                    GCancellable          *cancellable, /* IN */
                                    GAsyncReadyCallback    callback,    /* IN */
                                    gpointer               user_data)   /* IN */
{
	ChannelSetEnvCall *call;
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_channel_set_env_async);
	call = ChannelSetEnvCall_Create();
	call->channel = channel;
	call->env = g_strdupv(env);
	g_simple_async_result_set_op_res_gpointer(
			result, call, (GDestroyNotify)ChannelSetEnvCall_Free);
	g_simple_async_result_complete(result);
	g_object_unref(result);
	EXIT;
}

/**
 * pk_connection_channel_set_env_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "channel_set_env_finish" RPC.
 *
 * Sets the environment of the target process.  This may only be set before
 * the channel has started.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_channel_set_env_finish (PkaListener    *listener, /* IN */
                                     GAsyncResult   *result,   /* IN */
                                     GError        **error)    /* OUT */
{
	PkaChannel *channel;
	ChannelSetEnvCall *call;
	gboolean ret = FALSE;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(channel_set_env), FALSE);

	ENTRY;
	call = GET_RESULT_POINTER(ChannelSetEnvCall, result);
	if (!pka_manager_find_channel(DEFAULT_CONTEXT, call->channel, &channel, error)) {
		GOTO(failed);
	}
	ret = pka_channel_set_env(channel, DEFAULT_CONTEXT, call->env, error);
	g_object_unref(channel);
  failed:
	RETURN(ret);
}

/**
 * pk_connection_channel_set_kill_pid_async:
 * @connection: A #PkConnection.
 * @channel: A #gint.
 * @kill_pid: A #gboolean.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "channel_set_kill_pid_async" RPC.  @callback
 * MUST call pka_listener_channel_set_kill_pid_finish().
 *
 * Sets if the process should be killed when the channel is stopped.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_channel_set_kill_pid_async (PkaListener           *listener,    /* IN */
                                         gint                   channel,     /* IN */
                                         gboolean               kill_pid,    /* IN */
                                         GCancellable          *cancellable, /* IN */
                                         GAsyncReadyCallback    callback,    /* IN */
                                         gpointer               user_data)   /* IN */
{
	GSimpleAsyncResult *result;
	ChannelSetKillPidCall *call;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_channel_set_kill_pid_async);
	call = ChannelSetKillPidCall_Create();
	call->channel = channel;
	call->kill_pid = kill_pid;
	g_simple_async_result_set_op_res_gpointer(
			result, call, (GDestroyNotify)ChannelSetKillPidCall_Free);
	g_simple_async_result_complete(result);
	g_object_unref(result);
	EXIT;
}

/**
 * pk_connection_channel_set_kill_pid_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "channel_set_kill_pid_finish" RPC.
 *
 * Sets if the process should be killed when the channel is stopped.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_channel_set_kill_pid_finish (PkaListener    *listener, /* IN */
                                          GAsyncResult   *result,   /* IN */
                                          GError        **error)    /* OUT */
{
	PkaChannel *channel;
	ChannelSetKillPidCall *call;
	gboolean ret = FALSE;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(channel_set_kill_pid), FALSE);

	ENTRY;
	call = GET_RESULT_POINTER(ChannelSetKillPidCall, result);
	if (!pka_manager_find_channel(DEFAULT_CONTEXT, call->channel,
	                              &channel, error)) {
	    GOTO(failed);
	}
	ret = pka_channel_set_kill_pid(channel, DEFAULT_CONTEXT,
	                               call->kill_pid, error);
	g_object_unref(channel);
  failed:
	RETURN(ret);
}

/**
 * pk_connection_channel_set_pid_async:
 * @connection: A #PkConnection.
 * @channel: A #gint.
 * @pid: A #gint.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "channel_set_pid_async" RPC.  @callback
 * MUST call pka_listener_channel_set_pid_finish().
 *
 * Sets the target pid to attach to rather than spawning a process.  This can
 * only be set before the channel has started.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_channel_set_pid_async (PkaListener           *listener,    /* IN */
                                    gint                   channel,     /* IN */
                                    gint                   pid,         /* IN */
                                    GCancellable          *cancellable, /* IN */
                                    GAsyncReadyCallback    callback,    /* IN */
                                    gpointer               user_data)   /* IN */
{
	ChannelSetPidCall *call;
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_channel_set_pid_async);
	call = ChannelSetPidCall_Create();
	call->channel = channel;
	call->pid = pid;
	g_simple_async_result_set_op_res_gpointer(
			result, call, (GDestroyNotify)ChannelSetPidCall_Free);
	g_simple_async_result_complete(result);
	g_object_unref(result);
	EXIT;
}

/**
 * pk_connection_channel_set_pid_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "channel_set_pid_finish" RPC.
 *
 * Sets the target pid to attach to rather than spawning a process.  This can
 * only be set before the channel has started.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_channel_set_pid_finish (PkaListener    *listener, /* IN */
                                     GAsyncResult   *result,   /* IN */
                                     GError        **error)    /* OUT */
{
	PkaChannel *channel;
	ChannelSetPidCall *call;
	gboolean ret = FALSE;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);
	g_return_val_if_fail(RESULT_IS_VALID(channel_set_pid), FALSE);

	ENTRY;
	call = GET_RESULT_POINTER(ChannelSetPidCall, result);
	if (!pka_manager_find_channel(DEFAULT_CONTEXT, call->channel,
	                              &channel, error)) {
		GOTO(failed);
	}
	ret = pka_channel_set_pid(channel, DEFAULT_CONTEXT, call->pid, error);
	g_object_unref(channel);
  failed:
	RETURN(ret);
}

#if 0
static void
pka_listener_channel_set_target_cb (GObject      *listener,    /* IN */
                                    GAsyncResult *result,      /* IN */
                                    gpointer      user_data)   /* IN */
{
	GSimpleAsyncResult *real_result;

	g_return_if_fail(PKA_IS_LISTENER(listener));
	g_return_if_fail(RESULT_IS_VALID(channel_set_target));

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	g_simple_async_result_set_op_res_gpointer(real_result, result);
	g_simple_async_result_complete(real_result);
	EXIT;
}
#endif

/**
 * pk_connection_channel_set_target_async:
 * @connection: A #PkConnection.
 * @channel: A #gint.
 * @target: A #const gchar.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "channel_set_target_async" RPC.  @callback
 * MUST call pka_listener_channel_set_target_finish().
 *
 * Sets the channels target.  This may only be set before the channel has
 * started.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_channel_set_target_async (PkaListener           *listener,    /* IN */
                                       gint                   channel,     /* IN */
                                       const gchar           *target,      /* IN */
                                       GCancellable          *cancellable, /* IN */
                                       GAsyncReadyCallback    callback,    /* IN */
                                       gpointer               user_data)   /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_channel_set_target_async);
// TEMP TO TEST RPC RESULTS
	g_simple_async_result_complete(result);
	g_object_unref(result);
#if 0
	pka_channel_set_target_async(instance,
	                             NULL,
	                             pka_listener_channel_set_target_cb,
	                             result);
#endif
	EXIT;
}

/**
 * pk_connection_channel_set_target_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "channel_set_target_finish" RPC.
 *
 * Sets the channels target.  This may only be set before the channel has
 * started.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_channel_set_target_finish (PkaListener    *listener, /* IN */
                                        GAsyncResult   *result,   /* IN */
                                        GError        **error)    /* OUT */
{
	ENTRY;
// TEMP TO TEST RPC RESULTS
	RETURN(TRUE);
#if 0
	GSimpleAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	ret = pka_channel_set_target_finish(instance,
	                                    real_result,
	                                    error);
	RETURN(ret);
#endif
}

#if 0
static void
pka_listener_channel_set_working_dir_cb (GObject      *listener,    /* IN */
                                         GAsyncResult *result,      /* IN */
                                         gpointer      user_data)   /* IN */
{
	GSimpleAsyncResult *real_result;

	g_return_if_fail(PKA_IS_LISTENER(listener));
	g_return_if_fail(RESULT_IS_VALID(channel_set_working_dir));

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	g_simple_async_result_set_op_res_gpointer(real_result, result);
	g_simple_async_result_complete(real_result);
	EXIT;
}
#endif

/**
 * pk_connection_channel_set_working_dir_async:
 * @connection: A #PkConnection.
 * @channel: A #gint.
 * @working_dir: A #const gchar.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "channel_set_working_dir_async" RPC.  @callback
 * MUST call pka_listener_channel_set_working_dir_finish().
 *
 * Sets the targets working directory.  This may only be set before the
 * channel has started.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_channel_set_working_dir_async (PkaListener           *listener,    /* IN */
                                            gint                   channel,     /* IN */
                                            const gchar           *working_dir, /* IN */
                                            GCancellable          *cancellable, /* IN */
                                            GAsyncReadyCallback    callback,    /* IN */
                                            gpointer               user_data)   /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_channel_set_working_dir_async);
// TEMP TO TEST RPC RESULTS
	g_simple_async_result_complete(result);
	g_object_unref(result);
#if 0
	pka_channel_set_working_dir_async(instance,
	                                  NULL,
	                                  pka_listener_channel_set_working_dir_cb,
	                                  result);
#endif
	EXIT;
}

/**
 * pk_connection_channel_set_working_dir_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "channel_set_working_dir_finish" RPC.
 *
 * Sets the targets working directory.  This may only be set before the
 * channel has started.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_channel_set_working_dir_finish (PkaListener    *listener, /* IN */
                                             GAsyncResult   *result,   /* IN */
                                             GError        **error)    /* OUT */
{
	ENTRY;
// TEMP TO TEST RPC RESULTS
	RETURN(TRUE);
#if 0
	GSimpleAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	ret = pka_channel_set_working_dir_finish(instance,
	                                         real_result,
	                                         error);
	RETURN(ret);
#endif
}

#if 0
static void
pka_listener_channel_start_cb (GObject      *listener,    /* IN */
                               GAsyncResult *result,      /* IN */
                               gpointer      user_data)   /* IN */
{
	GSimpleAsyncResult *real_result;

	g_return_if_fail(PKA_IS_LISTENER(listener));
	g_return_if_fail(RESULT_IS_VALID(channel_start));

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	g_simple_async_result_set_op_res_gpointer(real_result, result);
	g_simple_async_result_complete(real_result);
	EXIT;
}
#endif

/**
 * pk_connection_channel_start_async:
 * @connection: A #PkConnection.
 * @channel: A #gint.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "channel_start_async" RPC.  @callback
 * MUST call pka_listener_channel_start_finish().
 *
 * Start the channel. If required, the process will be spawned.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_channel_start_async (PkaListener           *listener,    /* IN */
                                  gint                   channel,     /* IN */
                                  GCancellable          *cancellable, /* IN */
                                  GAsyncReadyCallback    callback,    /* IN */
                                  gpointer               user_data)   /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_channel_start_async);
// TEMP TO TEST RPC RESULTS
	g_simple_async_result_complete(result);
	g_object_unref(result);
#if 0
	pka_channel_start_async(instance,
	                        NULL,
	                        pka_listener_channel_start_cb,
	                        result);
#endif
	EXIT;
}

/**
 * pk_connection_channel_start_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "channel_start_finish" RPC.
 *
 * Start the channel. If required, the process will be spawned.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_channel_start_finish (PkaListener    *listener, /* IN */
                                   GAsyncResult   *result,   /* IN */
                                   GError        **error)    /* OUT */
{
	ENTRY;
// TEMP TO TEST RPC RESULTS
	RETURN(TRUE);
#if 0
	GSimpleAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	ret = pka_channel_start_finish(instance,
	                               real_result,
	                               error);
	RETURN(ret);
#endif
}

#if 0
static void
pka_listener_channel_stop_cb (GObject      *listener,    /* IN */
                              GAsyncResult *result,      /* IN */
                              gpointer      user_data)   /* IN */
{
	GSimpleAsyncResult *real_result;

	g_return_if_fail(PKA_IS_LISTENER(listener));
	g_return_if_fail(RESULT_IS_VALID(channel_stop));

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	g_simple_async_result_set_op_res_gpointer(real_result, result);
	g_simple_async_result_complete(real_result);
	EXIT;
}
#endif

/**
 * pk_connection_channel_stop_async:
 * @connection: A #PkConnection.
 * @channel: A #gint.
 * @killpid: A #gboolean.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "channel_stop_async" RPC.  @callback
 * MUST call pka_listener_channel_stop_finish().
 *
 * Stop the channel. If @killpid, the inferior process is terminated.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_channel_stop_async (PkaListener           *listener,    /* IN */
                                 gint                   channel,     /* IN */
                                 gboolean               killpid,     /* IN */
                                 GCancellable          *cancellable, /* IN */
                                 GAsyncReadyCallback    callback,    /* IN */
                                 gpointer               user_data)   /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_channel_stop_async);
// TEMP TO TEST RPC RESULTS
	g_simple_async_result_complete(result);
	g_object_unref(result);
#if 0
	pka_channel_stop_async(instance,
	                       NULL,
	                       pka_listener_channel_stop_cb,
	                       result);
#endif
	EXIT;
}

/**
 * pk_connection_channel_stop_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "channel_stop_finish" RPC.
 *
 * Stop the channel. If @killpid, the inferior process is terminated.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_channel_stop_finish (PkaListener    *listener, /* IN */
                                  GAsyncResult   *result,   /* IN */
                                  GError        **error)    /* OUT */
{
	ENTRY;
// TEMP TO TEST RPC RESULTS
	RETURN(TRUE);
#if 0
	GSimpleAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	ret = pka_channel_stop_finish(instance,
	                              real_result,
	                              error);
	RETURN(ret);
#endif
}

#if 0
static void
pka_listener_channel_unmute_cb (GObject      *listener,    /* IN */
                                GAsyncResult *result,      /* IN */
                                gpointer      user_data)   /* IN */
{
	GSimpleAsyncResult *real_result;

	g_return_if_fail(PKA_IS_LISTENER(listener));
	g_return_if_fail(RESULT_IS_VALID(channel_unmute));

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	g_simple_async_result_set_op_res_gpointer(real_result, result);
	g_simple_async_result_complete(real_result);
	g_object_unref(real_result);
	EXIT;
}
#endif

/**
 * pk_connection_channel_unmute_async:
 * @connection: A #PkConnection.
 * @channel: A #gint.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "channel_unmute_async" RPC.  @callback
 * MUST call pka_listener_channel_unmute_finish().
 *
 * Resumes delivery of manifest and samples for sources within the channel.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_channel_unmute_async (PkaListener           *listener,    /* IN */
                                   gint                   channel,     /* IN */
                                   GCancellable          *cancellable, /* IN */
                                   GAsyncReadyCallback    callback,    /* IN */
                                   gpointer               user_data)   /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_channel_unmute_async);
// TEMP TO TEST RPC RESULTS
	g_simple_async_result_complete(result);
	g_object_unref(result);
#if 0
	pka_channel_unmute_async(instance,
	                         NULL,
	                         pka_listener_channel_unmute_cb,
	                         result);
#endif
	EXIT;
}

/**
 * pk_connection_channel_unmute_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "channel_unmute_finish" RPC.
 *
 * Resumes delivery of manifest and samples for sources within the channel.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_channel_unmute_finish (PkaListener    *listener, /* IN */
                                    GAsyncResult   *result,   /* IN */
                                    GError        **error)    /* OUT */
{
	ENTRY;
// TEMP TO TEST RPC RESULTS
	RETURN(TRUE);
#if 0
	GSimpleAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	ret = pka_channel_unmute_finish(instance,
	                                real_result,
	                                error);
	RETURN(ret);
#endif
}

#if 0
static void
pka_listener_encoder_get_plugin_cb (GObject      *listener,    /* IN */
                                    GAsyncResult *result,      /* IN */
                                    gpointer      user_data)   /* IN */
{
	GSimpleAsyncResult *real_result;

	g_return_if_fail(PKA_IS_LISTENER(listener));
	g_return_if_fail(RESULT_IS_VALID(encoder_get_plugin));

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	g_simple_async_result_set_op_res_gpointer(real_result, result);
	g_simple_async_result_complete(real_result);
	EXIT;
}
#endif

/**
 * pk_connection_encoder_get_plugin_async:
 * @connection: A #PkConnection.
 * @encoder: A #gint.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "encoder_get_plugin_async" RPC.  @callback
 * MUST call pka_listener_encoder_get_plugin_finish().
 *
 * Retrieves the plugin which created the encoder instance.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_encoder_get_plugin_async (PkaListener           *listener,    /* IN */
                                       gint                   encoder,     /* IN */
                                       GCancellable          *cancellable, /* IN */
                                       GAsyncReadyCallback    callback,    /* IN */
                                       gpointer               user_data)   /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_encoder_get_plugin_async);
// TEMP TO TEST RPC RESULTS
	g_simple_async_result_complete(result);
	g_object_unref(result);
#if 0
	pka_encoder_get_plugin_async(instance,
	                             NULL,
	                             pka_listener_encoder_get_plugin_cb,
	                             result);
#endif
	EXIT;
}

/**
 * pk_connection_encoder_get_plugin_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @pluign: A #gchar.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "encoder_get_plugin_finish" RPC.
 *
 * Retrieves the plugin which created the encoder instance.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_encoder_get_plugin_finish (PkaListener    *listener, /* IN */
                                        GAsyncResult   *result,   /* IN */
                                        gchar         **pluign,   /* OUT */
                                        GError        **error)    /* OUT */
{
	ENTRY;
// TEMP TO TEST RPC RESULTS
	RETURN(TRUE);
#if 0
	GSimpleAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	ret = pka_encoder_get_plugin_finish(instance,
	                                    real_result,
	                                    pluign,
	                                    error);
	RETURN(ret);
#endif
}

/**
 * pk_connection_manager_add_channel_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "manager_add_channel_async" RPC.  @callback
 * MUST call pka_listener_manager_add_channel_finish().
 *
 * Adds a channel to the agent.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_manager_add_channel_async (PkaListener           *listener,    /* IN */
                                        GCancellable          *cancellable, /* IN */
                                        GAsyncReadyCallback    callback,    /* IN */
                                        gpointer               user_data)   /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_manager_add_channel_async);
	g_simple_async_result_complete(result);
	g_object_unref(result);
	EXIT;
}

/**
 * pk_connection_manager_add_channel_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @channel: A #gint.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "manager_add_channel_finish" RPC.
 *
 * Adds a channel to the agent.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_manager_add_channel_finish (PkaListener    *listener, /* IN */
                                         GAsyncResult   *result,   /* IN */
                                         gint           *channel,  /* OUT */
                                         GError        **error)    /* OUT */
{
	PkaChannel *real_channel = NULL;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);

	ENTRY;
	if (pka_manager_add_channel(DEFAULT_CONTEXT, &real_channel, error)) {
		*channel = pka_channel_get_id(real_channel);
		g_object_unref(real_channel);
		RETURN(TRUE);
	}
	RETURN(FALSE);
}

#if 0
static void
pka_listener_manager_add_subscription_cb (GObject      *listener,    /* IN */
                                          GAsyncResult *result,      /* IN */
                                          gpointer      user_data)   /* IN */
{
	GSimpleAsyncResult *real_result;

	g_return_if_fail(PKA_IS_LISTENER(listener));
	g_return_if_fail(RESULT_IS_VALID(manager_add_subscription));

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	g_simple_async_result_set_op_res_gpointer(real_result, result);
	g_simple_async_result_complete(real_result);
	EXIT;
}
#endif

/**
 * pk_connection_manager_add_subscription_async:
 * @connection: A #PkConnection.
 * @buffer_size: A #gsize.
 * @timeout: A #gsize.
 * @encoder: A #gint.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "manager_add_subscription_async" RPC.  @callback
 * MUST call pka_listener_manager_add_subscription_finish().
 *
 * Adds a new subscription to the agent. @buffer_size is the size of the
 * internal buffer in bytes to queue before flushing data to the subscriber.
 * @timeout is the maximum number of milliseconds that should pass before
 * flushing data to the subscriber.
 * 
 * If @buffer_size and @timeout are 0, then no buffering will occur.
 * 
 * @encoder is an optional encoder that can be used to encode the data
 * into a particular format the subscriber is expecting.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_manager_add_subscription_async (PkaListener           *listener,    /* IN */
                                             gsize                  buffer_size, /* IN */
                                             gsize                  timeout,     /* IN */
                                             gint                   encoder,     /* IN */
                                             GCancellable          *cancellable, /* IN */
                                             GAsyncReadyCallback    callback,    /* IN */
                                             gpointer               user_data)   /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_manager_add_subscription_async);
// TEMP TO TEST RPC RESULTS
	g_simple_async_result_complete(result);
	g_object_unref(result);
#if 0
	pka_manager_add_subscription_async(instance,
	                                   NULL,
	                                   pka_listener_manager_add_subscription_cb,
	                                   result);
#endif
	EXIT;
}

/**
 * pk_connection_manager_add_subscription_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @subscription: A #gint.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "manager_add_subscription_finish" RPC.
 *
 * Adds a new subscription to the agent. @buffer_size is the size of the
 * internal buffer in bytes to queue before flushing data to the subscriber.
 * @timeout is the maximum number of milliseconds that should pass before
 * flushing data to the subscriber.
 * 
 * If @buffer_size and @timeout are 0, then no buffering will occur.
 * 
 * @encoder is an optional encoder that can be used to encode the data
 * into a particular format the subscriber is expecting.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_manager_add_subscription_finish (PkaListener    *listener,     /* IN */
                                              GAsyncResult   *result,       /* IN */
                                              gint           *subscription, /* OUT */
                                              GError        **error)        /* OUT */
{
	ENTRY;
// TEMP TO TEST RPC RESULTS
	RETURN(TRUE);
#if 0
	GSimpleAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	ret = pka_manager_add_subscription_finish(instance,
	                                          real_result,
	                                          subscription,
	                                          error);
	RETURN(ret);
#endif
}

/**
 * pk_connection_manager_get_channels_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "manager_get_channels_async" RPC.  @callback
 * MUST call pka_listener_manager_get_channels_finish().
 *
 * Retrieves the list of channels located within the agent.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_manager_get_channels_async (PkaListener           *listener,    /* IN */
                                         GCancellable          *cancellable, /* IN */
                                         GAsyncReadyCallback    callback,    /* IN */
                                         gpointer               user_data)   /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_manager_get_channels_async);
	g_simple_async_result_complete(result);
	g_object_unref(result);
	EXIT;
}

/**
 * pk_connection_manager_get_channels_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @channels: A #gint.
 * @channels_len: A #gsize.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "manager_get_channels_finish" RPC.
 *
 * Retrieves the list of channels located within the agent.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_manager_get_channels_finish (PkaListener    *listener,     /* IN */
                                          GAsyncResult   *result,       /* IN */
                                          gint          **channels,     /* OUT */
                                          gsize          *channels_len, /* OUT */
                                          GError        **error)        /* OUT */
{
	GList *list = NULL;
	GList *iter;
	gboolean ret;
	gint *ids = NULL;
	gint i = 0;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);

	ENTRY;
	if (!(ret = pka_manager_get_channels(DEFAULT_CONTEXT, &list, error))) {
		*channels_len = g_list_length(list);
		ids = g_new0(gint, *channels_len);
		for (iter = list; iter; iter = iter->next) {
			ids[i++] = pka_channel_get_id(iter->data);
		}
		*channels = ids;
		g_list_foreach(list, (GFunc)g_object_unref, NULL);
		g_list_free(list);
	}
	RETURN(ret);
}

/**
 * pk_connection_manager_get_plugins_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "manager_get_plugins_async" RPC.  @callback
 * MUST call pka_listener_manager_get_plugins_finish().
 *
 * Retrieves the list of available plugins within the agent.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_manager_get_plugins_async (PkaListener           *listener,    /* IN */
                                        GCancellable          *cancellable, /* IN */
                                        GAsyncReadyCallback    callback,    /* IN */
                                        gpointer               user_data)   /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_manager_get_plugins_async);
	g_simple_async_result_complete(result);
	g_object_unref(result);
	EXIT;
}

/**
 * pk_connection_manager_get_plugins_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @plugins: A #gchar.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "manager_get_plugins_finish" RPC.
 *
 * Retrieves the list of available plugins within the agent.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_manager_get_plugins_finish (PkaListener    *listener, /* IN */
                                         GAsyncResult   *result,   /* IN */
                                         gchar        ***plugins,  /* OUT */
                                         GError        **error)    /* OUT */
{
	GList *list = NULL;
	GList *iter;
	gchar **names;
	gboolean ret;
	gint i = 0;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);

	ENTRY;
	if ((ret = pka_manager_get_plugins(DEFAULT_CONTEXT, &list, error))) {
		names = g_new0(gchar*, g_list_length(list) + 1);
		for (iter = list; iter; iter = iter->next) {
			names[i++] = g_strdup(pka_plugin_get_id(iter->data));
		}
		*plugins = names;
		g_list_foreach(list, (GFunc)g_object_unref, NULL);
		g_list_free(list);
	}
	RETURN(ret);
}

/**
 * pk_connection_manager_get_version_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "manager_get_version_async" RPC.  @callback
 * MUST call pka_listener_manager_get_version_finish().
 *
 * Retrieves the version of the agent.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_manager_get_version_async (PkaListener           *listener,    /* IN */
                                        GCancellable          *cancellable, /* IN */
                                        GAsyncReadyCallback    callback,    /* IN */
                                        gpointer               user_data)   /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_manager_get_version_async);
	g_simple_async_result_complete(result);
	g_object_unref(result);
	EXIT;
}

/**
 * pk_connection_manager_get_version_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @version: A #gchar.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "manager_get_version_finish" RPC.
 *
 * Retrieves the version of the agent.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_manager_get_version_finish (PkaListener    *listener, /* IN */
                                         GAsyncResult   *result,   /* IN */
                                         gchar         **version,  /* OUT */
                                         GError        **error)    /* OUT */
{
	ENTRY;
	*version = g_strdup(PKA_VERSION_S);
	RETURN(TRUE);
}

/**
 * pk_connection_manager_ping_async:
 * @connection: A #PkConnection.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "manager_ping_async" RPC.  @callback
 * MUST call pka_listener_manager_ping_finish().
 *
 * Pings the agent over the RPC protocol to determine one-way latency.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_manager_ping_async (PkaListener           *listener,    /* IN */
                                 GCancellable          *cancellable, /* IN */
                                 GAsyncReadyCallback    callback,    /* IN */
                                 gpointer               user_data)   /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_manager_ping_async);
	g_simple_async_result_complete(result);
	g_object_unref(result);
	EXIT;
}

/**
 * pk_connection_manager_ping_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @tv: A #GTimeVal.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "manager_ping_finish" RPC.
 *
 * Pings the agent over the RPC protocol to determine one-way latency.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_manager_ping_finish (PkaListener    *listener, /* IN */
                                  GAsyncResult   *result,   /* IN */
                                  GTimeVal       *tv,       /* OUT */
                                  GError        **error)    /* OUT */
{
	ENTRY;
	g_get_current_time(tv);
	RETURN(TRUE);
}

#if 0
static void
pka_listener_manager_remove_channel_cb (GObject      *listener,    /* IN */
                                        GAsyncResult *result,      /* IN */
                                        gpointer      user_data)   /* IN */
{
	GSimpleAsyncResult *real_result;

	g_return_if_fail(PKA_IS_LISTENER(listener));
	g_return_if_fail(RESULT_IS_VALID(manager_remove_channel));

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	g_simple_async_result_set_op_res_gpointer(real_result, result);
	g_simple_async_result_complete(real_result);
	EXIT;
}
#endif

/**
 * pk_connection_manager_remove_channel_async:
 * @connection: A #PkConnection.
 * @channel: A #gint.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "manager_remove_channel_async" RPC.  @callback
 * MUST call pka_listener_manager_remove_channel_finish().
 *
 * Removes a channel from the agent.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_manager_remove_channel_async (PkaListener           *listener,    /* IN */
                                           gint                   channel,     /* IN */
                                           GCancellable          *cancellable, /* IN */
                                           GAsyncReadyCallback    callback,    /* IN */
                                           gpointer               user_data)   /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_manager_remove_channel_async);
// TEMP TO TEST RPC RESULTS
	g_simple_async_result_complete(result);
	g_object_unref(result);
#if 0
	pka_manager_remove_channel_async(instance,
	                                 NULL,
	                                 pka_listener_manager_remove_channel_cb,
	                                 result);
#endif
	EXIT;
}

/**
 * pk_connection_manager_remove_channel_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @removed: A #gboolean.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "manager_remove_channel_finish" RPC.
 *
 * Removes a channel from the agent.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_manager_remove_channel_finish (PkaListener    *listener, /* IN */
                                            GAsyncResult   *result,   /* IN */
                                            gboolean       *removed,  /* OUT */
                                            GError        **error)    /* OUT */
{
	ENTRY;
// TEMP TO TEST RPC RESULTS
	RETURN(TRUE);
#if 0
	GSimpleAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	ret = pka_manager_remove_channel_finish(instance,
	                                        real_result,
	                                        removed,
	                                        error);
	RETURN(ret);
#endif
}

#if 0
static void
pka_listener_manager_remove_subscription_cb (GObject      *listener,    /* IN */
                                             GAsyncResult *result,      /* IN */
                                             gpointer      user_data)   /* IN */
{
	GSimpleAsyncResult *real_result;

	g_return_if_fail(PKA_IS_LISTENER(listener));
	g_return_if_fail(RESULT_IS_VALID(manager_remove_subscription));

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	g_simple_async_result_set_op_res_gpointer(real_result, result);
	g_simple_async_result_complete(real_result);
	EXIT;
}
#endif

/**
 * pk_connection_manager_remove_subscription_async:
 * @connection: A #PkConnection.
 * @subscription: A #gint.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "manager_remove_subscription_async" RPC.  @callback
 * MUST call pka_listener_manager_remove_subscription_finish().
 *
 * Removes a subscription from the agent.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_manager_remove_subscription_async (PkaListener           *listener,     /* IN */
                                                gint                   subscription, /* IN */
                                                GCancellable          *cancellable,  /* IN */
                                                GAsyncReadyCallback    callback,     /* IN */
                                                gpointer               user_data)    /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_manager_remove_subscription_async);
// TEMP TO TEST RPC RESULTS
	g_simple_async_result_complete(result);
	g_object_unref(result);
#if 0
	pka_manager_remove_subscription_async(instance,
	                                      NULL,
	                                      pka_listener_manager_remove_subscription_cb,
	                                      result);
#endif
	EXIT;
}

/**
 * pk_connection_manager_remove_subscription_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @removed: A #gboolean.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "manager_remove_subscription_finish" RPC.
 *
 * Removes a subscription from the agent.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_manager_remove_subscription_finish (PkaListener    *listener, /* IN */
                                                 GAsyncResult   *result,   /* IN */
                                                 gboolean       *removed,  /* OUT */
                                                 GError        **error)    /* OUT */
{
	ENTRY;
// TEMP TO TEST RPC RESULTS
	RETURN(TRUE);
#if 0
	GSimpleAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	ret = pka_manager_remove_subscription_finish(instance,
	                                             real_result,
	                                             removed,
	                                             error);
	RETURN(ret);
#endif
}

#if 0
static void
pka_listener_plugin_create_encoder_cb (GObject      *listener,    /* IN */
                                       GAsyncResult *result,      /* IN */
                                       gpointer      user_data)   /* IN */
{
	GSimpleAsyncResult *real_result;

	g_return_if_fail(PKA_IS_LISTENER(listener));
	g_return_if_fail(RESULT_IS_VALID(plugin_create_encoder));

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	g_simple_async_result_set_op_res_gpointer(real_result, result);
	g_simple_async_result_complete(real_result);
	EXIT;
}
#endif

/**
 * pk_connection_plugin_create_encoder_async:
 * @connection: A #PkConnection.
 * @plugin: A #const gchar.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "plugin_create_encoder_async" RPC.  @callback
 * MUST call pka_listener_plugin_create_encoder_finish().
 *
 * Creates a new instance of the encoder plugin.  If the plugin type is not
 * an encoder plugin then this will fail.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_plugin_create_encoder_async (PkaListener           *listener,    /* IN */
                                          const gchar           *plugin,      /* IN */
                                          GCancellable          *cancellable, /* IN */
                                          GAsyncReadyCallback    callback,    /* IN */
                                          gpointer               user_data)   /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_plugin_create_encoder_async);
// TEMP TO TEST RPC RESULTS
	g_simple_async_result_complete(result);
	g_object_unref(result);
#if 0
	pka_plugin_create_encoder_async(instance,
	                                NULL,
	                                pka_listener_plugin_create_encoder_cb,
	                                result);
#endif
	EXIT;
}

/**
 * pk_connection_plugin_create_encoder_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @encoder: A #gint.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "plugin_create_encoder_finish" RPC.
 *
 * Creates a new instance of the encoder plugin.  If the plugin type is not
 * an encoder plugin then this will fail.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_plugin_create_encoder_finish (PkaListener    *listener, /* IN */
                                           GAsyncResult   *result,   /* IN */
                                           gint           *encoder,  /* OUT */
                                           GError        **error)    /* OUT */
{
	ENTRY;
// TEMP TO TEST RPC RESULTS
	RETURN(TRUE);
#if 0
	GSimpleAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	ret = pka_plugin_create_encoder_finish(instance,
	                                       real_result,
	                                       encoder,
	                                       error);
	RETURN(ret);
#endif
}

#if 0
static void
pka_listener_plugin_create_source_cb (GObject      *listener,    /* IN */
                                      GAsyncResult *result,      /* IN */
                                      gpointer      user_data)   /* IN */
{
	GSimpleAsyncResult *real_result;

	g_return_if_fail(PKA_IS_LISTENER(listener));
	g_return_if_fail(RESULT_IS_VALID(plugin_create_source));

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	g_simple_async_result_set_op_res_gpointer(real_result, result);
	g_simple_async_result_complete(real_result);
	EXIT;
}
#endif

/**
 * pk_connection_plugin_create_source_async:
 * @connection: A #PkConnection.
 * @plugin: A #const gchar.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "plugin_create_source_async" RPC.  @callback
 * MUST call pka_listener_plugin_create_source_finish().
 *
 * Creates a new instance of the source plugin.  If the plugin type is not
 * a source plugin then this will fail.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_plugin_create_source_async (PkaListener           *listener,    /* IN */
                                         const gchar           *plugin,      /* IN */
                                         GCancellable          *cancellable, /* IN */
                                         GAsyncReadyCallback    callback,    /* IN */
                                         gpointer               user_data)   /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_plugin_create_source_async);
// TEMP TO TEST RPC RESULTS
	g_simple_async_result_complete(result);
	g_object_unref(result);
#if 0
	pka_plugin_create_source_async(instance,
	                               NULL,
	                               pka_listener_plugin_create_source_cb,
	                               result);
#endif
	EXIT;
}

/**
 * pk_connection_plugin_create_source_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @source: A #gint.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "plugin_create_source_finish" RPC.
 *
 * Creates a new instance of the source plugin.  If the plugin type is not
 * a source plugin then this will fail.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_plugin_create_source_finish (PkaListener    *listener, /* IN */
                                          GAsyncResult   *result,   /* IN */
                                          gint           *source,   /* OUT */
                                          GError        **error)    /* OUT */
{
	ENTRY;
// TEMP TO TEST RPC RESULTS
	RETURN(TRUE);
#if 0
	GSimpleAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	ret = pka_plugin_create_source_finish(instance,
	                                      real_result,
	                                      source,
	                                      error);
	RETURN(ret);
#endif
}

#if 0
static void
pka_listener_plugin_get_copyright_cb (GObject      *listener,    /* IN */
                                      GAsyncResult *result,      /* IN */
                                      gpointer      user_data)   /* IN */
{
	GSimpleAsyncResult *real_result;

	g_return_if_fail(PKA_IS_LISTENER(listener));
	g_return_if_fail(RESULT_IS_VALID(plugin_get_copyright));

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	g_simple_async_result_set_op_res_gpointer(real_result, result);
	g_simple_async_result_complete(real_result);
	EXIT;
}
#endif

/**
 * pk_connection_plugin_get_copyright_async:
 * @connection: A #PkConnection.
 * @plugin: A #const gchar.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "plugin_get_copyright_async" RPC.  @callback
 * MUST call pka_listener_plugin_get_copyright_finish().
 *
 * The plugin copyright.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_plugin_get_copyright_async (PkaListener           *listener,    /* IN */
                                         const gchar           *plugin,      /* IN */
                                         GCancellable          *cancellable, /* IN */
                                         GAsyncReadyCallback    callback,    /* IN */
                                         gpointer               user_data)   /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_plugin_get_copyright_async);
// TEMP TO TEST RPC RESULTS
	g_simple_async_result_complete(result);
	g_object_unref(result);
#if 0
	pka_plugin_get_copyright_async(instance,
	                               NULL,
	                               pka_listener_plugin_get_copyright_cb,
	                               result);
#endif
	EXIT;
}

/**
 * pk_connection_plugin_get_copyright_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @copyright: A #gchar.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "plugin_get_copyright_finish" RPC.
 *
 * The plugin copyright.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_plugin_get_copyright_finish (PkaListener    *listener,  /* IN */
                                          GAsyncResult   *result,    /* IN */
                                          gchar         **copyright, /* OUT */
                                          GError        **error)     /* OUT */
{
	ENTRY;
// TEMP TO TEST RPC RESULTS
	RETURN(TRUE);
#if 0
	GSimpleAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	ret = pka_plugin_get_copyright_finish(instance,
	                                      real_result,
	                                      copyright,
	                                      error);
	RETURN(ret);
#endif
}

#if 0
static void
pka_listener_plugin_get_description_cb (GObject      *listener,    /* IN */
                                        GAsyncResult *result,      /* IN */
                                        gpointer      user_data)   /* IN */
{
	GSimpleAsyncResult *real_result;

	g_return_if_fail(PKA_IS_LISTENER(listener));
	g_return_if_fail(RESULT_IS_VALID(plugin_get_description));

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	g_simple_async_result_set_op_res_gpointer(real_result, result);
	g_simple_async_result_complete(real_result);
	EXIT;
}
#endif

/**
 * pk_connection_plugin_get_description_async:
 * @connection: A #PkConnection.
 * @plugin: A #const gchar.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "plugin_get_description_async" RPC.  @callback
 * MUST call pka_listener_plugin_get_description_finish().
 *
 * The plugin description.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_plugin_get_description_async (PkaListener           *listener,    /* IN */
                                           const gchar           *plugin,      /* IN */
                                           GCancellable          *cancellable, /* IN */
                                           GAsyncReadyCallback    callback,    /* IN */
                                           gpointer               user_data)   /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_plugin_get_description_async);
// TEMP TO TEST RPC RESULTS
	g_simple_async_result_complete(result);
	g_object_unref(result);
#if 0
	pka_plugin_get_description_async(instance,
	                                 NULL,
	                                 pka_listener_plugin_get_description_cb,
	                                 result);
#endif
	EXIT;
}

/**
 * pk_connection_plugin_get_description_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @description: A #gchar.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "plugin_get_description_finish" RPC.
 *
 * The plugin description.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_plugin_get_description_finish (PkaListener    *listener,    /* IN */
                                            GAsyncResult   *result,      /* IN */
                                            gchar         **description, /* OUT */
                                            GError        **error)       /* OUT */
{
	ENTRY;
// TEMP TO TEST RPC RESULTS
	RETURN(TRUE);
#if 0
	GSimpleAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	ret = pka_plugin_get_description_finish(instance,
	                                        real_result,
	                                        description,
	                                        error);
	RETURN(ret);
#endif
}

#if 0
static void
pka_listener_plugin_get_name_cb (GObject      *listener,    /* IN */
                                 GAsyncResult *result,      /* IN */
                                 gpointer      user_data)   /* IN */
{
	GSimpleAsyncResult *real_result;

	g_return_if_fail(PKA_IS_LISTENER(listener));
	g_return_if_fail(RESULT_IS_VALID(plugin_get_name));

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	g_simple_async_result_set_op_res_gpointer(real_result, result);
	g_simple_async_result_complete(real_result);
	EXIT;
}
#endif

/**
 * pk_connection_plugin_get_name_async:
 * @connection: A #PkConnection.
 * @plugin: A #const gchar.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "plugin_get_name_async" RPC.  @callback
 * MUST call pka_listener_plugin_get_name_finish().
 *
 * The plugin name.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_plugin_get_name_async (PkaListener           *listener,    /* IN */
                                    const gchar           *plugin,      /* IN */
                                    GCancellable          *cancellable, /* IN */
                                    GAsyncReadyCallback    callback,    /* IN */
                                    gpointer               user_data)   /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_plugin_get_name_async);
// TEMP TO TEST RPC RESULTS
	g_simple_async_result_complete(result);
	g_object_unref(result);
#if 0
	pka_plugin_get_name_async(instance,
	                          NULL,
	                          pka_listener_plugin_get_name_cb,
	                          result);
#endif
	EXIT;
}

/**
 * pk_connection_plugin_get_name_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @name: A #gchar.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "plugin_get_name_finish" RPC.
 *
 * The plugin name.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_plugin_get_name_finish (PkaListener    *listener, /* IN */
                                     GAsyncResult   *result,   /* IN */
                                     gchar         **name,     /* OUT */
                                     GError        **error)    /* OUT */
{
	ENTRY;
// TEMP TO TEST RPC RESULTS
	RETURN(TRUE);
#if 0
	GSimpleAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	ret = pka_plugin_get_name_finish(instance,
	                                 real_result,
	                                 name,
	                                 error);
	RETURN(ret);
#endif
}

#if 0
static void
pka_listener_plugin_get_plugin_type_cb (GObject      *listener,    /* IN */
                                        GAsyncResult *result,      /* IN */
                                        gpointer      user_data)   /* IN */
{
	GSimpleAsyncResult *real_result;

	g_return_if_fail(PKA_IS_LISTENER(listener));
	g_return_if_fail(RESULT_IS_VALID(plugin_get_plugin_type));

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	g_simple_async_result_set_op_res_gpointer(real_result, result);
	g_simple_async_result_complete(real_result);
	EXIT;
}
#endif

/**
 * pk_connection_plugin_get_plugin_type_async:
 * @connection: A #PkConnection.
 * @plugin: A #const gchar.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "plugin_get_plugin_type_async" RPC.  @callback
 * MUST call pka_listener_plugin_get_plugin_type_finish().
 *
 * The plugin type.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_plugin_get_plugin_type_async (PkaListener           *listener,    /* IN */
                                           const gchar           *plugin,      /* IN */
                                           GCancellable          *cancellable, /* IN */
                                           GAsyncReadyCallback    callback,    /* IN */
                                           gpointer               user_data)   /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_plugin_get_plugin_type_async);
// TEMP TO TEST RPC RESULTS
	g_simple_async_result_complete(result);
	g_object_unref(result);
#if 0
	pka_plugin_get_plugin_type_async(instance,
	                                 NULL,
	                                 pka_listener_plugin_get_plugin_type_cb,
	                                 result);
#endif
	EXIT;
}

/**
 * pk_connection_plugin_get_plugin_type_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @type: A #gint.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "plugin_get_plugin_type_finish" RPC.
 *
 * The plugin type.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_plugin_get_plugin_type_finish (PkaListener    *listener, /* IN */
                                            GAsyncResult   *result,   /* IN */
                                            gint           *type,     /* OUT */
                                            GError        **error)    /* OUT */
{
	ENTRY;
// TEMP TO TEST RPC RESULTS
	RETURN(TRUE);
#if 0
	GSimpleAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	ret = pka_plugin_get_plugin_type_finish(instance,
	                                        real_result,
	                                        type,
	                                        error);
	RETURN(ret);
#endif
}

#if 0
static void
pka_listener_plugin_get_version_cb (GObject      *listener,    /* IN */
                                    GAsyncResult *result,      /* IN */
                                    gpointer      user_data)   /* IN */
{
	GSimpleAsyncResult *real_result;

	g_return_if_fail(PKA_IS_LISTENER(listener));
	g_return_if_fail(RESULT_IS_VALID(plugin_get_version));

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	g_simple_async_result_set_op_res_gpointer(real_result, result);
	g_simple_async_result_complete(real_result);
	EXIT;
}
#endif

/**
 * pk_connection_plugin_get_version_async:
 * @connection: A #PkConnection.
 * @plugin: A #const gchar.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "plugin_get_version_async" RPC.  @callback
 * MUST call pka_listener_plugin_get_version_finish().
 *
 * The plugin version.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_plugin_get_version_async (PkaListener           *listener,    /* IN */
                                       const gchar           *plugin,      /* IN */
                                       GCancellable          *cancellable, /* IN */
                                       GAsyncReadyCallback    callback,    /* IN */
                                       gpointer               user_data)   /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_plugin_get_version_async);
// TEMP TO TEST RPC RESULTS
	g_simple_async_result_complete(result);
	g_object_unref(result);
#if 0
	pka_plugin_get_version_async(instance,
	                             NULL,
	                             pka_listener_plugin_get_version_cb,
	                             result);
#endif
	EXIT;
}

/**
 * pk_connection_plugin_get_version_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @version: A #gchar.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "plugin_get_version_finish" RPC.
 *
 * The plugin version.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_plugin_get_version_finish (PkaListener    *listener, /* IN */
                                        GAsyncResult   *result,   /* IN */
                                        gchar         **version,  /* OUT */
                                        GError        **error)    /* OUT */
{
	ENTRY;
// TEMP TO TEST RPC RESULTS
	RETURN(TRUE);
#if 0
	GSimpleAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	ret = pka_plugin_get_version_finish(instance,
	                                    real_result,
	                                    version,
	                                    error);
	RETURN(ret);
#endif
}

#if 0
static void
pka_listener_source_get_plugin_cb (GObject      *listener,    /* IN */
                                   GAsyncResult *result,      /* IN */
                                   gpointer      user_data)   /* IN */
{
	GSimpleAsyncResult *real_result;

	g_return_if_fail(PKA_IS_LISTENER(listener));
	g_return_if_fail(RESULT_IS_VALID(source_get_plugin));

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	g_simple_async_result_set_op_res_gpointer(real_result, result);
	g_simple_async_result_complete(real_result);
	EXIT;
}
#endif

/**
 * pk_connection_source_get_plugin_async:
 * @connection: A #PkConnection.
 * @source: A #gint.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "source_get_plugin_async" RPC.  @callback
 * MUST call pka_listener_source_get_plugin_finish().
 *
 * Retrieves the plugin for which the source originates.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_source_get_plugin_async (PkaListener           *listener,    /* IN */
                                      gint                   source,      /* IN */
                                      GCancellable          *cancellable, /* IN */
                                      GAsyncReadyCallback    callback,    /* IN */
                                      gpointer               user_data)   /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_source_get_plugin_async);
// TEMP TO TEST RPC RESULTS
	g_simple_async_result_complete(result);
	g_object_unref(result);
#if 0
	pka_source_get_plugin_async(instance,
	                            NULL,
	                            pka_listener_source_get_plugin_cb,
	                            result);
#endif
	EXIT;
}

/**
 * pk_connection_source_get_plugin_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @plugin: A #gchar.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "source_get_plugin_finish" RPC.
 *
 * Retrieves the plugin for which the source originates.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_source_get_plugin_finish (PkaListener    *listener, /* IN */
                                       GAsyncResult   *result,   /* IN */
                                       gchar         **plugin,   /* OUT */
                                       GError        **error)    /* OUT */
{
	ENTRY;
// TEMP TO TEST RPC RESULTS
	RETURN(TRUE);
#if 0
	GSimpleAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	ret = pka_source_get_plugin_finish(instance,
	                                   real_result,
	                                   plugin,
	                                   error);
	RETURN(ret);
#endif
}

#if 0
static void
pka_listener_subscription_add_channel_cb (GObject      *listener,    /* IN */
                                          GAsyncResult *result,      /* IN */
                                          gpointer      user_data)   /* IN */
{
	GSimpleAsyncResult *real_result;

	g_return_if_fail(PKA_IS_LISTENER(listener));
	g_return_if_fail(RESULT_IS_VALID(subscription_add_channel));

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	g_simple_async_result_set_op_res_gpointer(real_result, result);
	g_simple_async_result_complete(real_result);
	EXIT;
}
#endif

/**
 * pk_connection_subscription_add_channel_async:
 * @connection: A #PkConnection.
 * @subscription: A #gint.
 * @channel: A #gint.
 * @monitor: A #gboolean.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "subscription_add_channel_async" RPC.  @callback
 * MUST call pka_listener_subscription_add_channel_finish().
 *
 * Adds all sources of @channel to the list of sources for which manifest
 * and samples are delivered to the subscriber.
 * 
 * If @monitor is TRUE, then sources added to @channel will automatically
 * be added to the subscription.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_subscription_add_channel_async (PkaListener           *listener,     /* IN */
                                             gint                   subscription, /* IN */
                                             gint                   channel,      /* IN */
                                             gboolean               monitor,      /* IN */
                                             GCancellable          *cancellable,  /* IN */
                                             GAsyncReadyCallback    callback,     /* IN */
                                             gpointer               user_data)    /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_subscription_add_channel_async);
// TEMP TO TEST RPC RESULTS
	g_simple_async_result_complete(result);
	g_object_unref(result);
#if 0
	pka_subscription_add_channel_async(instance,
	                                   NULL,
	                                   pka_listener_subscription_add_channel_cb,
	                                   result);
#endif
	EXIT;
}

/**
 * pk_connection_subscription_add_channel_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "subscription_add_channel_finish" RPC.
 *
 * Adds all sources of @channel to the list of sources for which manifest
 * and samples are delivered to the subscriber.
 * 
 * If @monitor is TRUE, then sources added to @channel will automatically
 * be added to the subscription.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_subscription_add_channel_finish (PkaListener    *listener, /* IN */
                                              GAsyncResult   *result,   /* IN */
                                              GError        **error)    /* OUT */
{
	ENTRY;
// TEMP TO TEST RPC RESULTS
	RETURN(TRUE);
#if 0
	GSimpleAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	ret = pka_subscription_add_channel_finish(instance,
	                                          real_result,
	                                          error);
	RETURN(ret);
#endif
}

#if 0
static void
pka_listener_subscription_add_source_cb (GObject      *listener,    /* IN */
                                         GAsyncResult *result,      /* IN */
                                         gpointer      user_data)   /* IN */
{
	GSimpleAsyncResult *real_result;

	g_return_if_fail(PKA_IS_LISTENER(listener));
	g_return_if_fail(RESULT_IS_VALID(subscription_add_source));

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	g_simple_async_result_set_op_res_gpointer(real_result, result);
	g_simple_async_result_complete(real_result);
	EXIT;
}
#endif

/**
 * pk_connection_subscription_add_source_async:
 * @connection: A #PkConnection.
 * @subscription: A #gint.
 * @source: A #gint.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "subscription_add_source_async" RPC.  @callback
 * MUST call pka_listener_subscription_add_source_finish().
 *
 * Adds @source to the list of sources for which manifest and samples are
 * delivered to the subscriber.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_subscription_add_source_async (PkaListener           *listener,     /* IN */
                                            gint                   subscription, /* IN */
                                            gint                   source,       /* IN */
                                            GCancellable          *cancellable,  /* IN */
                                            GAsyncReadyCallback    callback,     /* IN */
                                            gpointer               user_data)    /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_subscription_add_source_async);
// TEMP TO TEST RPC RESULTS
	g_simple_async_result_complete(result);
	g_object_unref(result);
#if 0
	pka_subscription_add_source_async(instance,
	                                  NULL,
	                                  pka_listener_subscription_add_source_cb,
	                                  result);
#endif
	EXIT;
}

/**
 * pk_connection_subscription_add_source_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "subscription_add_source_finish" RPC.
 *
 * Adds @source to the list of sources for which manifest and samples are
 * delivered to the subscriber.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_subscription_add_source_finish (PkaListener    *listener, /* IN */
                                             GAsyncResult   *result,   /* IN */
                                             GError        **error)    /* OUT */
{
	ENTRY;
// TEMP TO TEST RPC RESULTS
	RETURN(TRUE);
#if 0
	GSimpleAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	ret = pka_subscription_add_source_finish(instance,
	                                         real_result,
	                                         error);
	RETURN(ret);
#endif
}

#if 0
static void
pka_listener_subscription_mute_cb (GObject      *listener,    /* IN */
                                   GAsyncResult *result,      /* IN */
                                   gpointer      user_data)   /* IN */
{
	GSimpleAsyncResult *real_result;

	g_return_if_fail(PKA_IS_LISTENER(listener));
	g_return_if_fail(RESULT_IS_VALID(subscription_mute));

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	g_simple_async_result_set_op_res_gpointer(real_result, result);
	g_simple_async_result_complete(real_result);
	g_object_unref(real_result);
	EXIT;
}
#endif

/**
 * pk_connection_subscription_mute_async:
 * @connection: A #PkConnection.
 * @subscription: A #gint.
 * @drain: A #gboolean.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "subscription_mute_async" RPC.  @callback
 * MUST call pka_listener_subscription_mute_finish().
 *
 * Prevents the subscription from further manifest or sample delivery.  If
 * @drain is set, the current buffer will be flushed.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_subscription_mute_async (PkaListener           *listener,     /* IN */
                                      gint                   subscription, /* IN */
                                      gboolean               drain,        /* IN */
                                      GCancellable          *cancellable,  /* IN */
                                      GAsyncReadyCallback    callback,     /* IN */
                                      gpointer               user_data)    /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_subscription_mute_async);
// TEMP TO TEST RPC RESULTS
	g_simple_async_result_complete(result);
	g_object_unref(result);
#if 0
	pka_subscription_mute_async(instance,
	                            NULL,
	                            pka_listener_subscription_mute_cb,
	                            result);
#endif
	EXIT;
}

/**
 * pk_connection_subscription_mute_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "subscription_mute_finish" RPC.
 *
 * Prevents the subscription from further manifest or sample delivery.  If
 * @drain is set, the current buffer will be flushed.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_subscription_mute_finish (PkaListener    *listener, /* IN */
                                       GAsyncResult   *result,   /* IN */
                                       GError        **error)    /* OUT */
{
	ENTRY;
// TEMP TO TEST RPC RESULTS
	RETURN(TRUE);
#if 0
	GSimpleAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	ret = pka_subscription_mute_finish(instance,
	                                   real_result,
	                                   error);
	RETURN(ret);
#endif
}

#if 0
static void
pka_listener_subscription_remove_channel_cb (GObject      *listener,    /* IN */
                                             GAsyncResult *result,      /* IN */
                                             gpointer      user_data)   /* IN */
{
	GSimpleAsyncResult *real_result;

	g_return_if_fail(PKA_IS_LISTENER(listener));
	g_return_if_fail(RESULT_IS_VALID(subscription_remove_channel));

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	g_simple_async_result_set_op_res_gpointer(real_result, result);
	g_simple_async_result_complete(real_result);
	EXIT;
}
#endif

/**
 * pk_connection_subscription_remove_channel_async:
 * @connection: A #PkConnection.
 * @subscription: A #gint.
 * @channel: A #gint.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "subscription_remove_channel_async" RPC.  @callback
 * MUST call pka_listener_subscription_remove_channel_finish().
 *
 * Removes @channel and all of its sources from the subscription.  This
 * prevents further manifest and sample delivery to the subscriber.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_subscription_remove_channel_async (PkaListener           *listener,     /* IN */
                                                gint                   subscription, /* IN */
                                                gint                   channel,      /* IN */
                                                GCancellable          *cancellable,  /* IN */
                                                GAsyncReadyCallback    callback,     /* IN */
                                                gpointer               user_data)    /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_subscription_remove_channel_async);
// TEMP TO TEST RPC RESULTS
	g_simple_async_result_complete(result);
	g_object_unref(result);
#if 0
	pka_subscription_remove_channel_async(instance,
	                                      NULL,
	                                      pka_listener_subscription_remove_channel_cb,
	                                      result);
#endif
	EXIT;
}

/**
 * pk_connection_subscription_remove_channel_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "subscription_remove_channel_finish" RPC.
 *
 * Removes @channel and all of its sources from the subscription.  This
 * prevents further manifest and sample delivery to the subscriber.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_subscription_remove_channel_finish (PkaListener    *listener, /* IN */
                                                 GAsyncResult   *result,   /* IN */
                                                 GError        **error)    /* OUT */
{
	ENTRY;
// TEMP TO TEST RPC RESULTS
	RETURN(TRUE);
#if 0
	GSimpleAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	ret = pka_subscription_remove_channel_finish(instance,
	                                             real_result,
	                                             error);
	RETURN(ret);
#endif
}

#if 0
static void
pka_listener_subscription_remove_source_cb (GObject      *listener,    /* IN */
                                            GAsyncResult *result,      /* IN */
                                            gpointer      user_data)   /* IN */
{
	GSimpleAsyncResult *real_result;

	g_return_if_fail(PKA_IS_LISTENER(listener));
	g_return_if_fail(RESULT_IS_VALID(subscription_remove_source));

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	g_simple_async_result_set_op_res_gpointer(real_result, result);
	g_simple_async_result_complete(real_result);
	EXIT;
}
#endif

/**
 * pk_connection_subscription_remove_source_async:
 * @connection: A #PkConnection.
 * @subscription: A #gint.
 * @source: A #gint.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "subscription_remove_source_async" RPC.  @callback
 * MUST call pka_listener_subscription_remove_source_finish().
 *
 * Removes @source from the subscription.  This prevents further manifest
 * and sample delivery to the subscriber.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_subscription_remove_source_async (PkaListener           *listener,     /* IN */
                                               gint                   subscription, /* IN */
                                               gint                   source,       /* IN */
                                               GCancellable          *cancellable,  /* IN */
                                               GAsyncReadyCallback    callback,     /* IN */
                                               gpointer               user_data)    /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_subscription_remove_source_async);
// TEMP TO TEST RPC RESULTS
	g_simple_async_result_complete(result);
	g_object_unref(result);
#if 0
	pka_subscription_remove_source_async(instance,
	                                     NULL,
	                                     pka_listener_subscription_remove_source_cb,
	                                     result);
#endif
	EXIT;
}

/**
 * pk_connection_subscription_remove_source_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "subscription_remove_source_finish" RPC.
 *
 * Removes @source from the subscription.  This prevents further manifest
 * and sample delivery to the subscriber.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_subscription_remove_source_finish (PkaListener    *listener, /* IN */
                                                GAsyncResult   *result,   /* IN */
                                                GError        **error)    /* OUT */
{
	ENTRY;
// TEMP TO TEST RPC RESULTS
	RETURN(TRUE);
#if 0
	GSimpleAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	ret = pka_subscription_remove_source_finish(instance,
	                                            real_result,
	                                            error);
	RETURN(ret);
#endif
}

#if 0
static void
pka_listener_subscription_set_buffer_cb (GObject      *listener,    /* IN */
                                         GAsyncResult *result,      /* IN */
                                         gpointer      user_data)   /* IN */
{
	GSimpleAsyncResult *real_result;

	g_return_if_fail(PKA_IS_LISTENER(listener));
	g_return_if_fail(RESULT_IS_VALID(subscription_set_buffer));

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	g_simple_async_result_set_op_res_gpointer(real_result, result);
	g_simple_async_result_complete(real_result);
	EXIT;
}
#endif

/**
 * pk_connection_subscription_set_buffer_async:
 * @connection: A #PkConnection.
 * @subscription: A #gint.
 * @timeout: A #gint.
 * @size: A #gint.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "subscription_set_buffer_async" RPC.  @callback
 * MUST call pka_listener_subscription_set_buffer_finish().
 *
 * Sets the buffering timeout and maximum buffer size for the subscription.
 * If @timeout milliseconds pass or @size bytes are consummed buffering,
 * the data will be delivered to the subscriber.
 * 
 * Set @timeout and @size to 0 to disable buffering.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_subscription_set_buffer_async (PkaListener           *listener,     /* IN */
                                            gint                   subscription, /* IN */
                                            gint                   timeout,      /* IN */
                                            gint                   size,         /* IN */
                                            GCancellable          *cancellable,  /* IN */
                                            GAsyncReadyCallback    callback,     /* IN */
                                            gpointer               user_data)    /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_subscription_set_buffer_async);
// TEMP TO TEST RPC RESULTS
	g_simple_async_result_complete(result);
	g_object_unref(result);
#if 0
	pka_subscription_set_buffer_async(instance,
	                                  NULL,
	                                  pka_listener_subscription_set_buffer_cb,
	                                  result);
#endif
	EXIT;
}

/**
 * pk_connection_subscription_set_buffer_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "subscription_set_buffer_finish" RPC.
 *
 * Sets the buffering timeout and maximum buffer size for the subscription.
 * If @timeout milliseconds pass or @size bytes are consummed buffering,
 * the data will be delivered to the subscriber.
 * 
 * Set @timeout and @size to 0 to disable buffering.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_subscription_set_buffer_finish (PkaListener    *listener, /* IN */
                                             GAsyncResult   *result,   /* IN */
                                             GError        **error)    /* OUT */
{
	ENTRY;
// TEMP TO TEST RPC RESULTS
	RETURN(TRUE);
#if 0
	GSimpleAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	ret = pka_subscription_set_buffer_finish(instance,
	                                         real_result,
	                                         error);
	RETURN(ret);
#endif
}

#if 0
static void
pka_listener_subscription_unmute_cb (GObject      *listener,    /* IN */
                                     GAsyncResult *result,      /* IN */
                                     gpointer      user_data)   /* IN */
{
	GSimpleAsyncResult *real_result;

	g_return_if_fail(PKA_IS_LISTENER(listener));
	g_return_if_fail(RESULT_IS_VALID(subscription_unmute));

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	g_simple_async_result_set_op_res_gpointer(real_result, result);
	g_simple_async_result_complete(real_result);
	g_object_unref(real_result);
	EXIT;
}
#endif

/**
 * pk_connection_subscription_unmute_async:
 * @connection: A #PkConnection.
 * @subscription: A #gint.
 * @cancellable: A #GCancellable.
 * @callback: A #GAsyncReadyCallback.
 * @user_data: A #gpointer.
 *
 * Asynchronously requests the "subscription_unmute_async" RPC.  @callback
 * MUST call pka_listener_subscription_unmute_finish().
 *
 * Enables the subscription for manifest and sample delivery.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_subscription_unmute_async (PkaListener           *listener,     /* IN */
                                        gint                   subscription, /* IN */
                                        GCancellable          *cancellable,  /* IN */
                                        GAsyncReadyCallback    callback,     /* IN */
                                        gpointer               user_data)    /* IN */
{
	GSimpleAsyncResult *result;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	result = g_simple_async_result_new(G_OBJECT(listener),
	                                   callback,
	                                   user_data,
	                                   pka_listener_subscription_unmute_async);
// TEMP TO TEST RPC RESULTS
	g_simple_async_result_complete(result);
	g_object_unref(result);
#if 0
	pka_subscription_unmute_async(instance,
	                              NULL,
	                              pka_listener_subscription_unmute_cb,
	                              result);
#endif
	EXIT;
}

/**
 * pk_connection_subscription_unmute_finish:
 * @connection: A #PkConnection.
 * @result: A #GAsyncResult.
 * @error: A #GError.
 *
 * Completes an asynchronous request for the "subscription_unmute_finish" RPC.
 *
 * Enables the subscription for manifest and sample delivery.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 * Side effects: None.
 */
gboolean
pka_listener_subscription_unmute_finish (PkaListener    *listener, /* IN */
                                         GAsyncResult   *result,   /* IN */
                                         GError        **error)    /* OUT */
{
	ENTRY;
// TEMP TO TEST RPC RESULTS
	RETURN(TRUE);
#if 0
	GSimpleAsyncResult *real_result;
	gboolean ret;

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);

	ENTRY;
	real_result = GET_RESULT_POINTER(result);
	ret = pka_subscription_unmute_finish(instance,
	                                     real_result,
	                                     error);
	RETURN(ret);
#endif
}

/**
 * pka_listener_plugin_added:
 * @listener: A #PkaListener.
 * @plugin: The plugin identifier.
 *
 * Notifies the #PkaListener that a Plugin has been added.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_plugin_added (PkaListener *listener, /* IN */
                           const gchar *plugin)   /* IN */
{
	g_return_if_fail(PKA_IS_LISTENER(listener));

	if (PKA_LISTENER_GET_CLASS(listener)->plugin_added) {
		PKA_LISTENER_GET_CLASS(listener)->plugin_added(listener, plugin);
	}
}

/**
 * pka_listener_plugin_removed:
 * @listener: A #PkaListener.
 * @plugin: The plugin identifier.
 *
 * Notifies the #PkaListener that a Plugin has been added.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_plugin_removed (PkaListener *listener, /* IN */
                             const gchar *plugin)   /* IN */
{
	g_return_if_fail(PKA_IS_LISTENER(listener));

	if (PKA_LISTENER_GET_CLASS(listener)->plugin_removed) {
		PKA_LISTENER_GET_CLASS(listener)->plugin_removed(listener, plugin);
	}
}

/**
 * pka_listener_encoder_added:
 * @listener: A #PkaListener.
 * @encoder: The encoder identifier.
 *
 * Notifies the #PkaListener that a Encoder has been added.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_encoder_added (PkaListener *listener, /* IN */
                            gint         encoder)  /* IN */
{
	g_return_if_fail(PKA_IS_LISTENER(listener));

	if (PKA_LISTENER_GET_CLASS(listener)->encoder_added) {
		PKA_LISTENER_GET_CLASS(listener)->encoder_added(listener, encoder);
	}
}

/**
 * pka_listener_encoder_removed:
 * @listener: A #PkaListener.
 * @encoder: The encoder identifier.
 *
 * Notifies the #PkaListener that a Encoder has been added.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_encoder_removed (PkaListener *listener, /* IN */
                              gint         encoder)  /* IN */
{
	g_return_if_fail(PKA_IS_LISTENER(listener));

	if (PKA_LISTENER_GET_CLASS(listener)->encoder_removed) {
		PKA_LISTENER_GET_CLASS(listener)->encoder_removed(listener, encoder);
	}
}

/**
 * pka_listener_source_added:
 * @listener: A #PkaListener.
 * @source: The source identifier.
 *
 * Notifies the #PkaListener that a Source has been added.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_source_added (PkaListener *listener, /* IN */
                           gint         source)   /* IN */
{
	g_return_if_fail(PKA_IS_LISTENER(listener));

	if (PKA_LISTENER_GET_CLASS(listener)->source_added) {
		PKA_LISTENER_GET_CLASS(listener)->source_added(listener, source);
	}
}

/**
 * pka_listener_source_removed:
 * @listener: A #PkaListener.
 * @source: The source identifier.
 *
 * Notifies the #PkaListener that a Source has been added.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_source_removed (PkaListener *listener, /* IN */
                             gint         source)   /* IN */
{
	g_return_if_fail(PKA_IS_LISTENER(listener));

	if (PKA_LISTENER_GET_CLASS(listener)->source_removed) {
		PKA_LISTENER_GET_CLASS(listener)->source_removed(listener, source);
	}
}

/**
 * pka_listener_channel_added:
 * @listener: A #PkaListener.
 * @channel: The channel identifier.
 *
 * Notifies the #PkaListener that a Channel has been added.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_channel_added (PkaListener *listener, /* IN */
                            gint         channel)  /* IN */
{
	g_return_if_fail(PKA_IS_LISTENER(listener));

	if (PKA_LISTENER_GET_CLASS(listener)->channel_added) {
		PKA_LISTENER_GET_CLASS(listener)->channel_added(listener, channel);
	}
}

/**
 * pka_listener_channel_removed:
 * @listener: A #PkaListener.
 * @channel: The channel identifier.
 *
 * Notifies the #PkaListener that a Channel has been added.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_channel_removed (PkaListener *listener, /* IN */
                              gint         channel)  /* IN */
{
	g_return_if_fail(PKA_IS_LISTENER(listener));

	if (PKA_LISTENER_GET_CLASS(listener)->channel_removed) {
		PKA_LISTENER_GET_CLASS(listener)->channel_removed(listener, channel);
	}
}

/**
 * pka_listener_subscription_added:
 * @listener: A #PkaListener.
 * @subscription: The subscription identifier.
 *
 * Notifies the #PkaListener that a Subscription has been added.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_subscription_added (PkaListener *listener,     /* IN */
                                 gint         subscription) /* IN */
{
	g_return_if_fail(PKA_IS_LISTENER(listener));

	if (PKA_LISTENER_GET_CLASS(listener)->subscription_added) {
		PKA_LISTENER_GET_CLASS(listener)->subscription_added(listener, subscription);
	}
}

/**
 * pka_listener_subscription_removed:
 * @listener: A #PkaListener.
 * @subscription: The subscription identifier.
 *
 * Notifies the #PkaListener that a Subscription has been added.
 *
 * Returns: None.
 * Side effects: None.
 */
void
pka_listener_subscription_removed (PkaListener *listener,     /* IN */
                                   gint         subscription) /* IN */
{
	g_return_if_fail(PKA_IS_LISTENER(listener));

	if (PKA_LISTENER_GET_CLASS(listener)->subscription_removed) {
		PKA_LISTENER_GET_CLASS(listener)->subscription_removed(listener, subscription);
	}
}

static void
pka_listener_class_init (PkaListenerClass *klass)
{
}

static void
pka_listener_init (PkaListener *listener)
{
}

/* pk-dbus.c
 *
 * Copyright (C) 2010 Christian Hergert
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <glib/gstdio.h>
#include <gmodule.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include <perfkit/perfkit-lowlevel.h>

#include "pk-dbus.h"
#include "pk-manager-dbus.h"
#include "pk-channel-dbus.h"
#include "pk-subscription-dbus.h"

#define ENSURE_CONNECTED(c, e) G_STMT_START {                   \
    if (PK_DBUS((c))->priv->manager == NULL) {                  \
        g_set_error((e), PK_DBUS_ERROR, PK_DBUS_ERROR_UNKNOWN,  \
                    "%s called while not connected!",           \
                    G_STRFUNC);                                 \
        return FALSE;                                           \
    }                                                           \
} G_STMT_END

G_DEFINE_TYPE(PkDbus, pk_dbus, PK_TYPE_CONNECTION)

struct _PkDbusPrivate
{
	DBusGConnection *dbus;
	DBusGProxy      *manager;
	DBusServer      *sub_server;
	gchar           *sub_addr;
	DBusGConnection *sub_conn;
};

static GArray *sub_map = NULL;
static guint sub_seq = 0;

static inline DBusGProxy*
channel_proxy_new(DBusGConnection *conn,
                  gint             channel_id)
{
	DBusGProxy *proxy;
	gchar *path;

	path = g_strdup_printf("/org/perfkit/Agent/Channels/%d", channel_id);
	proxy = dbus_g_proxy_new_for_name(conn, "org.perfkit.Agent", path,
	                                  "org.perfkit.Agent.Channel");
    g_free(path);

    return proxy;
}

static gboolean
pk_dbus_is_connected (PkConnection *connection)
{
	return (PK_DBUS(connection)->priv->dbus != NULL);
}

static gboolean
pk_dbus_manager_ping (PkConnection  *connection,
                      GTimeVal      *tv,
                      GError       **error)
{
	PkDbusPrivate *priv = PK_DBUS(connection)->priv;
	gchar *str = NULL;
	gboolean res;

	res = org_perfkit_Agent_Manager_ping(priv->manager, &str, error);
	if (res) {
		g_time_val_from_iso8601(str, tv);
	}
	return res;
}

static gboolean
pk_dbus_manager_get_version (PkConnection  *connection,
                             gchar        **version,
                             GError       **error)
{
	PkDbusPrivate *priv = PK_DBUS(connection)->priv;

	return org_perfkit_Agent_Manager_get_version(priv->manager, version, error);
}

static gboolean
pk_dbus_channel_get_state (PkConnection    *connection,
                           gint             channel_id,
                           PkChannelState  *state,
                           GError         **error)
{
	PkDbusPrivate *priv = PK_DBUS(connection)->priv;
	DBusGProxy *proxy;
	gboolean result;

	proxy = channel_proxy_new(priv->dbus, channel_id);
	result = org_perfkit_Agent_Channel_get_state(proxy,
	                                                 (gint *)state,
	                                                 error);
	g_object_unref(proxy);

	return result;
}

static gboolean
pk_dbus_channel_get_pid (PkConnection  *connection,
                         gint           channel_id,
                         GPid          *pid,
                         GError       **error)
{
	PkDbusPrivate *priv = PK_DBUS(connection)->priv;
	DBusGProxy *proxy;
	gboolean result;

	proxy = channel_proxy_new(priv->dbus, channel_id);
	result = org_perfkit_Agent_Channel_get_pid(proxy,
	                                               (gint *)pid,
	                                               error);
	g_object_unref(proxy);

	return result;
}

static gboolean
pk_dbus_channel_get_target (PkConnection  *connection,
                            gint           channel_id,
                            gchar        **target,
                            GError       **error)
{
	PkDbusPrivate *priv = PK_DBUS(connection)->priv;
	DBusGProxy *proxy;
	gboolean result;

	proxy = channel_proxy_new(priv->dbus, channel_id);
	result = org_perfkit_Agent_Channel_get_target(proxy,
	                                                  target,
	                                                  error);
	g_object_unref(proxy);

	return result;
}

static gboolean
pk_dbus_channel_get_working_dir (PkConnection  *connection,
                                 gint           channel_id,
                                 gchar        **working_dir,
                                 GError       **error)
{
	PkDbusPrivate *priv = PK_DBUS(connection)->priv;
	DBusGProxy *proxy;
	gboolean result;

	proxy = channel_proxy_new(priv->dbus, channel_id);
	result = org_perfkit_Agent_Channel_get_working_dir(proxy,
	                                                       working_dir,
	                                                       error);
	g_object_unref(proxy);

	return result;
}

static gboolean
pk_dbus_channel_get_args (PkConnection   *connection,
                          gint            channel_id,
                          gchar        ***args,
                          GError        **error)
{
	PkDbusPrivate *priv = PK_DBUS(connection)->priv;
	DBusGProxy *proxy;
	gboolean result;

	proxy = channel_proxy_new(priv->dbus, channel_id);
	result = org_perfkit_Agent_Channel_get_args(proxy, args, error);
	g_object_unref(proxy);

	return result;
}

static gboolean
pk_dbus_channel_get_env (PkConnection   *connection,
                         gint            channel_id,
                         gchar        ***env,
                         GError        **error)
{
	PkDbusPrivate *priv = PK_DBUS(connection)->priv;
	DBusGProxy *proxy;
	gboolean result;

	proxy = channel_proxy_new(priv->dbus, channel_id);
	result = org_perfkit_Agent_Channel_get_env(proxy, env, error);
	g_object_unref(proxy);

	return result;
}

static gboolean
pk_dbus_channel_start (PkConnection  *connection,
                       gint           channel_id,
                       GError       **error)
{
	PkDbusPrivate *priv = PK_DBUS(connection)->priv;
	DBusGProxy *proxy;
	gboolean result;

	proxy = channel_proxy_new(priv->dbus, channel_id);
	result = org_perfkit_Agent_Channel_start(proxy, error);
	g_object_unref(proxy);

	return result;
}

static gboolean
pk_dbus_channel_stop (PkConnection  *connection,
                      gint           channel_id,
                      gboolean       killpid,
                      GError       **error)
{
	PkDbusPrivate *priv = PK_DBUS(connection)->priv;
	DBusGProxy *proxy;
	gboolean result;

	proxy = channel_proxy_new(priv->dbus, channel_id);
	result = org_perfkit_Agent_Channel_stop(proxy, killpid, error);
	g_object_unref(proxy);

	return result;
}

static gboolean
pk_dbus_channel_pause (PkConnection  *connection,
                       gint           channel_id,
                       GError       **error)
{
	PkDbusPrivate *priv = PK_DBUS(connection)->priv;
	DBusGProxy *proxy;
	gboolean result;

	proxy = channel_proxy_new(priv->dbus, channel_id);
	result = org_perfkit_Agent_Channel_pause(proxy, error);
	g_object_unref(proxy);

	return result;
}

static gboolean
pk_dbus_channel_unpause (PkConnection  *connection,
                         gint           channel_id,
                         GError       **error)
{
	PkDbusPrivate *priv = PK_DBUS(connection)->priv;
	DBusGProxy *proxy;
	gboolean result;

	proxy = channel_proxy_new(priv->dbus, channel_id);
	result = org_perfkit_Agent_Channel_unpause(proxy, error);
	g_object_unref(proxy);

	return result;
}

static gboolean
pk_dbus_channel_remove_source (PkConnection  *connection,
                               gint           channel_id,
                               gint           source_id,
                               GError       **error)
{
	PkDbusPrivate *priv = PK_DBUS(connection)->priv;
	DBusGProxy *proxy;
	gboolean result;

	proxy = channel_proxy_new(priv->dbus, channel_id);
	result = org_perfkit_Agent_Channel_remove_source(proxy, source_id, error);
	g_object_unref(proxy);

	return result;
}

G_LOCK_DEFINE(delivery_socket);

#define PK_DBUS_ERROR (g_quark_from_static_string("pk-dbus-error-quark"))
#define PK_DBUS_ERROR_UNKNOWN (1)
#define HAS_INTERFACE(m,i)   (g_strcmp0(i, dbus_message_get_interface(m)) == 0)
#define IS_MEMBER_NAMED(m,i) (g_strcmp0(i, dbus_message_get_member(m)) == 0)
#define IS_SIGNATURE(m,i)    (g_strcmp0(i, dbus_message_get_signature(m)) == 0)

enum
{
	MSG_UNKNOWN,
	MSG_MANIFEST,
	MSG_SAMPLE,
};

static inline gint
get_msg_type (DBusMessage *msg)
{
	if (IS_MEMBER_NAMED(msg, "Manifest"))
		return MSG_MANIFEST;
	else if (IS_MEMBER_NAMED(msg, "Sample"))
		return MSG_SAMPLE;
	return MSG_UNKNOWN;
}

static DBusHandlerResult
handle_msg (DBusConnection *conn,
            DBusMessage    *msg,
            void           *data)
{
	PkConnection *pk_conn = data;
	DBusMessageIter iter, ar_iter;
	const guint8 *buffer = NULL;
	gsize length = 0;
	gint sub_id = 0;

	if (!HAS_INTERFACE(msg, "org.perfkit.Agent.Subscription")) {
		g_warning("Not subscription interface.");
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	if (sscanf(dbus_message_get_path(msg), "/Subscriptions/%d", &sub_id) != 1) {
		g_warning("Not subscription bound.");
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	if (sub_id > sub_map->len) {
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	/* get the converted sub identifier */
	sub_id = g_array_index(sub_map, gint, sub_id);

	if (!dbus_message_iter_init(msg, &iter)) {
		g_warning("No mem for iter.");
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY) {
		g_warning("Arg not array.");
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	if (dbus_message_iter_get_element_type(&iter) != DBUS_TYPE_BYTE) {
		g_warning("Element not byte.");
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	dbus_message_iter_recurse(&iter, &ar_iter);
	dbus_message_iter_get_fixed_array(&ar_iter, &buffer, (gint *)&length);

	switch (get_msg_type(msg)) {
	case MSG_MANIFEST:
		pk_connection_subscription_deliver_manifest(pk_conn, sub_id,
		                                            buffer, length);
		break;
	case MSG_SAMPLE:
		pk_connection_subscription_deliver_sample(pk_conn, sub_id,
		                                          buffer, length);
		break;
	default:
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusObjectPathVTable subs_vtable = { 
	NULL,
	handle_msg,
};

static void
pk_dbus_on_new_sub (DBusServer     *server,
                    DBusConnection *connection,
                    gpointer        data)
{
	dbus_connection_ref(connection);
	dbus_connection_setup_with_g_main(connection, NULL);
	dbus_connection_register_fallback(connection,
	                                  "/Subscriptions",
	                                  &subs_vtable,
	                                  data);
}

static gboolean
pk_dbus_manager_ensure_delivery_server (PkDbus  *dbus,
                                        GError **error)
{
	PkDbusPrivate *priv = dbus->priv;
	gboolean result = TRUE;
	gchar *addr, *path, *dir;
	DBusError derror;

	G_LOCK(delivery_socket);

	if (priv->sub_server) {
		goto unlock;
	}

	/*
	 * Create path to subscription server.
	 */
	path = g_strdup_printf("%s/perfkit-%s/%lu-%p.socket",
	                       g_get_tmp_dir(),
	                       g_get_user_name(),
	                       (gulong)getpid(),
	                       dbus);
	addr = g_strdup_printf("unix:path=%s", path);

	/*
	 * Make sure directory exists.
	 */
	dir = g_dirname(path);
	if (!g_file_test(dir, G_FILE_TEST_IS_DIR)) {
		if (g_mkdir(dir, 0700) != 0) {
			g_warning("Could not create temporary directory: %s", dir);
			result = FALSE;
			g_free(dir);
			g_free(path);
			g_free(addr);
			goto unlock;
		}
	}
	g_free(dir);
	g_free(path);

	/*
	 * Create D-BUS server.
	 */
	dbus_error_init(&derror);
	priv->sub_server = dbus_server_listen(addr, &derror);
	if (!priv->sub_server) {
		g_set_error(error, PK_DBUS_ERROR, PK_DBUS_ERROR_UNKNOWN,
		            "Error enabling delivery D-BUS server: %s",
		            derror.message);
		dbus_error_free(&derror);
		result = FALSE;
		goto unlock;
	}

	/*
	 * Attach D-BUS to main loop for event delivery.
	 */
	dbus_server_setup_with_g_main(priv->sub_server, g_main_context_default());

	/*
	 * Set new connection handler.
	 */
	dbus_server_set_new_connection_function(priv->sub_server,
	                                        pk_dbus_on_new_sub,
	                                        g_object_ref(dbus),
	                                        (DBusFreeFunction)g_object_unref);

	/*
	 * Open the connection and listen for events.
	 */
	priv->sub_conn = dbus_g_connection_open(addr, NULL);
	priv->sub_addr = addr;

	if (!priv->sub_conn) {
		g_warning("Error creating callback connection.");
	}

unlock:
	G_UNLOCK(delivery_socket);

	return result;
}

static gboolean
pk_dbus_manager_create_subscription (PkConnection  *connection,
                                     gint           channel_id,
                                     gsize          buffer_size,
                                     gulong         buffer_timeout,
                                     const gchar   *encoder_info_uid,
                                     gint          *subscription_id,
                                     GError       **error)
{
	PkDbusPrivate *priv = PK_DBUS(connection)->priv;
	gboolean result = FALSE;
	gchar *channel_path = NULL,
	      *encoder_path = NULL,
	      *sub_path     = NULL,
	      *sub_handle   = NULL;
	guint  sub_id;

	if (!sub_map) {
		sub_map = g_array_new(FALSE, FALSE, sizeof(gint));
	}

	/*
	 * Setup delivery unix socket if needed.
	 */
	if (!pk_dbus_manager_ensure_delivery_server (PK_DBUS(connection), error)) {
		return FALSE;
	}

	sub_id = g_atomic_int_exchange_and_add((gint *)&sub_seq, 1);

	/*
	 * Setup EncoderInfo and Channel paths.
	 */
	if (encoder_info_uid) {
		encoder_path = g_strdup_printf(
				"/org/perfkit/Agent/Plugins/Encoders/%s",
				encoder_info_uid);
	} else {
		encoder_path = g_strdup("/");
	}
	channel_path = g_strdup_printf("/org/perfkit/Agent/Channels/%d",
	                               channel_id);
	sub_path = g_strdup_printf("/Subscriptions/%d", sub_id);

	if (!org_perfkit_Agent_Manager_create_subscription(priv->manager,
	                                                       priv->sub_addr,
	                                                       sub_path,
	                                                       channel_path,
	                                                       buffer_size,
	                                                       buffer_timeout,
	                                                       encoder_path,
	                                                       &sub_handle,
	                                                       error)) {
	    goto cleanup;
	}

	if (sscanf(sub_handle,
	           "/org/perfkit/Agent/Subscriptions/%d",
	           subscription_id) != 1) {
		goto cleanup;
	}

	g_array_append_val(sub_map, *subscription_id);

	result = TRUE;

cleanup:
	g_free(sub_handle);
	g_free(channel_path);
	g_free(encoder_path);
	g_free(sub_path);
	return result;
}

static gboolean
pk_dbus_subscription_enable (PkConnection  *connection,
                             gint           subscription_id,
                             GError       **error)
{
	PkDbusPrivate *priv = PK_DBUS(connection)->priv;
	DBusGProxy *proxy;
	gchar *path;
	gboolean ret;

	path = g_strdup_printf("/org/perfkit/Agent/Subscriptions/%d",
	                       subscription_id);
	proxy = dbus_g_proxy_new_for_name(priv->dbus,
	                                  "org.perfkit.Agent",
	                                  path,
	                                  "org.perfkit.Agent.Subscription");
	g_free(path);
	ret = org_perfkit_Agent_Subscription_enable(proxy, error);
	g_object_unref(proxy);
	return ret;
}

static gboolean
pk_dbus_subscription_disable (PkConnection  *connection,
                              gint           subscription_id,
                              gboolean       drain,
                              GError       **error)
{
	PkDbusPrivate *priv = PK_DBUS(connection)->priv;
	DBusGProxy *proxy;
	gchar *path;
	gboolean ret;

	path = g_strdup_printf("/org/perfkit/Agent/Subscriptions/%d",
	                       subscription_id);
	proxy = dbus_g_proxy_new_for_name(priv->dbus,
	                                  "org.perfkit.Agent",
	                                  path,
	                                  "org.perfkit.Agent.Subscription");
	g_free(path);
	ret = org_perfkit_Agent_Subscription_disable(proxy, drain, error);
	g_object_unref(proxy);
	return ret;
}

static gboolean
pk_dbus_connect (PkConnection  *connection,
                 GError       **error)
{
	PkDbusPrivate *priv = ((PkDbus *)connection)->priv;

	priv->dbus = dbus_g_bus_get(DBUS_BUS_SESSION, error);
	priv->manager = dbus_g_proxy_new_for_name(priv->dbus,
			"org.perfkit.Agent",
			"/org/perfkit/Agent/Manager",
			"org.perfkit.Agent.Manager");

	return TRUE;
}

static gboolean
pk_dbus_manager_remove_channel (PkConnection  *connection,
                                gint           channel_id,
                                GError       **error)
{
	PkDbusPrivate *priv = ((PkDbus *)connection)->priv;
	gboolean ret;
	gchar *path;

	path = g_strdup_printf("/org/perfkit/Agent/Channels/%d", channel_id);
	ret = org_perfkit_Agent_Manager_remove_channel(priv->manager,
	                                                   path,
	                                                   error);
	g_free(path);

	return ret;
}

static void
pk_dbus_disconnect (PkConnection *connection)
{
	PkDbusPrivate *priv = ((PkDbus *)connection)->priv;

	if (priv->manager) {
		g_object_unref(priv->manager);
	}

	if (priv->dbus) {
		dbus_g_connection_unref(priv->dbus);
	}
}

static gboolean
pk_dbus_manager_get_channels (PkConnection  *connection,
                              gint         **channels,
                              gint          *n_channels,
                              GError       **error)
{
	PkDbusPrivate *priv = ((PkDbus *)connection)->priv;
	gchar **sc = NULL;
	gint l, i;

	g_return_val_if_fail(channels != NULL, FALSE);
	g_return_val_if_fail(n_channels != NULL, FALSE);

	if (!org_perfkit_Agent_Manager_get_channels(priv->manager, &sc, error))
	    return FALSE;

	if (!sc) {
		*channels = NULL;
		*n_channels = 0;
		return TRUE;
	}

	if ((l = g_strv_length(sc)) == 0) {
		*channels = NULL;
		*n_channels = 0;
	} else {
		*channels = g_malloc0(sizeof(gint) * l);
		*n_channels = l;

		for (i = 0; i < l; i++) {
			sscanf(sc[i], "/org/perfkit/Agent/Channels/%d", &(*channels)[i]);
		}
	}

	g_strfreev(sc);

	return TRUE;
}

static gboolean
pk_dbus_manager_create_channel (PkConnection  *connection,
                                PkSpawnInfo   *spawn_info,
                                gint          *channel_id,
                                GError       **error)
{
	PkDbusPrivate *priv = ((PkDbus *)connection)->priv;
	gchar *path = NULL;

	g_return_val_if_fail(spawn_info != NULL, FALSE);
	g_return_val_if_fail(channel_id != NULL, FALSE);

	if (org_perfkit_Agent_Manager_create_channel(priv->manager,
	                                                 spawn_info->pid,
	                                                 spawn_info->target,
	                                                 (const gchar **)spawn_info->args,
	                                                 (const gchar **)spawn_info->env,
	                                                 spawn_info->working_dir,
	                                                 &path,
	                                                 error))
	{
		sscanf(path, "/org/perfkit/Agent/Channels/%d", channel_id);
		g_free(path);
		return TRUE;
	}

	return FALSE;
}

static gboolean
pk_dbus_manager_get_source_plugins (PkConnection   *connection,
                                    gchar        ***plugins,
                                    GError        **error)
{
	PkDbusPrivate *priv = ((PkDbus *)connection)->priv;
	gchar **p, uid[128];
	gint i, j;

	ENSURE_CONNECTED(connection, error);

	if (!org_perfkit_Agent_Manager_get_source_plugins(priv->manager, &p, error)) {
		return FALSE;
	}

	*plugins = g_malloc0((g_strv_length(p) + 1) * sizeof(gchar*));

	for (i = 0, j = 0; p[i]; i++) {
		if (strlen(p[i]) > (sizeof(uid) - 1)) {
			continue;
		}
		memset(uid, 0, sizeof(uid));
		sscanf(p[i], "/org/perfkit/Agent/Plugins/Sources/%s", uid);
		(*plugins)[j++] = g_strdup(uid);
	}

	return TRUE;
}

static gboolean
pk_dbus_channel_add_source (PkConnection  *connection,
                            gint           channel_id,
                            const gchar   *source_type,
                            gint          *source_id,
                            GError       **error)
{
	PkDbusPrivate *priv = ((PkDbus *)connection)->priv;
	DBusGProxy *proxy;
	gchar *path = NULL, *spath;
	gboolean res;

	proxy = channel_proxy_new(priv->dbus, channel_id);
	spath = g_strdup_printf("/org/perfkit/Agent/Plugins/Sources/%s",
	                        source_type);
	res = org_perfkit_Agent_Channel_add_source(proxy, spath, &path, error);
	g_object_unref(proxy);
	g_free(spath);

	if (res) {
		if (sscanf(path, "/org/perfkit/Agent/Sources/%d", source_id) != 1) {
			res = FALSE;
		} else if ((*source_id) < 0) {
			res = FALSE;
		}
	}

	g_free(path);

	return res;
}

static void
pk_dbus_finalize (GObject *object)
{
	G_OBJECT_CLASS(pk_dbus_parent_class)->finalize(object);
}

static void
pk_dbus_class_init (PkDbusClass *klass)
{
	GObjectClass *object_class;
	PkConnectionClass *conn_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = pk_dbus_finalize;
	g_type_class_add_private(object_class, sizeof(PkDbusPrivate));

	conn_class = PK_CONNECTION_CLASS(klass);
	conn_class->connect = pk_dbus_connect;
	conn_class->disconnect = pk_dbus_disconnect;
	conn_class->is_connected = pk_dbus_is_connected;
	conn_class->manager_ping = pk_dbus_manager_ping;
	conn_class->channel_get_state = pk_dbus_channel_get_state;
	conn_class->channel_get_pid = pk_dbus_channel_get_pid;
	conn_class->channel_get_target = pk_dbus_channel_get_target;
	conn_class->channel_get_working_dir = pk_dbus_channel_get_working_dir;
	conn_class->channel_get_args = pk_dbus_channel_get_args;
	conn_class->channel_get_env = pk_dbus_channel_get_env;
	conn_class->channel_start = pk_dbus_channel_start;
	conn_class->channel_stop = pk_dbus_channel_stop;
	conn_class->channel_pause = pk_dbus_channel_pause;
	conn_class->channel_unpause = pk_dbus_channel_unpause;
	conn_class->channel_add_source = pk_dbus_channel_add_source;
	conn_class->channel_remove_source = pk_dbus_channel_remove_source;
	conn_class->manager_get_channels = pk_dbus_manager_get_channels;
	conn_class->manager_create_channel = pk_dbus_manager_create_channel;
	conn_class->manager_create_subscription = pk_dbus_manager_create_subscription;
	conn_class->manager_get_version = pk_dbus_manager_get_version;
	conn_class->manager_get_source_plugins = pk_dbus_manager_get_source_plugins;
	conn_class->manager_remove_channel = pk_dbus_manager_remove_channel;
	conn_class->subscription_enable = pk_dbus_subscription_enable;
	conn_class->subscription_disable = pk_dbus_subscription_disable;
}

static void
pk_dbus_init (PkDbus *dbus)
{
	dbus->priv = G_TYPE_INSTANCE_GET_PRIVATE(dbus,
	                                         PK_TYPE_DBUS,
	                                         PkDbusPrivate);
}

G_MODULE_EXPORT GType
pk_connection_plugin (void)
{
	return PK_TYPE_DBUS;
}

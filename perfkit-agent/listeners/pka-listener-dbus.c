/* pka-listener-dbus.c
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

#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <stdio.h>
#include <stdlib.h>

#include "pka-context.h"
#include "pka-listener-dbus.h"
#include "pka-log.h"

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "DBus"

/**
 * SECTION:pka-listener-dbus
 * @title: PkaListenerDBus
 * @short_description:
 *
 * Section overview.
 */

G_DEFINE_TYPE(PkaListenerDBus, pka_listener_dbus, PKA_TYPE_LISTENER)

#define IS_INTERFACE(_m, _i) (g_strcmp0(dbus_message_get_interface(_m), _i) == 0)
#define IS_MEMBER(_m, _i) (g_strcmp0(dbus_message_get_member(_m), _i) == 0)

struct _PkaListenerDBusPrivate
{
	DBusConnection *dbus;
	GMutex         *mutex;
	GHashTable     *peers;
	GStaticRWLock   handlers_lock;
	GHashTable     *handlers;
};

typedef struct
{
	gchar          *sender;
	gchar          *path;
	DBusConnection *connection;
} Peer;

typedef struct
{
	gint            subscription;
	DBusConnection *client;
	gchar          *path;
} Handler;

static void
peer_free (Peer *peer)
{
	g_free(peer->sender);
	g_free(peer->path);
	dbus_connection_unref(peer->connection);
	g_slice_free(Peer, peer);
}

static void
handler_free (Handler *handler)
{
	g_free(handler->path);
	dbus_connection_unref(handler->client);
	g_slice_free(Handler, handler);
}

static const gchar * PluginIntrospection =
	DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE
	"<node>"
	" <interface name=\"org.perfkit.Agent.Plugin\">"
	"  <method name=\"GetCopyright\">"
    "   <arg name=\"copyright\" direction=\"out\" type=\"s\"/>"
	"  </method>"
	"  <method name=\"GetDescription\">"
    "   <arg name=\"description\" direction=\"out\" type=\"s\"/>"
	"  </method>"
	"  <method name=\"GetName\">"
    "   <arg name=\"name\" direction=\"out\" type=\"s\"/>"
	"  </method>"
	"  <method name=\"GetType\">"
    "   <arg name=\"type\" direction=\"out\" type=\"i\"/>"
	"  </method>"
	"  <method name=\"GetVersion\">"
    "   <arg name=\"version\" direction=\"out\" type=\"s\"/>"
	"  </method>"
	" </interface>"
	" <interface name=\"org.freedesktop.DBus.Introspectable\">"
	"  <method name=\"Introspect\">"
	"   <arg name=\"data\" direction=\"out\" type=\"s\"/>"
	"  </method>"
	" </interface>"
	"</node>";

/**
 * pka_listener_dbus_plugin_get_copyright_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "plugin_get_copyright" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_plugin_get_copyright_cb (GObject      *listener,  /* IN */
                                           GAsyncResult *result,    /* IN */
                                           gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;
	gchar* copyright = NULL;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_plugin_get_copyright_finish(
			PKA_LISTENER(listener),
			result,
			&copyright,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		if (!copyright) {
			copyright = g_strdup("");
		}
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_STRING, &copyright,
		                         DBUS_TYPE_INVALID);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_plugin_get_description_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "plugin_get_description" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_plugin_get_description_cb (GObject      *listener,  /* IN */
                                             GAsyncResult *result,    /* IN */
                                             gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;
	gchar* description = NULL;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_plugin_get_description_finish(
			PKA_LISTENER(listener),
			result,
			&description,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		if (!description) {
			description = g_strdup("");
		}
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_STRING, &description,
		                         DBUS_TYPE_INVALID);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_plugin_get_name_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "plugin_get_name" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_plugin_get_name_cb (GObject      *listener,  /* IN */
                                      GAsyncResult *result,    /* IN */
                                      gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;
	gchar* name = NULL;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_plugin_get_name_finish(
			PKA_LISTENER(listener),
			result,
			&name,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		if (!name) {
			name = g_strdup("");
		}
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_STRING, &name,
		                         DBUS_TYPE_INVALID);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_plugin_get_plugin_type_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "plugin_get_plugin_type" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_plugin_get_plugin_type_cb (GObject      *listener,  /* IN */
                                             GAsyncResult *result,    /* IN */
                                             gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;
	gint type = 0;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_plugin_get_plugin_type_finish(
			PKA_LISTENER(listener),
			result,
			&type,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_INT32, &type,
		                         DBUS_TYPE_INVALID);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_plugin_get_version_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "plugin_get_version" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_plugin_get_version_cb (GObject      *listener,  /* IN */
                                         GAsyncResult *result,    /* IN */
                                         gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;
	gchar* version = NULL;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_plugin_get_version_finish(
			PKA_LISTENER(listener),
			result,
			&version,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		if (!version) {
			version = g_strdup("");
		}
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_STRING, &version,
		                         DBUS_TYPE_INVALID);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_handle_plugin_message:
 * @connection: A #DBusConnection.
 * @message: A #DBusMessage.
 * @user_data: user data provided to dbus_connection_register_object_path().
 *
 * Handler for incoming DBus messages destined for the Plugin object.
 *
 * Returns: DBUS_HANDLER_RESULT_HANDLED if the message was handled.
 * Side effects: None.
 */
static DBusHandlerResult
pka_listener_dbus_handle_plugin_message (DBusConnection *connection, /* IN */
                                         DBusMessage    *message,    /* IN */
                                         gpointer        user_data)  /* IN */
{
	PkaListenerDBus *listener = user_data;
	DBusHandlerResult ret = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	DBusMessage *reply = NULL;

	g_return_val_if_fail(listener != NULL, ret);

	ENTRY;
	if (dbus_message_is_method_call(message,
	                                "org.freedesktop.DBus.Introspectable",
	                                "Introspect")) {
		if (!(reply = dbus_message_new_method_return(message))) {
			GOTO(oom);
		}
		if (!dbus_message_append_args(reply,
		                              DBUS_TYPE_STRING,
		                              &PluginIntrospection,
		                              DBUS_TYPE_INVALID)) {
			GOTO(oom);
		}
		if (!dbus_connection_send(connection, reply, NULL)) {
			GOTO(oom);
		}
		ret = DBUS_HANDLER_RESULT_HANDLED;
	} else if (IS_INTERFACE(message, "org.perfkit.Agent.Plugin")) {
		if (IS_MEMBER(message, "GetCopyright")) {
			gchar* plugin = NULL;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Plugin/%as", &plugin) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_plugin_get_copyright_async(PKA_LISTENER(listener),
			                                        plugin,
			                                        NULL,
			                                        pka_listener_dbus_plugin_get_copyright_cb,
			                                        dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
			free(plugin);
		}
		else if (IS_MEMBER(message, "GetDescription")) {
			gchar* plugin = NULL;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Plugin/%as", &plugin) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_plugin_get_description_async(PKA_LISTENER(listener),
			                                          plugin,
			                                          NULL,
			                                          pka_listener_dbus_plugin_get_description_cb,
			                                          dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
			free(plugin);
		}
		else if (IS_MEMBER(message, "GetName")) {
			gchar* plugin = NULL;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Plugin/%as", &plugin) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_plugin_get_name_async(PKA_LISTENER(listener),
			                                   plugin,
			                                   NULL,
			                                   pka_listener_dbus_plugin_get_name_cb,
			                                   dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
			free(plugin);
		}
		else if (IS_MEMBER(message, "GetType")) {
			gchar* plugin = NULL;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Plugin/%as", &plugin) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_plugin_get_plugin_type_async(PKA_LISTENER(listener),
			                                          plugin,
			                                          NULL,
			                                          pka_listener_dbus_plugin_get_plugin_type_cb,
			                                          dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
			free(plugin);
		}
		else if (IS_MEMBER(message, "GetVersion")) {
			gchar* plugin = NULL;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Plugin/%as", &plugin) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_plugin_get_version_async(PKA_LISTENER(listener),
			                                      plugin,
			                                      NULL,
			                                      pka_listener_dbus_plugin_get_version_cb,
			                                      dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
			free(plugin);
		}
	}
oom:
	if (reply) {
		dbus_message_unref(reply);
	}
	RETURN(ret);
}

static const DBusObjectPathVTable PluginVTable = {
	.message_function = pka_listener_dbus_handle_plugin_message,
};

static const gchar * EncoderIntrospection =
	DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE
	"<node>"
	" <interface name=\"org.perfkit.Agent.Encoder\">"
	"  <method name=\"GetPlugin\">"
    "   <arg name=\"plugin\" direction=\"out\" type=\"o\"/>"
	"  </method>"
	" </interface>"
	" <interface name=\"org.freedesktop.DBus.Introspectable\">"
	"  <method name=\"Introspect\">"
	"   <arg name=\"data\" direction=\"out\" type=\"s\"/>"
	"  </method>"
	" </interface>"
	"</node>";

/**
 * pka_listener_dbus_encoder_get_plugin_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "encoder_get_plugin" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_encoder_get_plugin_cb (GObject      *listener,  /* IN */
                                         GAsyncResult *result,    /* IN */
                                         gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;
	gchar* plugin = NULL;
	gchar *plugin_path = NULL;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_encoder_get_plugin_finish(
			PKA_LISTENER(listener),
			result,
			&plugin,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		plugin_path = g_strdup_printf("/org/perfkit/Agent/Plugin/%s", plugin);
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_OBJECT_PATH, &plugin_path,
		                         DBUS_TYPE_INVALID);
		g_free(plugin_path);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_handle_encoder_message:
 * @connection: A #DBusConnection.
 * @message: A #DBusMessage.
 * @user_data: user data provided to dbus_connection_register_object_path().
 *
 * Handler for incoming DBus messages destined for the Encoder object.
 *
 * Returns: DBUS_HANDLER_RESULT_HANDLED if the message was handled.
 * Side effects: None.
 */
static DBusHandlerResult
pka_listener_dbus_handle_encoder_message (DBusConnection *connection, /* IN */
                                          DBusMessage    *message,    /* IN */
                                          gpointer        user_data)  /* IN */
{
	PkaListenerDBus *listener = user_data;
	DBusHandlerResult ret = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	DBusMessage *reply = NULL;

	g_return_val_if_fail(listener != NULL, ret);

	ENTRY;
	if (dbus_message_is_method_call(message,
	                                "org.freedesktop.DBus.Introspectable",
	                                "Introspect")) {
		if (!(reply = dbus_message_new_method_return(message))) {
			GOTO(oom);
		}
		if (!dbus_message_append_args(reply,
		                              DBUS_TYPE_STRING,
		                              &EncoderIntrospection,
		                              DBUS_TYPE_INVALID)) {
			GOTO(oom);
		}
		if (!dbus_connection_send(connection, reply, NULL)) {
			GOTO(oom);
		}
		ret = DBUS_HANDLER_RESULT_HANDLED;
	} else if (IS_INTERFACE(message, "org.perfkit.Agent.Encoder")) {
		if (IS_MEMBER(message, "GetPlugin")) {
			gint encoder = 0;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Encoder/%d", &encoder) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_encoder_get_plugin_async(PKA_LISTENER(listener),
			                                      encoder,
			                                      NULL,
			                                      pka_listener_dbus_encoder_get_plugin_cb,
			                                      dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
	}
oom:
	if (reply) {
		dbus_message_unref(reply);
	}
	RETURN(ret);
}

static const DBusObjectPathVTable EncoderVTable = {
	.message_function = pka_listener_dbus_handle_encoder_message,
};

static const gchar * SourceIntrospection =
	DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE
	"<node>"
	" <interface name=\"org.perfkit.Agent.Source\">"
	"  <method name=\"GetPlugin\">"
    "   <arg name=\"plugin\" direction=\"out\" type=\"o\"/>"
	"  </method>"
	" </interface>"
	" <interface name=\"org.freedesktop.DBus.Introspectable\">"
	"  <method name=\"Introspect\">"
	"   <arg name=\"data\" direction=\"out\" type=\"s\"/>"
	"  </method>"
	" </interface>"
	"</node>";

/**
 * pka_listener_dbus_source_get_plugin_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "source_get_plugin" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_source_get_plugin_cb (GObject      *listener,  /* IN */
                                        GAsyncResult *result,    /* IN */
                                        gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;
	gchar* plugin = NULL;
	gchar *plugin_path = NULL;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_source_get_plugin_finish(
			PKA_LISTENER(listener),
			result,
			&plugin,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		plugin_path = g_strdup_printf("/org/perfkit/Agent/Plugin/%s", plugin);
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_OBJECT_PATH, &plugin_path,
		                         DBUS_TYPE_INVALID);
		g_free(plugin_path);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_handle_source_message:
 * @connection: A #DBusConnection.
 * @message: A #DBusMessage.
 * @user_data: user data provided to dbus_connection_register_object_path().
 *
 * Handler for incoming DBus messages destined for the Source object.
 *
 * Returns: DBUS_HANDLER_RESULT_HANDLED if the message was handled.
 * Side effects: None.
 */
static DBusHandlerResult
pka_listener_dbus_handle_source_message (DBusConnection *connection, /* IN */
                                         DBusMessage    *message,    /* IN */
                                         gpointer        user_data)  /* IN */
{
	PkaListenerDBus *listener = user_data;
	DBusHandlerResult ret = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	DBusMessage *reply = NULL;

	g_return_val_if_fail(listener != NULL, ret);

	ENTRY;
	if (dbus_message_is_method_call(message,
	                                "org.freedesktop.DBus.Introspectable",
	                                "Introspect")) {
		if (!(reply = dbus_message_new_method_return(message))) {
			GOTO(oom);
		}
		if (!dbus_message_append_args(reply,
		                              DBUS_TYPE_STRING,
		                              &SourceIntrospection,
		                              DBUS_TYPE_INVALID)) {
			GOTO(oom);
		}
		if (!dbus_connection_send(connection, reply, NULL)) {
			GOTO(oom);
		}
		ret = DBUS_HANDLER_RESULT_HANDLED;
	} else if (IS_INTERFACE(message, "org.perfkit.Agent.Source")) {
		if (IS_MEMBER(message, "GetPlugin")) {
			gint source = 0;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Source/%d", &source) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_source_get_plugin_async(PKA_LISTENER(listener),
			                                     source,
			                                     NULL,
			                                     pka_listener_dbus_source_get_plugin_cb,
			                                     dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
	}
oom:
	if (reply) {
		dbus_message_unref(reply);
	}
	RETURN(ret);
}

static const DBusObjectPathVTable SourceVTable = {
	.message_function = pka_listener_dbus_handle_source_message,
};

static const gchar * ManagerIntrospection =
	DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE
	"<node>"
	" <interface name=\"org.perfkit.Agent.Manager\">"
	"  <method name=\"AddChannel\">"
    "   <arg name=\"channel\" direction=\"out\" type=\"o\"/>"
	"  </method>"
	"  <method name=\"AddSource\">"
    "   <arg name=\"plugin\" direction=\"in\" type=\"o\"/>"
    "   <arg name=\"source\" direction=\"out\" type=\"o\"/>"
	"  </method>"
	"  <method name=\"AddSubscription\">"
    "   <arg name=\"buffer_size\" direction=\"in\" type=\"u\"/>"
    "   <arg name=\"timeout\" direction=\"in\" type=\"u\"/>"
    "   <arg name=\"subscription\" direction=\"out\" type=\"o\"/>"
	"  </method>"
	"  <method name=\"GetChannels\">"
    "   <arg name=\"channels\" direction=\"out\" type=\"ao\"/>"
	"  </method>"
	"  <method name=\"GetHostname\">"
    "   <arg name=\"hostname\" direction=\"out\" type=\"s\"/>"
	"  </method>"
	"  <method name=\"GetPlugins\">"
    "   <arg name=\"plugins\" direction=\"out\" type=\"ao\"/>"
	"  </method>"
	"  <method name=\"GetSources\">"
    "   <arg name=\"sources\" direction=\"out\" type=\"ao\"/>"
	"  </method>"
	"  <method name=\"GetSubscriptions\">"
    "   <arg name=\"subscriptions\" direction=\"out\" type=\"ao\"/>"
	"  </method>"
	"  <method name=\"GetVersion\">"
    "   <arg name=\"version\" direction=\"out\" type=\"s\"/>"
	"  </method>"
	"  <method name=\"Ping\">"
    "   <arg name=\"tv\" direction=\"out\" type=\"s\"/>"
	"  </method>"
	"  <method name=\"RemoveChannel\">"
    "   <arg name=\"channel\" direction=\"in\" type=\"i\"/>"
    "   <arg name=\"removed\" direction=\"out\" type=\"b\"/>"
	"  </method>"
	"  <method name=\"RemoveSource\">"
    "   <arg name=\"source\" direction=\"in\" type=\"o\"/>"
	"  </method>"
	"  <method name=\"RemoveSubscription\">"
    "   <arg name=\"subscription\" direction=\"in\" type=\"o\"/>"
    "   <arg name=\"removed\" direction=\"out\" type=\"b\"/>"
	"  </method>"
	"  <method name=\"SetDeliveryPath\">"
	"   <arg name=\"path\" direction=\"in\" type=\"s\"/>"
	"  </method>"
	"  <signal name=\"PluginAdded\">"
	"   <arg name=\"path\" type=\"o\"/>"
	"  </signal>"
	"  <signal name=\"PluginRemoved\">"
	"   <arg name=\"path\" type=\"o\"/>"
	"  </signal>"
	"  <signal name=\"ChannelAdded\">"
	"   <arg name=\"path\" type=\"o\"/>"
	"  </signal>"
	"  <signal name=\"ChannelRemoved\">"
	"   <arg name=\"path\" type=\"o\"/>"
	"  </signal>"
	"  <signal name=\"SourceAdded\">"
	"   <arg name=\"path\" type=\"o\"/>"
	"  </signal>"
	"  <signal name=\"SourceRemoved\">"
	"   <arg name=\"path\" type=\"o\"/>"
	"  </signal>"
	"  <signal name=\"EncoderAdded\">"
	"   <arg name=\"path\" type=\"o\"/>"
	"  </signal>"
	"  <signal name=\"EncoderRemoved\">"
	"   <arg name=\"path\" type=\"o\"/>"
	"  </signal>"
	"  <signal name=\"SubscriptionAdded\">"
	"   <arg name=\"path\" type=\"o\"/>"
	"  </signal>"
	"  <signal name=\"SubscriptionRemoved\">"
	"   <arg name=\"path\" type=\"o\"/>"
	"  </signal>"
	" </interface>"
	" <interface name=\"org.freedesktop.DBus.Introspectable\">"
	"  <method name=\"Introspect\">"
	"   <arg name=\"data\" direction=\"out\" type=\"s\"/>"
	"  </method>"
	" </interface>"
	"</node>";

/**
 * pka_listener_dbus_manager_add_channel_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "manager_add_channel" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_manager_add_channel_cb (GObject      *listener,  /* IN */
                                          GAsyncResult *result,    /* IN */
                                          gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;
	gint channel = 0;
	gchar *channel_path = NULL;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_manager_add_channel_finish(
			PKA_LISTENER(listener),
			result,
			&channel,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		channel_path = g_strdup_printf("/org/perfkit/Agent/Channel/%d", channel);
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_OBJECT_PATH, &channel_path,
		                         DBUS_TYPE_INVALID);
		g_free(channel_path);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_manager_add_source_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "manager_add_source" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_manager_add_source_cb (GObject      *listener,  /* IN */
                                         GAsyncResult *result,    /* IN */
                                         gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;
	gint source = 0;
	gchar *source_path = NULL;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_manager_add_source_finish(
			PKA_LISTENER(listener),
			result,
			&source,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		source_path = g_strdup_printf("/org/perfkit/Agent/Source/%d", source);
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_OBJECT_PATH, &source_path,
		                         DBUS_TYPE_INVALID);
		g_free(source_path);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_manager_add_subscription_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "manager_add_subscription" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_manager_add_subscription_cb (GObject      *listener,  /* IN */
                                               GAsyncResult *result,    /* IN */
                                               gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;
	gint subscription = 0;
	gchar *subscription_path = NULL;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_manager_add_subscription_finish(
			PKA_LISTENER(listener),
			result,
			&subscription,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		subscription_path = g_strdup_printf("/org/perfkit/Agent/Subscription/%d", subscription);
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_OBJECT_PATH, &subscription_path,
		                         DBUS_TYPE_INVALID);
		g_free(subscription_path);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_manager_get_channels_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "manager_get_channels" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_manager_get_channels_cb (GObject      *listener,  /* IN */
                                           GAsyncResult *result,    /* IN */
                                           gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;
	gint* channels = NULL;
	gchar **channels_paths = NULL;
	gsize channels_len = 0;
	gint i;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_manager_get_channels_finish(
			PKA_LISTENER(listener),
			result,
			&channels,
			&channels_len,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		channels_paths = g_new0(gchar*, channels_len + 1);
		for (i = 0; i < channels_len; i++) {
			channels_paths[i] = g_strdup_printf("/org/perfkit/Agent/Channel/%d", channels[i]);
		}
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_ARRAY, DBUS_TYPE_OBJECT_PATH, &channels_paths, channels_len,

		                         DBUS_TYPE_INVALID);
		g_free(channels);
		g_strfreev(channels_paths);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_manager_get_hostname_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "manager_get_hostname" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_manager_get_hostname_cb (GObject      *listener,  /* IN */
                                           GAsyncResult *result,    /* IN */
                                           gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;
	gchar* hostname = NULL;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_manager_get_hostname_finish(
			PKA_LISTENER(listener),
			result,
			&hostname,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		if (!hostname) {
			hostname = g_strdup("");
		}
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_STRING, &hostname,
		                         DBUS_TYPE_INVALID);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_manager_get_plugins_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "manager_get_plugins" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_manager_get_plugins_cb (GObject      *listener,  /* IN */
                                          GAsyncResult *result,    /* IN */
                                          gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;
	gchar** plugins = NULL;
	gchar **plugins_paths = NULL;
	gint plugins_len;
	gint i;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_manager_get_plugins_finish(
			PKA_LISTENER(listener),
			result,
			&plugins,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		plugins_len = plugins ? g_strv_length(plugins) : 0;
		plugins_paths = g_new0(gchar*, plugins_len + 1);
		for (i = 0; i < plugins_len; i++) {
			plugins_paths[i] = g_strdup_printf("/org/perfkit/Agent/Plugin/%s", plugins[i]);
		}
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_ARRAY, DBUS_TYPE_OBJECT_PATH, &plugins_paths, plugins_len,
		                         DBUS_TYPE_INVALID);
		g_free(plugins);
		g_strfreev(plugins_paths);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_manager_get_sources_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "manager_get_sources" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_manager_get_sources_cb (GObject      *listener,  /* IN */
                                          GAsyncResult *result,    /* IN */
                                          gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;
	gint* sources = NULL;
	gchar **sources_paths = NULL;
	gsize sources_len = 0;
	gint i;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_manager_get_sources_finish(
			PKA_LISTENER(listener),
			result,
			&sources,
			&sources_len,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		sources_paths = g_new0(gchar*, sources_len + 1);
		for (i = 0; i < sources_len; i++) {
			sources_paths[i] = g_strdup_printf("/org/perfkit/Agent/Source/%d", sources[i]);
		}
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_ARRAY, DBUS_TYPE_OBJECT_PATH, &sources_paths, sources_len,

		                         DBUS_TYPE_INVALID);
		g_free(sources);
		g_strfreev(sources_paths);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_manager_get_subscriptions_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "manager_get_subscriptions" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_manager_get_subscriptions_cb (GObject      *listener,  /* IN */
                                                GAsyncResult *result,    /* IN */
                                                gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;
	gint* subscriptions = NULL;
	gchar **subscriptions_paths = NULL;
	gsize subscriptions_len = 0;
	gint i;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_manager_get_subscriptions_finish(
			PKA_LISTENER(listener),
			result,
			&subscriptions,
			&subscriptions_len,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		subscriptions_paths = g_new0(gchar*, subscriptions_len + 1);
		for (i = 0; i < subscriptions_len; i++) {
			subscriptions_paths[i] = g_strdup_printf("/org/perfkit/Agent/Subscription/%d", subscriptions[i]);
		}
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_ARRAY, DBUS_TYPE_OBJECT_PATH, &subscriptions_paths, subscriptions_len,

		                         DBUS_TYPE_INVALID);
		g_free(subscriptions);
		g_strfreev(subscriptions_paths);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_manager_get_version_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "manager_get_version" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_manager_get_version_cb (GObject      *listener,  /* IN */
                                          GAsyncResult *result,    /* IN */
                                          gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;
	gchar* version = NULL;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_manager_get_version_finish(
			PKA_LISTENER(listener),
			result,
			&version,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		if (!version) {
			version = g_strdup("");
		}
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_STRING, &version,
		                         DBUS_TYPE_INVALID);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_manager_ping_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "manager_ping" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_manager_ping_cb (GObject      *listener,  /* IN */
                                   GAsyncResult *result,    /* IN */
                                   gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;
	GTimeVal tv = { 0 };
	gchar *tv_str = NULL;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_manager_ping_finish(
			PKA_LISTENER(listener),
			result,
			&tv,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		tv_str = g_time_val_to_iso8601(&tv);
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_STRING, &tv_str,
		                         DBUS_TYPE_INVALID);
		g_free(tv_str);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_manager_remove_channel_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "manager_remove_channel" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_manager_remove_channel_cb (GObject      *listener,  /* IN */
                                             GAsyncResult *result,    /* IN */
                                             gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;
	gboolean removed = 0;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_manager_remove_channel_finish(
			PKA_LISTENER(listener),
			result,
			&removed,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_BOOLEAN, &removed,
		                         DBUS_TYPE_INVALID);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_manager_remove_source_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "manager_remove_source" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_manager_remove_source_cb (GObject      *listener,  /* IN */
                                            GAsyncResult *result,    /* IN */
                                            gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_manager_remove_source_finish(
			PKA_LISTENER(listener),
			result,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_INVALID);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_manager_remove_subscription_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "manager_remove_subscription" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_manager_remove_subscription_cb (GObject      *listener,  /* IN */
                                                  GAsyncResult *result,    /* IN */
                                                  gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;
	gboolean removed = 0;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_manager_remove_subscription_finish(
			PKA_LISTENER(listener),
			result,
			&removed,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_BOOLEAN, &removed,
		                         DBUS_TYPE_INVALID);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_set_delivery_path:
 * @listener: A #PkaListenerDBus.
 * @sender: The senders identifier on the bus.
 * @path: The DBus connection path.
 * @error: A location for a #GError, or %NULL.
 *
 * Sets the destination DBus connection to deliver samples and out of band
 * data to a client.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: A DBus connection may be opened.
 */
static gboolean
pka_listener_dbus_set_delivery_path (PkaListenerDBus  *listener, /* IN */
                                     const gchar      *sender,   /* IN */
                                     const gchar      *path,     /* IN */
                                     GError          **error)    /* OUT */
{
	PkaListenerDBusPrivate *priv;
	DBusConnection *connection;
	DBusError dbus_error = { 0 };
	Peer *peer;

	g_return_val_if_fail(PKA_IS_LISTENER_DBUS(listener), FALSE);
	g_return_val_if_fail(sender != NULL, FALSE);
	g_return_val_if_fail(path != NULL, FALSE);

	ENTRY;
	priv = listener->priv;
	if (!(connection = dbus_connection_open(path, &dbus_error))) {
		g_set_error(error, PKA_LISTENER_DBUS_ERROR,
		            PKA_LISTENER_DBUS_ERROR_NOT_AVAILABLE,
		            "%s: %s", dbus_error.name, dbus_error.message);
		dbus_error_free(&dbus_error);
		RETURN(FALSE);
	}
	peer = g_slice_new0(Peer);
	peer->sender = g_strdup(sender);
	peer->path = g_strdup(path);
	peer->connection = connection;
	g_mutex_lock(priv->mutex);
	g_hash_table_insert(priv->peers, g_strdup(sender), peer);
	g_mutex_unlock(priv->mutex);
	RETURN(TRUE);
}

/**
 * pka_listener_dbus_handle_manager_message:
 * @connection: A #DBusConnection.
 * @message: A #DBusMessage.
 * @user_data: user data provided to dbus_connection_register_object_path().
 *
 * Handler for incoming DBus messages destined for the Manager object.
 *
 * Returns: DBUS_HANDLER_RESULT_HANDLED if the message was handled.
 * Side effects: None.
 */
static DBusHandlerResult
pka_listener_dbus_handle_manager_message (DBusConnection *connection, /* IN */
                                          DBusMessage    *message,    /* IN */
                                          gpointer        user_data)  /* IN */
{
	PkaListenerDBus *listener = user_data;
	DBusHandlerResult ret = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	DBusMessage *reply = NULL;

	g_return_val_if_fail(listener != NULL, ret);

	ENTRY;
	if (dbus_message_is_method_call(message,
	                                "org.freedesktop.DBus.Introspectable",
	                                "Introspect")) {
		if (!(reply = dbus_message_new_method_return(message))) {
			GOTO(oom);
		}
		if (!dbus_message_append_args(reply,
		                              DBUS_TYPE_STRING,
		                              &ManagerIntrospection,
		                              DBUS_TYPE_INVALID)) {
			GOTO(oom);
		}
		if (!dbus_connection_send(connection, reply, NULL)) {
			GOTO(oom);
		}
		ret = DBUS_HANDLER_RESULT_HANDLED;
	} else if (IS_INTERFACE(message, "org.perfkit.Agent.Manager")) {
		if (IS_MEMBER(message, "AddChannel")) {
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_manager_add_channel_async(PKA_LISTENER(listener),
			                                       NULL,
			                                       pka_listener_dbus_manager_add_channel_cb,
			                                       dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "AddSource")) {
			gchar* plugin = NULL;
			gchar* plugin_path = NULL;
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_OBJECT_PATH, &plugin_path,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			if (sscanf(plugin_path, "/org/perfkit/Agent/Plugin/%as", &plugin) != 1) {
				goto oom;
			}
			pka_listener_manager_add_source_async(PKA_LISTENER(listener),
			                                      plugin,
			                                      NULL,
			                                      pka_listener_dbus_manager_add_source_cb,
			                                      dbus_message_ref(message));
			g_free(plugin);
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "AddSubscription")) {
			gsize buffer_size = 0;
			gsize timeout = 0;
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_manager_add_subscription_async(PKA_LISTENER(listener),
			                                            buffer_size,
			                                            timeout,
			                                            NULL,
			                                            pka_listener_dbus_manager_add_subscription_cb,
			                                            dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "GetChannels")) {
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_manager_get_channels_async(PKA_LISTENER(listener),
			                                        NULL,
			                                        pka_listener_dbus_manager_get_channels_cb,
			                                        dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "GetHostname")) {
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_manager_get_hostname_async(PKA_LISTENER(listener),
			                                        NULL,
			                                        pka_listener_dbus_manager_get_hostname_cb,
			                                        dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "GetPlugins")) {
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_manager_get_plugins_async(PKA_LISTENER(listener),
			                                       NULL,
			                                       pka_listener_dbus_manager_get_plugins_cb,
			                                       dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "GetSources")) {
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_manager_get_sources_async(PKA_LISTENER(listener),
			                                       NULL,
			                                       pka_listener_dbus_manager_get_sources_cb,
			                                       dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "GetSubscriptions")) {
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_manager_get_subscriptions_async(PKA_LISTENER(listener),
			                                             NULL,
			                                             pka_listener_dbus_manager_get_subscriptions_cb,
			                                             dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "GetVersion")) {
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_manager_get_version_async(PKA_LISTENER(listener),
			                                       NULL,
			                                       pka_listener_dbus_manager_get_version_cb,
			                                       dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "Ping")) {
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_manager_ping_async(PKA_LISTENER(listener),
			                                NULL,
			                                pka_listener_dbus_manager_ping_cb,
			                                dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "RemoveChannel")) {
			gint channel = 0;
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_manager_remove_channel_async(PKA_LISTENER(listener),
			                                          channel,
			                                          NULL,
			                                          pka_listener_dbus_manager_remove_channel_cb,
			                                          dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "RemoveSource")) {
			const gchar *path = NULL;
			gint source = 0;
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_OBJECT_PATH, &path,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			if (sscanf(path, "/org/perfkit/Agent/Source/%d", &source) != 1) {
				GOTO(oom);
			}
			pka_listener_manager_remove_source_async(PKA_LISTENER(listener),
			                                         source,
			                                         NULL,
			                                         pka_listener_dbus_manager_remove_source_cb,
			                                         dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "RemoveSubscription")) {
			gchar *path = NULL;
			gint subscription = 0;
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_OBJECT_PATH, &path,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			if (sscanf(path, "/org/perfkit/Agent/Subscription/%d", &subscription) != 1) {
				GOTO(oom);
			}
			pka_listener_manager_remove_subscription_async(PKA_LISTENER(listener),
			                                               subscription,
			                                               NULL,
			                                               pka_listener_dbus_manager_remove_subscription_cb,
			                                               dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "SetDeliveryPath")) {
			gchar *path = NULL;
			GError *error = NULL;
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_STRING, &path,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			if (!pka_listener_dbus_set_delivery_path(listener,
			                                         dbus_message_get_sender(message),
			                                         path,
			                                         &error)) {
				reply = dbus_message_new_error(message,
				                               DBUS_ERROR_FAILED,
				                               error->message);
				dbus_connection_send(connection, reply, NULL);
				g_error_free(error);
				GOTO(oom);
			}
			if (!(reply = dbus_message_new_method_return(message))) {
				GOTO(oom);
			}
			dbus_connection_send(connection, reply, NULL);
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
	}
oom:
	if (reply) {
		dbus_message_unref(reply);
	}
	RETURN(ret);
}

static const DBusObjectPathVTable ManagerVTable = {
	.message_function = pka_listener_dbus_handle_manager_message,
};

static const gchar * ChannelIntrospection =
	DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE
	"<node>"
	" <interface name=\"org.perfkit.Agent.Channel\">"
	"  <method name=\"AddSource\">"
    "   <arg name=\"source\" direction=\"in\" type=\"i\"/>"
	"  </method>"
	"  <method name=\"GetArgs\">"
    "   <arg name=\"args\" direction=\"out\" type=\"as\"/>"
	"  </method>"
	"  <method name=\"GetCreatedAt\">"
    "   <arg name=\"tv\" direction=\"out\" type=\"s\"/>"
	"  </method>"
	"  <method name=\"GetEnv\">"
    "   <arg name=\"env\" direction=\"out\" type=\"as\"/>"
	"  </method>"
	"  <method name=\"GetExitStatus\">"
    "   <arg name=\"exit_status\" direction=\"out\" type=\"i\"/>"
	"  </method>"
	"  <method name=\"GetKillPid\">"
    "   <arg name=\"kill_pid\" direction=\"out\" type=\"b\"/>"
	"  </method>"
	"  <method name=\"GetPid\">"
    "   <arg name=\"pid\" direction=\"out\" type=\"i\"/>"
	"  </method>"
	"  <method name=\"GetPidSet\">"
    "   <arg name=\"pid_set\" direction=\"out\" type=\"b\"/>"
	"  </method>"
	"  <method name=\"GetSources\">"
    "   <arg name=\"sources\" direction=\"out\" type=\"ao\"/>"
	"  </method>"
	"  <method name=\"GetState\">"
    "   <arg name=\"state\" direction=\"out\" type=\"i\"/>"
	"  </method>"
	"  <method name=\"GetTarget\">"
    "   <arg name=\"target\" direction=\"out\" type=\"s\"/>"
	"  </method>"
	"  <method name=\"GetWorkingDir\">"
    "   <arg name=\"working_dir\" direction=\"out\" type=\"s\"/>"
	"  </method>"
	"  <method name=\"Mute\">"
	"  </method>"
	"  <method name=\"SetArgs\">"
    "   <arg name=\"args\" direction=\"in\" type=\"as\"/>"
	"  </method>"
	"  <method name=\"SetEnv\">"
    "   <arg name=\"env\" direction=\"in\" type=\"as\"/>"
	"  </method>"
	"  <method name=\"SetKillPid\">"
    "   <arg name=\"kill_pid\" direction=\"in\" type=\"b\"/>"
	"  </method>"
	"  <method name=\"SetPid\">"
    "   <arg name=\"pid\" direction=\"in\" type=\"i\"/>"
	"  </method>"
	"  <method name=\"SetTarget\">"
    "   <arg name=\"target\" direction=\"in\" type=\"s\"/>"
	"  </method>"
	"  <method name=\"SetWorkingDir\">"
    "   <arg name=\"working_dir\" direction=\"in\" type=\"s\"/>"
	"  </method>"
	"  <method name=\"Start\">"
	"  </method>"
	"  <method name=\"Stop\">"
	"  </method>"
	"  <method name=\"Unmute\">"
	"  </method>"
	"  <signal name=\"Started\">"
	"  </signal>"
	"  <signal name=\"Stopped\">"
	"  </signal>"
	"  <signal name=\"Muted\">"
	"  </signal>"
	"  <signal name=\"Unmuted\">"
	"  </signal>"
	" </interface>"
	" <interface name=\"org.freedesktop.DBus.Introspectable\">"
	"  <method name=\"Introspect\">"
	"   <arg name=\"data\" direction=\"out\" type=\"s\"/>"
	"  </method>"
	" </interface>"
	"</node>";

/**
 * pka_listener_dbus_channel_add_source_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "channel_add_source" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_channel_add_source_cb (GObject      *listener,  /* IN */
                                         GAsyncResult *result,    /* IN */
                                         gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_channel_add_source_finish(
			PKA_LISTENER(listener),
			result,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_INVALID);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_channel_get_args_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "channel_get_args" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_channel_get_args_cb (GObject      *listener,  /* IN */
                                       GAsyncResult *result,    /* IN */
                                       gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;
	gchar** args = NULL;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_channel_get_args_finish(
			PKA_LISTENER(listener),
			result,
			&args,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &args, args ? g_strv_length(args) : 0,
		                         DBUS_TYPE_INVALID);
		g_free(args);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_channel_get_created_at_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "channel_get_created_at" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_channel_get_created_at_cb (GObject      *listener,  /* IN */
                                             GAsyncResult *result,    /* IN */
                                             gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;
	GTimeVal tv = { 0 };
	gchar *tv_str = NULL;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_channel_get_created_at_finish(
			PKA_LISTENER(listener),
			result,
			&tv,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		tv_str = g_time_val_to_iso8601(&tv);
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_STRING, &tv_str,
		                         DBUS_TYPE_INVALID);
		g_free(tv_str);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_channel_get_env_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "channel_get_env" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_channel_get_env_cb (GObject      *listener,  /* IN */
                                      GAsyncResult *result,    /* IN */
                                      gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;
	gchar** env = NULL;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_channel_get_env_finish(
			PKA_LISTENER(listener),
			result,
			&env,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &env, env ? g_strv_length(env) : 0,
		                         DBUS_TYPE_INVALID);
		g_free(env);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_channel_get_exit_status_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "channel_get_exit_status" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_channel_get_exit_status_cb (GObject      *listener,  /* IN */
                                              GAsyncResult *result,    /* IN */
                                              gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;
	gint exit_status = 0;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_channel_get_exit_status_finish(
			PKA_LISTENER(listener),
			result,
			&exit_status,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_INT32, &exit_status,
		                         DBUS_TYPE_INVALID);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_channel_get_kill_pid_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "channel_get_kill_pid" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_channel_get_kill_pid_cb (GObject      *listener,  /* IN */
                                           GAsyncResult *result,    /* IN */
                                           gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;
	gboolean kill_pid = 0;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_channel_get_kill_pid_finish(
			PKA_LISTENER(listener),
			result,
			&kill_pid,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_BOOLEAN, &kill_pid,
		                         DBUS_TYPE_INVALID);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_channel_get_pid_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "channel_get_pid" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_channel_get_pid_cb (GObject      *listener,  /* IN */
                                      GAsyncResult *result,    /* IN */
                                      gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;
	gint pid = 0;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_channel_get_pid_finish(
			PKA_LISTENER(listener),
			result,
			&pid,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_INT32, &pid,
		                         DBUS_TYPE_INVALID);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_channel_get_pid_set_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "channel_get_pid_set" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_channel_get_pid_set_cb (GObject      *listener,  /* IN */
                                          GAsyncResult *result,    /* IN */
                                          gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;
	gboolean pid_set = 0;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_channel_get_pid_set_finish(
			PKA_LISTENER(listener),
			result,
			&pid_set,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_BOOLEAN, &pid_set,
		                         DBUS_TYPE_INVALID);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_channel_get_sources_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "channel_get_sources" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_channel_get_sources_cb (GObject      *listener,  /* IN */
                                          GAsyncResult *result,    /* IN */
                                          gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;
	gint* sources = NULL;
	gchar **sources_paths = NULL;
	gsize sources_len = 0;
	gint i;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_channel_get_sources_finish(
			PKA_LISTENER(listener),
			result,
			&sources,
			&sources_len,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		sources_paths = g_new0(gchar*, sources_len + 1);
		for (i = 0; i < sources_len; i++) {
			sources_paths[i] = g_strdup_printf("/org/perfkit/Agent/Source/%d", sources[i]);
		}
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_ARRAY, DBUS_TYPE_OBJECT_PATH, &sources_paths, sources_len,

		                         DBUS_TYPE_INVALID);
		g_free(sources);
		g_strfreev(sources_paths);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_channel_get_state_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "channel_get_state" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_channel_get_state_cb (GObject      *listener,  /* IN */
                                        GAsyncResult *result,    /* IN */
                                        gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;
	gint state = 0;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_channel_get_state_finish(
			PKA_LISTENER(listener),
			result,
			&state,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_INT32, &state,
		                         DBUS_TYPE_INVALID);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_channel_get_target_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "channel_get_target" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_channel_get_target_cb (GObject      *listener,  /* IN */
                                         GAsyncResult *result,    /* IN */
                                         gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;
	gchar* target = NULL;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_channel_get_target_finish(
			PKA_LISTENER(listener),
			result,
			&target,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		if (!target) {
			target = g_strdup("");
		}
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_STRING, &target,
		                         DBUS_TYPE_INVALID);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_channel_get_working_dir_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "channel_get_working_dir" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_channel_get_working_dir_cb (GObject      *listener,  /* IN */
                                              GAsyncResult *result,    /* IN */
                                              gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;
	gchar* working_dir = NULL;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_channel_get_working_dir_finish(
			PKA_LISTENER(listener),
			result,
			&working_dir,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		if (!working_dir) {
			working_dir = g_strdup("");
		}
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_STRING, &working_dir,
		                         DBUS_TYPE_INVALID);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_channel_mute_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "channel_mute" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_channel_mute_cb (GObject      *listener,  /* IN */
                                   GAsyncResult *result,    /* IN */
                                   gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_channel_mute_finish(
			PKA_LISTENER(listener),
			result,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_INVALID);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_channel_set_args_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "channel_set_args" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_channel_set_args_cb (GObject      *listener,  /* IN */
                                       GAsyncResult *result,    /* IN */
                                       gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_channel_set_args_finish(
			PKA_LISTENER(listener),
			result,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_INVALID);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_channel_set_env_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "channel_set_env" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_channel_set_env_cb (GObject      *listener,  /* IN */
                                      GAsyncResult *result,    /* IN */
                                      gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_channel_set_env_finish(
			PKA_LISTENER(listener),
			result,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_INVALID);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_channel_set_kill_pid_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "channel_set_kill_pid" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_channel_set_kill_pid_cb (GObject      *listener,  /* IN */
                                           GAsyncResult *result,    /* IN */
                                           gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_channel_set_kill_pid_finish(
			PKA_LISTENER(listener),
			result,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_INVALID);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_channel_set_pid_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "channel_set_pid" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_channel_set_pid_cb (GObject      *listener,  /* IN */
                                      GAsyncResult *result,    /* IN */
                                      gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_channel_set_pid_finish(
			PKA_LISTENER(listener),
			result,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_INVALID);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_channel_set_target_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "channel_set_target" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_channel_set_target_cb (GObject      *listener,  /* IN */
                                         GAsyncResult *result,    /* IN */
                                         gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_channel_set_target_finish(
			PKA_LISTENER(listener),
			result,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_INVALID);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_channel_set_working_dir_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "channel_set_working_dir" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_channel_set_working_dir_cb (GObject      *listener,  /* IN */
                                              GAsyncResult *result,    /* IN */
                                              gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_channel_set_working_dir_finish(
			PKA_LISTENER(listener),
			result,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_INVALID);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_channel_start_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "channel_start" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_channel_start_cb (GObject      *listener,  /* IN */
                                    GAsyncResult *result,    /* IN */
                                    gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_channel_start_finish(
			PKA_LISTENER(listener),
			result,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_INVALID);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_channel_stop_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "channel_stop" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_channel_stop_cb (GObject      *listener,  /* IN */
                                   GAsyncResult *result,    /* IN */
                                   gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_channel_stop_finish(
			PKA_LISTENER(listener),
			result,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_INVALID);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_channel_unmute_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "channel_unmute" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_channel_unmute_cb (GObject      *listener,  /* IN */
                                     GAsyncResult *result,    /* IN */
                                     gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_channel_unmute_finish(
			PKA_LISTENER(listener),
			result,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_INVALID);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_handle_channel_message:
 * @connection: A #DBusConnection.
 * @message: A #DBusMessage.
 * @user_data: user data provided to dbus_connection_register_object_path().
 *
 * Handler for incoming DBus messages destined for the Channel object.
 *
 * Returns: DBUS_HANDLER_RESULT_HANDLED if the message was handled.
 * Side effects: None.
 */
static DBusHandlerResult
pka_listener_dbus_handle_channel_message (DBusConnection *connection, /* IN */
                                          DBusMessage    *message,    /* IN */
                                          gpointer        user_data)  /* IN */
{
	PkaListenerDBus *listener = user_data;
	DBusHandlerResult ret = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	DBusMessage *reply = NULL;

	g_return_val_if_fail(listener != NULL, ret);

	ENTRY;
	if (dbus_message_is_method_call(message,
	                                "org.freedesktop.DBus.Introspectable",
	                                "Introspect")) {
		if (!(reply = dbus_message_new_method_return(message))) {
			GOTO(oom);
		}
		if (!dbus_message_append_args(reply,
		                              DBUS_TYPE_STRING,
		                              &ChannelIntrospection,
		                              DBUS_TYPE_INVALID)) {
			GOTO(oom);
		}
		if (!dbus_connection_send(connection, reply, NULL)) {
			GOTO(oom);
		}
		ret = DBUS_HANDLER_RESULT_HANDLED;
	} else if (IS_INTERFACE(message, "org.perfkit.Agent.Channel")) {
		if (IS_MEMBER(message, "AddSource")) {
			gint channel = 0;
			gint source = 0;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Channel/%d", &channel) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INT32, &source,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_channel_add_source_async(PKA_LISTENER(listener),
			                                      channel,
			                                      source,
			                                      NULL,
			                                      pka_listener_dbus_channel_add_source_cb,
			                                      dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "GetArgs")) {
			gint channel = 0;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Channel/%d", &channel) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_channel_get_args_async(PKA_LISTENER(listener),
			                                    channel,
			                                    NULL,
			                                    pka_listener_dbus_channel_get_args_cb,
			                                    dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "GetCreatedAt")) {
			gint channel = 0;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Channel/%d", &channel) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_channel_get_created_at_async(PKA_LISTENER(listener),
			                                          channel,
			                                          NULL,
			                                          pka_listener_dbus_channel_get_created_at_cb,
			                                          dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "GetEnv")) {
			gint channel = 0;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Channel/%d", &channel) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_channel_get_env_async(PKA_LISTENER(listener),
			                                   channel,
			                                   NULL,
			                                   pka_listener_dbus_channel_get_env_cb,
			                                   dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "GetExitStatus")) {
			gint channel = 0;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Channel/%d", &channel) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_channel_get_exit_status_async(PKA_LISTENER(listener),
			                                           channel,
			                                           NULL,
			                                           pka_listener_dbus_channel_get_exit_status_cb,
			                                           dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "GetKillPid")) {
			gint channel = 0;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Channel/%d", &channel) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_channel_get_kill_pid_async(PKA_LISTENER(listener),
			                                        channel,
			                                        NULL,
			                                        pka_listener_dbus_channel_get_kill_pid_cb,
			                                        dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "GetPid")) {
			gint channel = 0;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Channel/%d", &channel) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_channel_get_pid_async(PKA_LISTENER(listener),
			                                   channel,
			                                   NULL,
			                                   pka_listener_dbus_channel_get_pid_cb,
			                                   dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "GetPidSet")) {
			gint channel = 0;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Channel/%d", &channel) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_channel_get_pid_set_async(PKA_LISTENER(listener),
			                                       channel,
			                                       NULL,
			                                       pka_listener_dbus_channel_get_pid_set_cb,
			                                       dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "GetSources")) {
			gint channel = 0;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Channel/%d", &channel) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_channel_get_sources_async(PKA_LISTENER(listener),
			                                       channel,
			                                       NULL,
			                                       pka_listener_dbus_channel_get_sources_cb,
			                                       dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "GetState")) {
			gint channel = 0;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Channel/%d", &channel) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_channel_get_state_async(PKA_LISTENER(listener),
			                                     channel,
			                                     NULL,
			                                     pka_listener_dbus_channel_get_state_cb,
			                                     dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "GetTarget")) {
			gint channel = 0;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Channel/%d", &channel) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_channel_get_target_async(PKA_LISTENER(listener),
			                                      channel,
			                                      NULL,
			                                      pka_listener_dbus_channel_get_target_cb,
			                                      dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "GetWorkingDir")) {
			gint channel = 0;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Channel/%d", &channel) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_channel_get_working_dir_async(PKA_LISTENER(listener),
			                                           channel,
			                                           NULL,
			                                           pka_listener_dbus_channel_get_working_dir_cb,
			                                           dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "Mute")) {
			gint channel = 0;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Channel/%d", &channel) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_channel_mute_async(PKA_LISTENER(listener),
			                                channel,
			                                NULL,
			                                pka_listener_dbus_channel_mute_cb,
			                                dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "SetArgs")) {
			gint channel = 0;
			gchar** args = NULL;
			gint args_len = 0;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Channel/%d", &channel) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &args, &args_len,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_channel_set_args_async(PKA_LISTENER(listener),
			                                    channel,
			                                    args,
			                                    NULL,
			                                    pka_listener_dbus_channel_set_args_cb,
			                                    dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
			dbus_free_string_array(args);
		}
		else if (IS_MEMBER(message, "SetEnv")) {
			gint channel = 0;
			gchar** env = NULL;
			gint env_len = 0;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Channel/%d", &channel) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &env, &env_len,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_channel_set_env_async(PKA_LISTENER(listener),
			                                   channel,
			                                   env,
			                                   NULL,
			                                   pka_listener_dbus_channel_set_env_cb,
			                                   dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
			dbus_free_string_array(env);
		}
		else if (IS_MEMBER(message, "SetKillPid")) {
			gint channel = 0;
			gboolean kill_pid = 0;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Channel/%d", &channel) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_BOOLEAN, &kill_pid,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_channel_set_kill_pid_async(PKA_LISTENER(listener),
			                                        channel,
			                                        kill_pid,
			                                        NULL,
			                                        pka_listener_dbus_channel_set_kill_pid_cb,
			                                        dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "SetPid")) {
			gint channel = 0;
			gint pid = 0;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Channel/%d", &channel) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INT32, &pid,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_channel_set_pid_async(PKA_LISTENER(listener),
			                                   channel,
			                                   pid,
			                                   NULL,
			                                   pka_listener_dbus_channel_set_pid_cb,
			                                   dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "SetTarget")) {
			gint channel = 0;
			gchar* target = NULL;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Channel/%d", &channel) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_STRING, &target,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_channel_set_target_async(PKA_LISTENER(listener),
			                                      channel,
			                                      target,
			                                      NULL,
			                                      pka_listener_dbus_channel_set_target_cb,
			                                      dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "SetWorkingDir")) {
			gint channel = 0;
			gchar* working_dir = NULL;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Channel/%d", &channel) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_STRING, &working_dir,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_channel_set_working_dir_async(PKA_LISTENER(listener),
			                                           channel,
			                                           working_dir,
			                                           NULL,
			                                           pka_listener_dbus_channel_set_working_dir_cb,
			                                           dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "Start")) {
			gint channel = 0;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Channel/%d", &channel) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_channel_start_async(PKA_LISTENER(listener),
			                                 channel,
			                                 NULL,
			                                 pka_listener_dbus_channel_start_cb,
			                                 dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "Stop")) {
			gint channel = 0;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Channel/%d", &channel) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_channel_stop_async(PKA_LISTENER(listener),
			                                channel,
			                                NULL,
			                                pka_listener_dbus_channel_stop_cb,
			                                dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "Unmute")) {
			gint channel = 0;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Channel/%d", &channel) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_channel_unmute_async(PKA_LISTENER(listener),
			                                  channel,
			                                  NULL,
			                                  pka_listener_dbus_channel_unmute_cb,
			                                  dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
	}
oom:
	if (reply) {
		dbus_message_unref(reply);
	}
	RETURN(ret);
}

static const DBusObjectPathVTable ChannelVTable = {
	.message_function = pka_listener_dbus_handle_channel_message,
};

static const gchar * SubscriptionIntrospection =
	DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE
	"<node>"
	" <interface name=\"org.perfkit.Agent.Subscription\">"
	"  <method name=\"AddChannel\">"
    "   <arg name=\"channel\" direction=\"in\" type=\"i\"/>"
    "   <arg name=\"monitor\" direction=\"in\" type=\"b\"/>"
	"  </method>"
	"  <method name=\"AddSource\">"
    "   <arg name=\"source\" direction=\"in\" type=\"i\"/>"
	"  </method>"
	"  <method name=\"GetBuffer\">"
    "   <arg name=\"timeout\" direction=\"out\" type=\"i\"/>"
    "   <arg name=\"size\" direction=\"out\" type=\"i\"/>"
	"  </method>"
	"  <method name=\"GetCreatedAt\">"
    "   <arg name=\"tv\" direction=\"out\" type=\"s\"/>"
	"  </method>"
	"  <method name=\"GetSources\">"
    "   <arg name=\"sources\" direction=\"out\" type=\"ao\"/>"
	"  </method>"
	"  <method name=\"Mute\">"
    "   <arg name=\"drain\" direction=\"in\" type=\"b\"/>"
	"  </method>"
	"  <method name=\"RemoveChannel\">"
    "   <arg name=\"channel\" direction=\"in\" type=\"i\"/>"
	"  </method>"
	"  <method name=\"RemoveSource\">"
    "   <arg name=\"source\" direction=\"in\" type=\"i\"/>"
	"  </method>"
	"  <method name=\"SetBuffer\">"
    "   <arg name=\"timeout\" direction=\"in\" type=\"i\"/>"
    "   <arg name=\"size\" direction=\"in\" type=\"i\"/>"
	"  </method>"
	"  <method name=\"SetEncoder\">"
    "   <arg name=\"encoder\" direction=\"in\" type=\"i\"/>"
	"  </method>"
	"  <method name=\"Unmute\">"
	"  </method>"
	" </interface>"
	" <interface name=\"org.freedesktop.DBus.Introspectable\">"
	"  <method name=\"Introspect\">"
	"   <arg name=\"data\" direction=\"out\" type=\"s\"/>"
	"  </method>"
	" </interface>"
	"</node>";

/**
 * pka_listener_dbus_subscription_add_channel_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "subscription_add_channel" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_subscription_add_channel_cb (GObject      *listener,  /* IN */
                                               GAsyncResult *result,    /* IN */
                                               gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_subscription_add_channel_finish(
			PKA_LISTENER(listener),
			result,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_INVALID);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_subscription_add_source_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "subscription_add_source" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_subscription_add_source_cb (GObject      *listener,  /* IN */
                                              GAsyncResult *result,    /* IN */
                                              gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_subscription_add_source_finish(
			PKA_LISTENER(listener),
			result,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_INVALID);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_subscription_get_buffer_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "subscription_get_buffer" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_subscription_get_buffer_cb (GObject      *listener,  /* IN */
                                              GAsyncResult *result,    /* IN */
                                              gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;
	gint timeout = 0;
	gint size = 0;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_subscription_get_buffer_finish(
			PKA_LISTENER(listener),
			result,
			&timeout,
			&size,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_INT32, &timeout,
		                         DBUS_TYPE_INT32, &size,
		                         DBUS_TYPE_INVALID);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_subscription_get_created_at_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "subscription_get_created_at" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_subscription_get_created_at_cb (GObject      *listener,  /* IN */
                                                  GAsyncResult *result,    /* IN */
                                                  gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;
	GTimeVal tv = { 0 };
	gchar *tv_str = NULL;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_subscription_get_created_at_finish(
			PKA_LISTENER(listener),
			result,
			&tv,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		tv_str = g_time_val_to_iso8601(&tv);
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_STRING, &tv_str,
		                         DBUS_TYPE_INVALID);
		g_free(tv_str);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_subscription_get_sources_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "subscription_get_sources" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_subscription_get_sources_cb (GObject      *listener,  /* IN */
                                               GAsyncResult *result,    /* IN */
                                               gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;
	gint* sources = NULL;
	gchar **sources_paths = NULL;
	gsize sources_len = 0;
	gint i;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_subscription_get_sources_finish(
			PKA_LISTENER(listener),
			result,
			&sources,
			&sources_len,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		sources_paths = g_new0(gchar*, sources_len + 1);
		for (i = 0; i < sources_len; i++) {
			sources_paths[i] = g_strdup_printf("/org/perfkit/Agent/Source/%d", sources[i]);
		}
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_ARRAY, DBUS_TYPE_OBJECT_PATH, &sources_paths, sources_len,

		                         DBUS_TYPE_INVALID);
		g_free(sources);
		g_strfreev(sources_paths);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_subscription_mute_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "subscription_mute" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_subscription_mute_cb (GObject      *listener,  /* IN */
                                        GAsyncResult *result,    /* IN */
                                        gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_subscription_mute_finish(
			PKA_LISTENER(listener),
			result,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_INVALID);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_subscription_remove_channel_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "subscription_remove_channel" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_subscription_remove_channel_cb (GObject      *listener,  /* IN */
                                                  GAsyncResult *result,    /* IN */
                                                  gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_subscription_remove_channel_finish(
			PKA_LISTENER(listener),
			result,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_INVALID);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_subscription_remove_source_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "subscription_remove_source" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_subscription_remove_source_cb (GObject      *listener,  /* IN */
                                                 GAsyncResult *result,    /* IN */
                                                 gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_subscription_remove_source_finish(
			PKA_LISTENER(listener),
			result,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_INVALID);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_subscription_set_buffer_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "subscription_set_buffer" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_subscription_set_buffer_cb (GObject      *listener,  /* IN */
                                              GAsyncResult *result,    /* IN */
                                              gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_subscription_set_buffer_finish(
			PKA_LISTENER(listener),
			result,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_INVALID);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_subscription_set_encoder_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "subscription_set_encoder" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_subscription_set_encoder_cb (GObject      *listener,  /* IN */
                                               GAsyncResult *result,    /* IN */
                                               gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_subscription_set_encoder_finish(
			PKA_LISTENER(listener),
			result,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_INVALID);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

/**
 * pka_listener_dbus_subscription_unmute_cb:
 * @listener: A #PkaListenerDBus.
 * @result: A #GAsyncResult.
 * @user_data: A #DBusMessage containing the incoming method call.
 *
 * Handles the completion of the "subscription_unmute" RPC.  A response
 * to the message is created and sent as a reply to the caller.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_subscription_unmute_cb (GObject      *listener,  /* IN */
                                          GAsyncResult *result,    /* IN */
                                          gpointer      user_data) /* IN */
{
	PkaListenerDBusPrivate *priv;
	DBusMessage *message = user_data;
	DBusMessage *reply = NULL;
	GError *error = NULL;

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!pka_listener_subscription_unmute_finish(
			PKA_LISTENER(listener),
			result,
			&error)) {
		reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
		                               error->message);
		g_error_free(error);
	} else {
		reply = dbus_message_new_method_return(message);
		dbus_message_append_args(reply,
		                         DBUS_TYPE_INVALID);
	}
	dbus_connection_send(priv->dbus, reply, NULL);
	dbus_message_unref(reply);
	dbus_message_unref(message);
	EXIT;
}

static void
pka_listener_dbus_dispatch_manifest (PkaSubscription *subscription, /* IN */
                                     const guint8    *data,         /* IN */
                                     gsize            data_len,     /* IN */
                                     gpointer         user_data)    /* IN */
{
	PkaListenerDBusPrivate *priv;
	Handler *handler;
	gint subscription_id;
	DBusMessage *message;

	g_return_if_fail(subscription != NULL);
	g_return_if_fail(data != NULL);
	g_return_if_fail(data_len > 0);
	g_return_if_fail(PKA_IS_LISTENER_DBUS(user_data));

	ENTRY;
	priv = PKA_LISTENER_DBUS(user_data)->priv;
	subscription_id = pka_subscription_get_id(subscription);
	if (!(handler = g_hash_table_lookup(priv->handlers, &subscription_id))) {
		WARNING(DBus, "Received manifest with no active handler for "
		              "subscription %d.", subscription_id);
		GOTO(oom);
	}
	if (!(message = dbus_message_new_method_call(NULL, handler->path,
	                                             "org.perfkit.Agent.Handler",
	                                             "SendManifest"))) {
		GOTO(oom);
	}
	if (!dbus_message_append_args(message,
	                              DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE, &data, data_len,
	                              DBUS_TYPE_INVALID)) {
	    dbus_message_unref(message);
		GOTO(oom);
	}
	/*
	 * TODO: Handle failure notification and detach subscription.
	 */
	dbus_connection_send(handler->client, message, NULL);
	dbus_message_unref(message);
  oom:
	EXIT;
}

static void
pka_listener_dbus_dispatch_sample (PkaSubscription *subscription, /* IN */
                                   const guint8    *data,         /* IN */
                                   gsize            data_len,     /* IN */
                                   gpointer         user_data)    /* IN */
{
	PkaListenerDBusPrivate *priv;
	Handler *handler;
	gint subscription_id;
	DBusMessage *message;

	g_return_if_fail(subscription != NULL);
	g_return_if_fail(data != NULL);
	g_return_if_fail(data_len > 0);
	g_return_if_fail(PKA_IS_LISTENER_DBUS(user_data));

	ENTRY;
	priv = PKA_LISTENER_DBUS(user_data)->priv;
	subscription_id = pka_subscription_get_id(subscription);
	if (!(handler = g_hash_table_lookup(priv->handlers, &subscription_id))) {
		WARNING(DBus, "Received sample with no active handler for "
		              "subscription %d.", subscription_id);
		GOTO(oom);
	}
	if (!(message = dbus_message_new_method_call(NULL, handler->path,
	                                             "org.perfkit.Agent.Handler",
	                                             "SendSample"))) {
		GOTO(oom);
	}
	if (!dbus_message_append_args(message,
	                              DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE, &data, data_len,
	                              DBUS_TYPE_INVALID)) {
		dbus_message_unref(message);
		GOTO(oom);
	}
	/*
	 * TODO: Handle failure notification and detach subscription.
	 */
	dbus_connection_send(handler->client, message, NULL);
	dbus_message_unref(message);
  oom:
	EXIT;
}

/**
 * pka_listener_dbus_handle_subscription_message:
 * @connection: A #DBusConnection.
 * @message: A #DBusMessage.
 * @user_data: user data provided to dbus_connection_register_object_path().
 *
 * Handler for incoming DBus messages destined for the Subscription object.
 *
 * Returns: DBUS_HANDLER_RESULT_HANDLED if the message was handled.
 * Side effects: None.
 */
static DBusHandlerResult
pka_listener_dbus_handle_subscription_message (DBusConnection *connection, /* IN */
                                               DBusMessage    *message,    /* IN */
                                               gpointer        user_data)  /* IN */
{
	PkaListenerDBusPrivate *priv;
	PkaListenerDBus *listener = user_data;
	DBusHandlerResult ret = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	DBusMessage *reply = NULL;

	g_return_val_if_fail(listener != NULL, ret);

	ENTRY;
	priv = listener->priv;
	if (dbus_message_is_method_call(message,
	                                "org.freedesktop.DBus.Introspectable",
	                                "Introspect")) {
		if (!(reply = dbus_message_new_method_return(message))) {
			GOTO(oom);
		}
		if (!dbus_message_append_args(reply,
		                              DBUS_TYPE_STRING,
		                              &SubscriptionIntrospection,
		                              DBUS_TYPE_INVALID)) {
			GOTO(oom);
		}
		if (!dbus_connection_send(connection, reply, NULL)) {
			GOTO(oom);
		}
		ret = DBUS_HANDLER_RESULT_HANDLED;
	} else if (IS_INTERFACE(message, "org.perfkit.Agent.Subscription")) {
		if (IS_MEMBER(message, "AddChannel")) {
			gint subscription = 0;
			gint channel = 0;
			gboolean monitor = 0;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Subscription/%d", &subscription) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INT32, &channel,
			                           DBUS_TYPE_BOOLEAN, &monitor,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_subscription_add_channel_async(PKA_LISTENER(listener),
			                                            subscription,
			                                            channel,
			                                            monitor,
			                                            NULL,
			                                            pka_listener_dbus_subscription_add_channel_cb,
			                                            dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "AddSource")) {
			gint subscription = 0;
			gint source = 0;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Subscription/%d", &subscription) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INT32, &source,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_subscription_add_source_async(PKA_LISTENER(listener),
			                                           subscription,
			                                           source,
			                                           NULL,
			                                           pka_listener_dbus_subscription_add_source_cb,
			                                           dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "GetBuffer")) {
			gint subscription = 0;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Subscription/%d", &subscription) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_subscription_get_buffer_async(PKA_LISTENER(listener),
			                                           subscription,
			                                           NULL,
			                                           pka_listener_dbus_subscription_get_buffer_cb,
			                                           dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "GetCreatedAt")) {
			gint subscription = 0;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Subscription/%d", &subscription) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_subscription_get_created_at_async(PKA_LISTENER(listener),
			                                               subscription,
			                                               NULL,
			                                               pka_listener_dbus_subscription_get_created_at_cb,
			                                               dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "GetSources")) {
			gint subscription = 0;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Subscription/%d", &subscription) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_subscription_get_sources_async(PKA_LISTENER(listener),
			                                            subscription,
			                                            NULL,
			                                            pka_listener_dbus_subscription_get_sources_cb,
			                                            dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "Mute")) {
			gint subscription = 0;
			gboolean drain = 0;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Subscription/%d", &subscription) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_BOOLEAN, &drain,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_subscription_mute_async(PKA_LISTENER(listener),
			                                     subscription,
			                                     drain,
			                                     NULL,
			                                     pka_listener_dbus_subscription_mute_cb,
			                                     dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "RemoveChannel")) {
			gint subscription = 0;
			gint channel = 0;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Subscription/%d", &subscription) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INT32, &channel,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_subscription_remove_channel_async(PKA_LISTENER(listener),
			                                               subscription,
			                                               channel,
			                                               NULL,
			                                               pka_listener_dbus_subscription_remove_channel_cb,
			                                               dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "RemoveSource")) {
			gint subscription = 0;
			gint source = 0;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Subscription/%d", &subscription) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INT32, &source,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_subscription_remove_source_async(PKA_LISTENER(listener),
			                                              subscription,
			                                              source,
			                                              NULL,
			                                              pka_listener_dbus_subscription_remove_source_cb,
			                                              dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "SetBuffer")) {
			gint subscription = 0;
			gint timeout = 0;
			gint size = 0;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Subscription/%d", &subscription) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INT32, &timeout,
			                           DBUS_TYPE_INT32, &size,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_subscription_set_buffer_async(PKA_LISTENER(listener),
			                                           subscription,
			                                           timeout,
			                                           size,
			                                           NULL,
			                                           pka_listener_dbus_subscription_set_buffer_cb,
			                                           dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "SetEncoder")) {
			gint subscription = 0;
			gint encoder = 0;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Subscription/%d", &subscription) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INT32, &encoder,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_subscription_set_encoder_async(PKA_LISTENER(listener),
			                                            subscription,
			                                            encoder,
			                                            NULL,
			                                            pka_listener_dbus_subscription_set_encoder_cb,
			                                            dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "SetHandler")) {
			gint subscription = 0;
			const gchar *handler_path = NULL;
			const gchar *dbus_path;
			Handler *handler = NULL;
			PkaSubscription *sub = NULL;
			GError *error = NULL;
			DBusConnection *client = NULL;
			Peer *peer = NULL;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Subscription/%d", &subscription) != 1) {
				GOTO(oom);
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_OBJECT_PATH, &handler_path,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			if (!pka_manager_find_subscription(pka_context_default(),
			                                   subscription, &sub, &error)) {
				reply = dbus_message_new_error(message, DBUS_ERROR_FAILED, error->message);
				dbus_connection_send(connection, reply, NULL);
				g_error_free(error);
				GOTO(oom);
			}
			g_mutex_lock(priv->mutex);
			peer = g_hash_table_lookup(priv->peers, dbus_message_get_sender(message));
			if (peer && peer->connection) {
				client = dbus_connection_ref(peer->connection);
			}
			g_mutex_unlock(priv->mutex);
			if (!peer || !peer->connection) {
				reply = dbus_message_new_error(message, DBUS_ERROR_FAILED,
				                               "Must call Manager::SetDeliveryPath before "
				                               "setting subscription handlers.");
				dbus_connection_send(connection, reply, NULL);
				GOTO(oom);
			}
			g_static_rw_lock_writer_lock(&priv->handlers_lock);
			handler = g_slice_new0(Handler);
			handler->subscription = subscription;
			handler->path = g_strdup(handler_path);
			handler->client = client;
			g_hash_table_insert(priv->handlers, &handler->subscription, handler);
			pka_subscription_set_handlers(sub,
			                              pka_context_default(),
			                              pka_listener_dbus_dispatch_manifest,
			                              g_object_ref(listener),
			                              g_object_unref,
			                              pka_listener_dbus_dispatch_sample,
			                              g_object_ref(listener),
			                              g_object_unref,
			                              NULL);
			g_static_rw_lock_writer_unlock(&priv->handlers_lock);
			pka_subscription_unref(sub);
			if (!(reply = dbus_message_new_method_return(message))) {
				GOTO(oom);
			}
			INFO(DBus, "Registered handler for subscription %d at %s%s.",
			     subscription, dbus_message_get_sender(message), handler_path);
			dbus_connection_send(connection, reply, NULL);
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
		else if (IS_MEMBER(message, "Unmute")) {
			gint subscription = 0;
			const gchar *dbus_path;

			dbus_path = dbus_message_get_path(message);
			if (sscanf(dbus_path, "/org/perfkit/Agent/Subscription/%d", &subscription) != 1) {
				goto oom;
			}
			if (!dbus_message_get_args(message, NULL,
			                           DBUS_TYPE_INVALID)) {
				GOTO(oom);
			}
			pka_listener_subscription_unmute_async(PKA_LISTENER(listener),
			                                       subscription,
			                                       NULL,
			                                       pka_listener_dbus_subscription_unmute_cb,
			                                       dbus_message_ref(message));
			ret = DBUS_HANDLER_RESULT_HANDLED;
		}
	}
oom:
	if (reply) {
		dbus_message_unref(reply);
	}
	RETURN(ret);
}

static const DBusObjectPathVTable SubscriptionVTable = {
	.message_function = pka_listener_dbus_handle_subscription_message,
};

static gboolean
pka_listener_dbus_register_singletons (PkaListenerDBus *listener) /* IN */
{
	PkaListenerDBusPrivate *priv = listener->priv;

	ENTRY;
	if (!dbus_connection_register_object_path(priv->dbus,
	                                          "/org/perfkit/Agent/Manager",
	                                          &ManagerVTable,
	                                          listener)) {
		RETURN(FALSE);
	}
	RETURN(TRUE);
}

/**
 * pka_listener_dbus_listen:
 * @listener: A #PkaListener.
 * @error: A location for a #GError, or %NULL.
 *
 * Starts the #PkaListenerDBus instance.  The DBus service name is acquired
 * and objects registered.
 *
 * Integration with the default #GMainLoop is established.
 *
 * Returns: %TRUE if the listener started listening; otherwise %FALSE.
 * Side effects: None.
 */
static gboolean
pka_listener_dbus_listen (PkaListener  *listener, /* IN */
                          GError      **error)    /* OUT */
{
	PkaListenerDBusPrivate *priv;
	DBusError dbus_error = { 0 };

	g_return_val_if_fail(PKA_IS_LISTENER(listener), FALSE);

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (priv->dbus) {
		g_set_error(error, PKA_LISTENER_DBUS_ERROR,
		            PKA_LISTENER_DBUS_ERROR_STATE,
		            "Listener already connected");
		RETURN(FALSE);
	}
	if (!(priv->dbus = dbus_bus_get(DBUS_BUS_SESSION, &dbus_error))) {
		g_set_error(error, PKA_LISTENER_DBUS_ERROR,
		            PKA_LISTENER_DBUS_ERROR_NOT_AVAILABLE,
		            "%s: %s", dbus_error.name, dbus_error.message);
		dbus_error_free(&dbus_error);
		RETURN(FALSE);
	}
	if (!pka_listener_dbus_register_singletons(PKA_LISTENER_DBUS(listener))) {
		RETURN(FALSE);
	}
	if (dbus_bus_request_name(priv->dbus, "org.perfkit.Agent",
	                          DBUS_NAME_FLAG_DO_NOT_QUEUE,
	                          NULL) == DBUS_REQUEST_NAME_REPLY_EXISTS) {
		g_set_error(error, PKA_LISTENER_DBUS_ERROR,
		            PKA_LISTENER_DBUS_ERROR_NOT_AVAILABLE,
		            "An existing instance of Perfkit was discovered");
		RETURN(FALSE);
	}
	dbus_connection_setup_with_g_main(priv->dbus, NULL);
	RETURN(TRUE);
}

/**
 * pka_listener_dbus_close:
 * @listener: A #PkaListener.
 *
 * Closes the #PkaListenerDBus by disconnecting from the DBus.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_close (PkaListener *listener) /* IN */
{
	PkaListenerDBusPrivate *priv;

	g_return_if_fail(PKA_IS_LISTENER(listener));

	ENTRY;
	priv = PKA_LISTENER_DBUS(listener)->priv;
	if (!priv->dbus) {
		EXIT;
	}
	dbus_connection_unref(priv->dbus);
	priv->dbus = NULL;
	EXIT;
}

/**
 * pka_listener_dbus_new:
 *
 * Creates a new instance of #PkaListenerDBus.
 *
 * Returns: the newly created instance of #PkaListenerDBus.
 * Side effects: None.
 */
PkaListenerDBus*
pka_listener_dbus_new (void)
{
	return g_object_new(PKA_TYPE_LISTENER_DBUS, NULL);
}

/**
 * pka_listener_dbus_error_quark:
 *
 * Retrieves the #GQuark for the #PkaListenerDBus error domain.
 *
 * Returns: A #GQuark.
 * Side effects: None.
 */
GQuark
pka_listener_dbus_error_quark (void)
{
	return g_quark_from_static_string("pka-listener-dbus-error-quark");
}

/**
 * pka_listener_dbus_plugin_added:
 * @listener: A #PkaListenerDBus.
 * @plugin: The plugin identifier.
 *
 * Notifies the #PkaListener that a Plugin has been added.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_plugin_added (PkaListener *listener, /* IN */
                                const gchar *plugin)   /* IN */
{
	PkaListenerDBusPrivate *priv = PKA_LISTENER_DBUS(listener)->priv;
	DBusMessage *message;
	gchar *path;

	ENTRY;
	path = g_strdup_printf("/org/perfkit/Agent/Plugin/%s", plugin);
	dbus_connection_register_object_path(priv->dbus,
	                                     path,
	                                     &PluginVTable,
	                                     listener);
	if (!(message = dbus_message_new_signal("/org/perfkit/Agent/Manager",
	                                        "org.perfkit.Agent.Manager",
	                                        "PluginAdded"))) {
		GOTO(failed);
	}
	dbus_message_append_args(message,
	                         DBUS_TYPE_OBJECT_PATH, &path,
	                         DBUS_TYPE_INVALID);
	if (!dbus_connection_send(priv->dbus, message, NULL)) {
		GOTO(failed);
	}
  failed:
  	if (message) {
  		dbus_message_unref(message);
	}
	g_free(path);
	EXIT;
}

/**
 * pka_listener_dbus_plugin_removed:
 * @listener: A #PkaListenerDBus.
 * @plugin: The plugin identifier.
 *
 * Notifies the #PkaListener that a Plugin has been added.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_plugin_removed (PkaListener *listener, /* IN */
                                  const gchar *plugin)   /* IN */
{
	PkaListenerDBusPrivate *priv = PKA_LISTENER_DBUS(listener)->priv;
	DBusMessage *message;
	gchar *path;

	ENTRY;
	path = g_strdup_printf("/org/perfkit/Agent/Plugin/%s", plugin);
	dbus_connection_unregister_object_path(priv->dbus, path);
	if (!(message = dbus_message_new_signal("/org/perfkit/Agent/Manager",
	                                        "org.perfkit.Agent.Manager",
	                                        "PluginRemoved"))) {
		GOTO(failed);
	}
	dbus_message_append_args(message,
	                         DBUS_TYPE_OBJECT_PATH, &path,
	                         DBUS_TYPE_INVALID);
	if (!dbus_connection_send(priv->dbus, message, NULL)) {
		GOTO(failed);
	}
  failed:
  	if (message) {
  		dbus_message_unref(message);
	}
	g_free(path);
	EXIT;
}

/**
 * pka_listener_dbus_encoder_added:
 * @listener: A #PkaListenerDBus.
 * @encoder: The encoder identifier.
 *
 * Notifies the #PkaListener that a Encoder has been added.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_encoder_added (PkaListener *listener, /* IN */
                                 gint         encoder)  /* IN */
{
	PkaListenerDBusPrivate *priv = PKA_LISTENER_DBUS(listener)->priv;
	DBusMessage *message;
	gchar *path;

	ENTRY;
	path = g_strdup_printf("/org/perfkit/Agent/Encoder/%d", encoder);
	dbus_connection_register_object_path(priv->dbus,
	                                     path,
	                                     &EncoderVTable,
	                                     listener);
	if (!(message = dbus_message_new_signal("/org/perfkit/Agent/Manager",
	                                        "org.perfkit.Agent.Manager",
	                                        "EncoderAdded"))) {
		GOTO(failed);
	}
	dbus_message_append_args(message,
	                         DBUS_TYPE_OBJECT_PATH, &path,
	                         DBUS_TYPE_INVALID);
	if (!dbus_connection_send(priv->dbus, message, NULL)) {
		GOTO(failed);
	}
  failed:
  	if (message) {
  		dbus_message_unref(message);
	}
	g_free(path);
	EXIT;
}

/**
 * pka_listener_dbus_encoder_removed:
 * @listener: A #PkaListenerDBus.
 * @encoder: The encoder identifier.
 *
 * Notifies the #PkaListener that a Encoder has been added.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_encoder_removed (PkaListener *listener, /* IN */
                                   gint         encoder)  /* IN */
{
	PkaListenerDBusPrivate *priv = PKA_LISTENER_DBUS(listener)->priv;
	DBusMessage *message;
	gchar *path;

	ENTRY;
	path = g_strdup_printf("/org/perfkit/Agent/Encoder/%d", encoder);
	dbus_connection_unregister_object_path(priv->dbus, path);
	if (!(message = dbus_message_new_signal("/org/perfkit/Agent/Manager",
	                                        "org.perfkit.Agent.Manager",
	                                        "EncoderRemoved"))) {
		GOTO(failed);
	}
	dbus_message_append_args(message,
	                         DBUS_TYPE_OBJECT_PATH, &path,
	                         DBUS_TYPE_INVALID);
	if (!dbus_connection_send(priv->dbus, message, NULL)) {
		GOTO(failed);
	}
  failed:
  	if (message) {
  		dbus_message_unref(message);
	}
	g_free(path);
	EXIT;
}

/**
 * pka_listener_dbus_source_added:
 * @listener: A #PkaListenerDBus.
 * @source: The source identifier.
 *
 * Notifies the #PkaListener that a Source has been added.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_source_added (PkaListener *listener, /* IN */
                                gint         source)   /* IN */
{
	PkaListenerDBusPrivate *priv = PKA_LISTENER_DBUS(listener)->priv;
	DBusMessage *message;
	gchar *path;

	ENTRY;
	path = g_strdup_printf("/org/perfkit/Agent/Source/%d", source);
	dbus_connection_register_object_path(priv->dbus,
	                                     path,
	                                     &SourceVTable,
	                                     listener);
	if (!(message = dbus_message_new_signal("/org/perfkit/Agent/Manager",
	                                        "org.perfkit.Agent.Manager",
	                                        "SourceAdded"))) {
		GOTO(failed);
	}
	dbus_message_append_args(message,
	                         DBUS_TYPE_OBJECT_PATH, &path,
	                         DBUS_TYPE_INVALID);
	if (!dbus_connection_send(priv->dbus, message, NULL)) {
		GOTO(failed);
	}
  failed:
  	if (message) {
  		dbus_message_unref(message);
	}
	g_free(path);
	EXIT;
}

/**
 * pka_listener_dbus_source_removed:
 * @listener: A #PkaListenerDBus.
 * @source: The source identifier.
 *
 * Notifies the #PkaListener that a Source has been added.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_source_removed (PkaListener *listener, /* IN */
                                  gint         source)   /* IN */
{
	PkaListenerDBusPrivate *priv = PKA_LISTENER_DBUS(listener)->priv;
	DBusMessage *message;
	gchar *path;

	ENTRY;
	path = g_strdup_printf("/org/perfkit/Agent/Source/%d", source);
	dbus_connection_unregister_object_path(priv->dbus, path);
	if (!(message = dbus_message_new_signal("/org/perfkit/Agent/Manager",
	                                        "org.perfkit.Agent.Manager",
	                                        "SourceRemoved"))) {
		GOTO(failed);
	}
	dbus_message_append_args(message,
	                         DBUS_TYPE_OBJECT_PATH, &path,
	                         DBUS_TYPE_INVALID);
	if (!dbus_connection_send(priv->dbus, message, NULL)) {
		GOTO(failed);
	}
  failed:
  	if (message) {
  		dbus_message_unref(message);
	}
	g_free(path);
	EXIT;
}

/**
 * pka_listener_dbus_channel_started:
 * @listener: (in): A #PkaListenerDbus.
 * @channel: (in): A #PkaChannel.
 *
 * Emits the "Started" event for the channel via DBus.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_channel_started (PkaListenerDBus *listener,
                                   PkaChannel      *channel)
{
	PkaListenerDBusPrivate *priv = PKA_LISTENER_DBUS(listener)->priv;
	DBusMessage *message = NULL;
	gint channel_id;
	gchar *path;

	ENTRY;
	channel_id = pka_channel_get_id(channel);
	path = g_strdup_printf("/org/perfkit/Agent/Channel/%d", channel_id);
	if (!(message = dbus_message_new_signal(path,
	                                        "org.perfkit.Agent.Channel",
	                                        "Started"))) {
		GOTO(failed);
	}
	if (!dbus_connection_send(priv->dbus, message, NULL)) {
		GOTO(failed);
	}
  failed:
  	if (message) {
  		dbus_message_unref(message);
	}
	g_free(path);
	EXIT;
}

/**
 * pka_listener_dbus_channel_stopped:
 * @listener: (in): A #PkaListenerDbus.
 * @channel: (in): A #PkaChannel.
 *
 * Emits the "Stopped" event for the channel via DBus.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_channel_stopped (PkaListener *listener,
                                   PkaChannel  *channel)
{
	PkaListenerDBusPrivate *priv = PKA_LISTENER_DBUS(listener)->priv;
	DBusMessage *message = NULL;
	gint channel_id;
	gchar *path;

	ENTRY;
	channel_id = pka_channel_get_id(channel);
	path = g_strdup_printf("/org/perfkit/Agent/Channel/%d", channel_id);
	if (!(message = dbus_message_new_signal(path,
	                                        "org.perfkit.Agent.Channel",
	                                        "Stopped"))) {
		GOTO(failed);
	}
	if (!dbus_connection_send(priv->dbus, message, NULL)) {
		GOTO(failed);
	}
  failed:
  	if (message) {
  		dbus_message_unref(message);
	}
	g_free(path);
	EXIT;
}

/**
 * pka_listener_dbus_channel_muted:
 * @listener: (in): A #PkaListenerDbus.
 * @channel: (in): A #PkaChannel.
 *
 * Emits the "Muted" event for the channel via DBus.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_channel_muted (PkaListener *listener,
                                 PkaChannel  *channel)
{
	PkaListenerDBusPrivate *priv = PKA_LISTENER_DBUS(listener)->priv;
	DBusMessage *message = NULL;
	gint channel_id;
	gchar *path;

	ENTRY;
	channel_id = pka_channel_get_id(channel);
	path = g_strdup_printf("/org/perfkit/Agent/Channel/%d", channel_id);
	if (!(message = dbus_message_new_signal(path,
	                                        "org.perfkit.Agent.Channel",
	                                        "Muted"))) {
		GOTO(failed);
	}
	if (!dbus_connection_send(priv->dbus, message, NULL)) {
		GOTO(failed);
	}
  failed:
  	if (message) {
  		dbus_message_unref(message);
	}
	g_free(path);
	EXIT;
}

/**
 * pka_listener_dbus_channel_unmuted:
 * @listener: (in): A #PkaListenerDbus.
 * @channel: (in): A #PkaChannel.
 *
 * Emits the "Unmuted" event for the channel via DBus.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_channel_unmuted (PkaListener *listener,
                                   PkaChannel  *channel)
{
	PkaListenerDBusPrivate *priv = PKA_LISTENER_DBUS(listener)->priv;
	DBusMessage *message = NULL;
	gint channel_id;
	gchar *path;

	ENTRY;
	channel_id = pka_channel_get_id(channel);
	path = g_strdup_printf("/org/perfkit/Agent/Channel/%d", channel_id);
	if (!(message = dbus_message_new_signal(path,
	                                        "org.perfkit.Agent.Channel",
	                                        "Unmuted"))) {
		GOTO(failed);
	}
	if (!dbus_connection_send(priv->dbus, message, NULL)) {
		GOTO(failed);
	}
  failed:
  	if (message) {
  		dbus_message_unref(message);
	}
	g_free(path);
	EXIT;
}

/**
 * pka_listener_dbus_channel_added:
 * @listener: A #PkaListenerDBus.
 * @channel: The channel identifier.
 *
 * Notifies the #PkaListener that a Channel has been added.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_channel_added (PkaListener *listener, /* IN */
                                 gint         channel)  /* IN */
{
	PkaListenerDBusPrivate *priv = PKA_LISTENER_DBUS(listener)->priv;
	DBusMessage *message = NULL;
	PkaChannel *channel_;
	gchar *path;

	ENTRY;
	path = g_strdup_printf("/org/perfkit/Agent/Channel/%d", channel);
	/*
	 * Register object on the DBus.
	 */
	dbus_connection_register_object_path(priv->dbus,
	                                     path,
	                                     &ChannelVTable,
	                                     listener);
	/*
	 * Signal observers of new channel.
	 */
	if (!(message = dbus_message_new_signal("/org/perfkit/Agent/Manager",
	                                        "org.perfkit.Agent.Manager",
	                                        "ChannelAdded"))) {
		GOTO(failed);
	}
	dbus_message_append_args(message,
	                         DBUS_TYPE_OBJECT_PATH, &path,
	                         DBUS_TYPE_INVALID);
	if (!dbus_connection_send(priv->dbus, message, NULL)) {
		GOTO(failed);
	}
  failed:
  	if (message) {
  		dbus_message_unref(message);
  		if (pka_manager_find_channel(pka_context_default(),
  		                             channel, &channel_, NULL)) {
  			g_signal_connect_swapped(channel_, "started",
  			                         G_CALLBACK(pka_listener_dbus_channel_started),
  			                         listener);
  			g_signal_connect_swapped(channel_, "stopped",
  			                         G_CALLBACK(pka_listener_dbus_channel_stopped),
  			                         listener);
  			g_signal_connect_swapped(channel_, "muted",
  			                         G_CALLBACK(pka_listener_dbus_channel_muted),
  			                         listener);
  			g_signal_connect_swapped(channel_, "unmuted",
  			                         G_CALLBACK(pka_listener_dbus_channel_unmuted),
  			                         listener);
  			g_object_unref(channel_);
		}
	}
	g_free(path);
	EXIT;
}

/**
 * pka_listener_dbus_channel_removed:
 * @listener: A #PkaListenerDBus.
 * @channel: The channel identifier.
 *
 * Notifies the #PkaListener that a Channel has been added.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_channel_removed (PkaListener *listener, /* IN */
                                   gint         channel)  /* IN */
{
	PkaListenerDBusPrivate *priv = PKA_LISTENER_DBUS(listener)->priv;
	DBusMessage *message;
	gchar *path;

	ENTRY;
	path = g_strdup_printf("/org/perfkit/Agent/Channel/%d", channel);
	dbus_connection_unregister_object_path(priv->dbus, path);
	if (!(message = dbus_message_new_signal("/org/perfkit/Agent/Manager",
	                                        "org.perfkit.Agent.Manager",
	                                        "ChannelRemoved"))) {
		GOTO(failed);
	}
	dbus_message_append_args(message,
	                         DBUS_TYPE_OBJECT_PATH, &path,
	                         DBUS_TYPE_INVALID);
	if (!dbus_connection_send(priv->dbus, message, NULL)) {
		GOTO(failed);
	}
  failed:
  	if (message) {
  		dbus_message_unref(message);
	}
	g_free(path);
	EXIT;
}

/**
 * pka_listener_dbus_subscription_added:
 * @listener: A #PkaListenerDBus.
 * @subscription: The subscription identifier.
 *
 * Notifies the #PkaListener that a Subscription has been added.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_subscription_added (PkaListener *listener,     /* IN */
                                      gint         subscription) /* IN */
{
	PkaListenerDBusPrivate *priv = PKA_LISTENER_DBUS(listener)->priv;
	DBusMessage *message;
	gchar *path;

	ENTRY;
	path = g_strdup_printf("/org/perfkit/Agent/Subscription/%d", subscription);
	dbus_connection_register_object_path(priv->dbus,
	                                     path,
	                                     &SubscriptionVTable,
	                                     listener);
	if (!(message = dbus_message_new_signal("/org/perfkit/Agent/Manager",
	                                        "org.perfkit.Agent.Manager",
	                                        "SubscriptionAdded"))) {
		GOTO(failed);
	}
	dbus_message_append_args(message,
	                         DBUS_TYPE_OBJECT_PATH, &path,
	                         DBUS_TYPE_INVALID);
	if (!dbus_connection_send(priv->dbus, message, NULL)) {
		GOTO(failed);
	}
  failed:
  	if (message) {
  		dbus_message_unref(message);
	}
	g_free(path);
	EXIT;
}

/**
 * pka_listener_dbus_subscription_removed:
 * @listener: A #PkaListenerDBus.
 * @subscription: The subscription identifier.
 *
 * Notifies the #PkaListener that a Subscription has been added.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_subscription_removed (PkaListener *listener,     /* IN */
                                        gint         subscription) /* IN */
{
	PkaListenerDBusPrivate *priv = PKA_LISTENER_DBUS(listener)->priv;
	DBusMessage *message;
	gchar *path;

	ENTRY;
	path = g_strdup_printf("/org/perfkit/Agent/Subscription/%d", subscription);
	dbus_connection_unregister_object_path(priv->dbus, path);
	if (!(message = dbus_message_new_signal("/org/perfkit/Agent/Manager",
	                                        "org.perfkit.Agent.Manager",
	                                        "SubscriptionRemoved"))) {
		GOTO(failed);
	}
	dbus_message_append_args(message,
	                         DBUS_TYPE_OBJECT_PATH, &path,
	                         DBUS_TYPE_INVALID);
	if (!dbus_connection_send(priv->dbus, message, NULL)) {
		GOTO(failed);
	}
  failed:
  	if (message) {
  		dbus_message_unref(message);
	}
	g_free(path);
	EXIT;
}

/**
 * pka_listener_dbus_finalize:
 * @object: A #PkaListenerDBus.
 *
 * Finalizes the listener and releases allocated resources.
 *
 * Returns: None.
 * Side effects: Everything.
 */
static void
pka_listener_dbus_finalize (GObject *object)
{
	G_OBJECT_CLASS(pka_listener_dbus_parent_class)->finalize(object);
}

/**
 * pka_listener_dbus_class_init:
 * @klass: A #PkaListenerClass.
 *
 * Initializes the #PkaListenerDBusClass class.
 *
 * Returns: None.
 * Side effects: Class is initialized and VTable set.
 */
static void
pka_listener_dbus_class_init (PkaListenerDBusClass *klass)
{
	GObjectClass *object_class;
	PkaListenerClass *listener_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = pka_listener_dbus_finalize;
	g_type_class_add_private(object_class, sizeof(PkaListenerDBusPrivate));

	listener_class = PKA_LISTENER_CLASS(klass);
	listener_class->listen = pka_listener_dbus_listen;
	listener_class->close = pka_listener_dbus_close;
	listener_class->plugin_added = pka_listener_dbus_plugin_added;
	listener_class->plugin_removed = pka_listener_dbus_plugin_removed;
	listener_class->encoder_added = pka_listener_dbus_encoder_added;
	listener_class->encoder_removed = pka_listener_dbus_encoder_removed;
	listener_class->source_added = pka_listener_dbus_source_added;
	listener_class->source_removed = pka_listener_dbus_source_removed;
	listener_class->channel_added = pka_listener_dbus_channel_added;
	listener_class->channel_removed = pka_listener_dbus_channel_removed;
	listener_class->subscription_added = pka_listener_dbus_subscription_added;
	listener_class->subscription_removed = pka_listener_dbus_subscription_removed;
}

/**
 * pka_listener_dbus_init:
 * @listener: A #PkaListenerDBus.
 *
 * Initializes the newly created #PkaListenerDBus instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pka_listener_dbus_init (PkaListenerDBus *listener)
{
	listener->priv = G_TYPE_INSTANCE_GET_PRIVATE(listener,
	                                             PKA_TYPE_LISTENER_DBUS,
	                                             PkaListenerDBusPrivate);
	listener->priv->mutex = g_mutex_new();
	listener->priv->peers = g_hash_table_new_full(
			g_str_hash, g_str_equal, g_free,
			(GDestroyNotify)peer_free);
	g_static_rw_lock_init(&listener->priv->handlers_lock);
	listener->priv->handlers = g_hash_table_new_full(
			g_int_hash, g_int_equal, NULL,
			(GDestroyNotify)handler_free);
}

const PkaPluginInfo pka_plugin_info = {
	.id          = "DBus",
	.name        = "DBus Listener",
	.description = "Provides DBus access to Perfkit",
	.plugin_type = PKA_PLUGIN_LISTENER,
	.factory     = (PkaPluginFactory)pka_listener_dbus_new,
};

/* gdkevent-module.c
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

/*
 * Some code in this file is borrowed from libsocialweb.
 *
 * libsocialweb - social data store
 * Copyright (C) 2009 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gio/gio.h>
#include <glib/gprintf.h>

#include <perfkit-agent/pka-log.h>

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "GdkEventModule"

#define TV2DOUBLE(tv) ((tv).tv_sec + ((tv).tv_usec / (gdouble)G_USEC_PER_SEC))

static GTimeVal         last_time;
static GPollFunc        poll_func;
static GDBusConnection *connection;

static void
gdkevent_handle_destroy (GdkEvent   *event,
                         GDBusConnection *channel)
{
#if 0
	gchar buffer[32];

	g_snprintf(buffer, sizeof(buffer), "%d|%p\n",
	           GDK_DESTROY,
	           event->any.window);
	g_io_channel_write_chars(channel, buffer, -1, NULL, NULL);
	g_io_channel_flush(channel, NULL);
#endif
}

static void
gdkevent_handle_expose (GdkEvent        *event,
                        GDBusMessage    *message,
                        GDBusConnection *dbus)
{
	GdkEventExpose *expose = (GdkEventExpose *)event;
	GVariant *body;

	body = g_variant_new("(uuu(iiii))",
	                     event->type,
	                     gtk_get_current_event_time(),
	                     GDK_WINDOW_XID(expose->window),
	                     expose->area.x, expose->area.y,
	                     expose->area.width, expose->area.height);
	g_dbus_message_set_body(message, body);
}

static void
gdkevent_handle_motion_notfiy (GdkEvent   *event,
                               GDBusConnection *channel)
{
#if 0
	gchar buffer[128];

	g_snprintf(buffer, sizeof(buffer), "%d|%p|%0.1f|%0.1f|%u\n",
	           GDK_MOTION_NOTIFY,
	           event->motion.window,
	           event->motion.x,
	           event->motion.y,
	           event->motion.state);
	g_io_channel_write_chars(channel, buffer, -1, NULL, NULL);
	g_io_channel_flush(channel, NULL);
#endif
}

static void
gdkevent_handle_button (GdkEvent   *event,
                        GDBusConnection *channel)
{
#if 0
	gchar buffer[128];

	g_snprintf(buffer, sizeof(buffer), "%d|%p|%u|%0.1f|%0.1f|%u\n",
	           event->any.type,
	           event->button.window,
	           event->button.button,
	           event->button.x,
	           event->button.y,
	           event->button.state);
	g_io_channel_write_chars(channel, buffer, -1, NULL, NULL);
	g_io_channel_flush(channel, NULL);
#endif
}

static void
gdkevent_handle_key (GdkEvent   *event,
                     GDBusConnection *channel)
{
#if 0
	gchar buffer[128];

	g_snprintf(buffer, sizeof(buffer), "%d|%p|%u|%u\n",
	           event->any.type,
	           event->key.window,
	           event->key.keyval,
	           event->key.state);
	g_io_channel_write_chars(channel, buffer, -1, NULL, NULL);
	g_io_channel_flush(channel, NULL);
#endif
}

static void
gdkevent_handle_crossing (GdkEvent   *event,
                          GDBusConnection *channel)
{
#if 0
	gchar buffer[128];

	g_snprintf(buffer, sizeof(buffer), "%d|%p|%u|%u\n",
	           event->any.type,
	           event->crossing.window,
	           event->crossing.mode,
	           event->crossing.state);
	g_io_channel_write_chars(channel, buffer, -1, NULL, NULL);
	g_io_channel_flush(channel, NULL);
#endif
}

static void
gdkevent_handle_focus (GdkEvent   *event,
                       GDBusConnection *channel)
{
#if 0
	gchar buffer[128];

	g_snprintf(buffer, sizeof(buffer), "%d|%p|%u\n",
	           event->any.type,
	           event->focus_change.window,
	           event->focus_change.in);
	g_io_channel_write_chars(channel, buffer, -1, NULL, NULL);
	g_io_channel_flush(channel, NULL);
#endif
}

static void
gdkevent_handle_configure (GdkEvent   *event,
                           GDBusConnection *channel)
{
#if 0
	gchar buffer[128];

	g_snprintf(buffer, sizeof(buffer), "%d|%p|%d|%d|%d|%d\n",
	           event->any.type,
	           event->configure.window,
	           event->configure.x,
	           event->configure.y,
	           event->configure.width,
	           event->configure.height);
	g_io_channel_write_chars(channel, buffer, -1, NULL, NULL);
	g_io_channel_flush(channel, NULL);
#endif
}

static void
gdkevent_handle_any (GdkEvent   *event,
                     GDBusConnection *channel)
{
#if 0
	gchar buffer[32];

	g_snprintf(buffer, sizeof(buffer), "%d|%p\n",
	           event->any.type,
	           event->configure.window);
	g_io_channel_write_chars(channel, buffer, -1, NULL, NULL);
	g_io_channel_flush(channel, NULL);
#endif
}

static void
gdkevent_dispatcher (GdkEvent *event,
                     gpointer  data)
{
	GDBusMessage *message;
	GError *error = NULL;

	message = g_dbus_message_new_method_call(NULL, "/",
	                                         "org.perfkit.Agent.GdkEvent",
	                                         "Event");

	switch (event->type) {
	case GDK_NOTHING:
		break;
	case GDK_DELETE:
		gdkevent_handle_any(event, connection);
		break;
	case GDK_DESTROY:
		gdkevent_handle_destroy(event, connection);
		break;
	case GDK_EXPOSE:
		gdkevent_handle_expose(event, message, connection);
		break;
	case GDK_MOTION_NOTIFY:
		gdkevent_handle_motion_notfiy(event, connection);
		break;
	case GDK_BUTTON_PRESS:
	case GDK_2BUTTON_PRESS:
	case GDK_3BUTTON_PRESS:
	case GDK_BUTTON_RELEASE:
		gdkevent_handle_button(event, connection);
		break;
	case GDK_KEY_PRESS:
	case GDK_KEY_RELEASE:
		gdkevent_handle_key(event, connection);
		break;
	case GDK_ENTER_NOTIFY:
	case GDK_LEAVE_NOTIFY:
		gdkevent_handle_crossing(event, connection);
		break;
	case GDK_FOCUS_CHANGE:
		gdkevent_handle_focus(event, connection);
		break;
	case GDK_CONFIGURE:
		gdkevent_handle_configure(event, connection);
		break;
	case GDK_MAP:
	case GDK_UNMAP:
		gdkevent_handle_any(event, connection);
		break;
	case GDK_PROPERTY_NOTIFY:
	case GDK_SELECTION_CLEAR:
	case GDK_SELECTION_REQUEST:
	case GDK_SELECTION_NOTIFY:
	case GDK_PROXIMITY_IN:
	case GDK_PROXIMITY_OUT:
	case GDK_DRAG_ENTER:
	case GDK_DRAG_LEAVE:
	case GDK_DRAG_MOTION:
	case GDK_DRAG_STATUS:
	case GDK_DROP_START:
	case GDK_DROP_FINISHED:
	case GDK_CLIENT_EVENT:
	case GDK_VISIBILITY_NOTIFY:
#if !GTK_CHECK_VERSION(2, 91, 0)
	case GDK_NO_EXPOSE:
#endif
	case GDK_SCROLL:
	case GDK_WINDOW_STATE:
	case GDK_SETTING:
	case GDK_OWNER_CHANGE:
	case GDK_GRAB_BROKEN:
	case GDK_DAMAGE:
	case GDK_EVENT_LAST:
	default:
		/*
		 * TODO: Handle more of these specificaly.
		 */
		gdkevent_handle_any(event, connection);
		break;
	}

	if (!g_dbus_connection_send_message(connection, message,
	                                    G_DBUS_SEND_MESSAGE_FLAGS_NONE,
	                                    NULL, &error)) {
		CRITICAL(Gdk, "Error sending message: %s", error->message);
		g_error_free(error);
	}

	gtk_main_do_event(event);
}

static void
gdkevent_module_unloaded (gpointer data)
{
	GDBusConnection *dbus = (GDBusConnection *)data;

	ENTRY;
	g_object_unref(dbus);
	EXIT;
}

static inline long
time_val_difference (GTimeVal *a,
                     GTimeVal *b)
{
	return ((a->tv_sec - b->tv_sec) * G_USEC_PER_SEC) +
	       (a->tv_usec - b->tv_usec);
}

static gint
gdkevent_poll (GPollFD *ufds,
               guint    nfds,
               gint     timeout_)
{
	GDBusMessage *message;
	GVariant *body;
	GError *error = NULL;
	GTimeVal now;
	long diff;
	gint ret;

	g_get_current_time(&now);
	diff = time_val_difference(&now, &last_time);

	/* Log if the delay was more than a tenth of a second */
	if (diff > (G_USEC_PER_SEC / 10)) {
		message = g_dbus_message_new_method_call(NULL, "/",
		                                         "org.perfkit.Agent.GdkEvent",
		                                         "Delay");
		body = g_variant_new("(dd)",
		                     TV2DOUBLE(last_time),
		                     TV2DOUBLE(now));
		g_dbus_message_set_body(message, body);
		if (!g_dbus_connection_send_message(connection, message,
		                                    G_DBUS_SEND_MESSAGE_FLAGS_NONE,
		                                    NULL, &error)) {
			CRITICAL(Gdk, "Error sending message: %s", error->message);
			g_error_free(error);
		}
	}

	ret = poll_func(ufds, nfds, timeout_);

	g_get_current_time(&now);
	last_time = now;

	return ret;
}

gint
gtk_module_init (gint   argc,
                 gchar *argv[])
{
	GDBusConnection *dbus;
	gchar *address;
	const gchar *socket;
	GError *error = NULL;

	ENTRY;

	if (!(socket = g_getenv("GDKEVENT_SOCKET"))) {
		CRITICAL(Gdk, "Failed to load gdkevent socket.");
		RETURN(-1);
	}

	address = g_strdup_printf("unix:path=%s", socket);
	dbus = g_dbus_connection_new_for_address_sync(address,
	                                              G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT,
	                                              NULL, NULL, &error);
	g_free(address);

	if (!dbus) {
		CRITICAL(Gdk, "Failed to load IPC socket: %s", error->message);
		g_error_free(error);
		RETURN(-1);
	}

	connection = dbus;

	gdk_event_handler_set(gdkevent_dispatcher, dbus,
	                      gdkevent_module_unloaded);

	g_get_current_time(&last_time);
	poll_func = g_main_context_get_poll_func(g_main_context_default());
	g_main_context_set_poll_func(g_main_context_default(), gdkevent_poll);

	RETURN(0);
}

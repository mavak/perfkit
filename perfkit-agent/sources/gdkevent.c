/* gdkevent.c
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

#include <gdk/gdk.h>
#include <glib/gstdio.h>
#include <perfkit-agent/perfkit-agent.h>

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "GdkEvent"

#define GDKEVENT_TYPE_SOURCE            (gdkevent_get_type())
#define GDKEVENT_SOURCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GDKEVENT_TYPE_SOURCE, Gdkevent))
#define GDKEVENT_SOURCE_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), GDKEVENT_TYPE_SOURCE, Gdkevent const))
#define GDKEVENT_SOURCE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GDKEVENT_TYPE_SOURCE, GdkeventClass))
#define GDKEVENT_IS_SOURCE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDKEVENT_TYPE_SOURCE))
#define GDKEVENT_IS_SOURCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GDKEVENT_TYPE_SOURCE))
#define GDKEVENT_SOURCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GDKEVENT_TYPE_SOURCE, GdkeventClass))

typedef struct _Gdkevent        Gdkevent;
typedef struct _GdkeventClass   GdkeventClass;
typedef struct _GdkeventPrivate GdkeventPrivate;

struct _Gdkevent
{
	PkaSource parent;

	/*< private >*/
	GdkeventPrivate *priv;
};

struct _GdkeventClass
{
	PkaSourceClass parent_class;
};

G_DEFINE_TYPE(Gdkevent, gdkevent, PKA_TYPE_SOURCE)

struct _GdkeventPrivate
{
	gchar       *socket;
	GDBusServer *server;
	GList       *conns;
};

enum
{
	FIELD_0,
	FIELD_FEATURE,
	FIELD_TYPE,
	FIELD_TIME,
	FIELD_WINDOW,
	FIELD_X,
	FIELD_Y,
	FIELD_WIDTH,
	FIELD_HEIGHT,
	FIELD_XROOT,
	FIELD_YROOT,
	FIELD_DELAY_BEGIN,
	FIELD_DELAY_END,
};

enum
{
	EVENT,
	DELAY,
};

static GQuark event_quark (void) G_GNUC_CONST;
static GQuark delay_quark (void) G_GNUC_CONST;

static void
gdkevent_started (PkaSource    *source,
                  PkaSpawnInfo *spawn_info)
{
	PkaManifest *manifest;

	ENTRY;

	manifest = pka_manifest_new();
	pka_manifest_append(manifest, "Feature",    G_TYPE_UINT);
	pka_manifest_append(manifest, "Type",       G_TYPE_INT);
	pka_manifest_append(manifest, "Time",       G_TYPE_UINT);
	pka_manifest_append(manifest, "Window",     G_TYPE_UINT);
	pka_manifest_append(manifest, "X",          G_TYPE_INT);
	pka_manifest_append(manifest, "Y",          G_TYPE_INT);
	pka_manifest_append(manifest, "Width",      G_TYPE_INT);
	pka_manifest_append(manifest, "Height",     G_TYPE_INT);
	pka_manifest_append(manifest, "XRoot",      G_TYPE_DOUBLE);
	pka_manifest_append(manifest, "YRoot",      G_TYPE_DOUBLE);
	pka_manifest_append(manifest, "Begin",      G_TYPE_DOUBLE);
	pka_manifest_append(manifest, "End",        G_TYPE_DOUBLE);
	pka_source_deliver_manifest(source, manifest);

	EXIT;
}

static void
gdkevent_stopped (PkaSource *source)
{
	ENTRY;
	EXIT;
}

static gboolean
gdkevent_modify_spawn_info (PkaSource     *source,
                            PkaSpawnInfo  *spawn_info,
                            GError       **error)
{
	Gdkevent *gdkevent = (Gdkevent *)source;
	GdkeventPrivate *priv;

	ENTRY;

	g_return_val_if_fail(GDKEVENT_IS_SOURCE(gdkevent), FALSE);

	priv = gdkevent->priv;

	pka_spawn_info_set_env(spawn_info, "GDKEVENT_SOCKET", priv->socket);
	pka_spawn_info_set_env(spawn_info, "GTK_MODULES",
	                       "gdkevent2-module,gdkevent3-module");

	RETURN(TRUE);
}

static void
gdkevent_populate_expose (Gdkevent  *source,
                          GVariant  *body,
                          PkaSample *sample)
{
	guint32 window;
	GdkRectangle area;

	ENTRY;

	g_variant_get_child(body, 2, "u", &window);
	g_variant_get_child(body, 3, "(iiii)",
	                    &area.x, &area.y, &area.width, &area.height);

	pka_sample_append_uint(sample, FIELD_WINDOW, window);
	pka_sample_append_int(sample, FIELD_X, area.x);
	pka_sample_append_int(sample, FIELD_Y, area.y);
	pka_sample_append_int(sample, FIELD_WIDTH, area.width);
	pka_sample_append_int(sample, FIELD_HEIGHT, area.height);

	EXIT;
}

static GQuark
event_quark (void)
{
	return g_quark_from_static_string("Event");
}

static GQuark
delay_quark (void)
{
	return g_quark_from_static_string("Delay");
}

static GDBusMessage *
gdkevent_filter_func (GDBusConnection *connection,
                      GDBusMessage    *message,
                      gboolean         incoming,
                      gpointer         user_data)
{
	Gdkevent *source = (Gdkevent *)user_data;
	PkaSample *sample;
	GVariant *body;
	guint32 msg_type = 0;
	guint32 msg_time = 0;
	gdouble begin;
	gdouble end;
	GQuark quark;

	ENTRY;

	if (!(body = g_dbus_message_get_body(message))) {
		RETURN(NULL);
	}

	quark = g_quark_from_string(g_dbus_message_get_member(message));

	if (quark == event_quark()) {
		g_variant_get_child(body, 0, "u", &msg_type);
		g_variant_get_child(body, 1, "u", &msg_time);

		sample = pka_sample_new();
		pka_sample_append_int(sample, FIELD_FEATURE, EVENT);
		pka_sample_append_int(sample, FIELD_TYPE, msg_type);
		pka_sample_append_uint(sample, FIELD_TIME, msg_time);

		switch (msg_type) {
		case GDK_EXPOSE:
			gdkevent_populate_expose(source, body, sample);
			break;
		default:
			g_assert_not_reached();
			RETURN(NULL);
		}

		pka_source_deliver_sample(PKA_SOURCE(source), sample);
	} else if (quark == delay_quark()) {
		g_variant_get_child(body, 0, "d", &begin);
		g_variant_get_child(body, 1, "d", &end);

		sample = pka_sample_new();
		pka_sample_append_int(sample, FIELD_FEATURE, DELAY);
		pka_sample_append_double(sample, FIELD_DELAY_BEGIN, begin);
		pka_sample_append_double(sample, FIELD_DELAY_END, end);

		pka_source_deliver_sample(PKA_SOURCE(source), sample);
	} else {
		g_assert_not_reached();
		RETURN(NULL);
	}

	RETURN(NULL);
}

static gboolean
gdkevent_new_connection (GDBusServer     *server,
                         GDBusConnection *connection,
                         Gdkevent        *gdkevent)
{
	GdkeventPrivate *priv;

	ENTRY;

	g_return_val_if_fail(GDKEVENT_IS_SOURCE(gdkevent), FALSE);

	priv = gdkevent->priv;
	priv->conns = g_list_prepend(priv->conns, g_object_ref(connection));
	g_dbus_connection_add_filter(connection, gdkevent_filter_func,
	                             gdkevent, NULL);
	RETURN(TRUE);
}

static void
gdkevent_finalize (GObject *object)
{
	GdkeventPrivate *priv = GDKEVENT_SOURCE(object)->priv;

	ENTRY;

	g_list_foreach(priv->conns, (GFunc)g_object_unref, NULL);
	g_list_free(priv->conns);
	g_unlink(priv->socket);
	g_object_unref(priv->server);
	g_free(priv->socket);

	G_OBJECT_CLASS(gdkevent_parent_class)->finalize(object);

	EXIT;
}

static void
gdkevent_class_init (GdkeventClass *klass)
{
	GObjectClass *object_class;
	PkaSourceClass *source_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = gdkevent_finalize;
	g_type_class_add_private(object_class, sizeof(GdkeventPrivate));

	source_class = PKA_SOURCE_CLASS(klass);
	source_class->modify_spawn_info = gdkevent_modify_spawn_info;
	source_class->started = gdkevent_started;
	source_class->stopped = gdkevent_stopped;
}

static void
gdkevent_init (Gdkevent *gdkevent)
{
	static gint instance = 0;
	GdkeventPrivate *priv;
	gchar *guid;
	gchar *address;
	GError *error = NULL;

	ENTRY;

	priv = G_TYPE_INSTANCE_GET_PRIVATE(gdkevent, GDKEVENT_TYPE_SOURCE,
	                                   GdkeventPrivate);
	gdkevent->priv = priv;

	priv->socket = g_strdup_printf("%s" G_DIR_SEPARATOR_S "gdkevent-%d-%d.socket;",
	                               pka_get_user_runtime_dir(),
	                               (gint)getpid(),
	                               instance++);
	address = g_strdup_printf("unix:path=%s;", priv->socket);
	guid = g_dbus_generate_guid();
	priv->server = g_dbus_server_new_sync(address,
	                                      G_DBUS_SERVER_FLAGS_AUTHENTICATION_ALLOW_ANONYMOUS,
	                                      guid,
	                                      NULL, NULL, &error);
	g_free(guid);
	g_free(address);
	if (!priv->server) {
		CRITICAL(GdkEvent, "Failed to create IPC socket: %s", error->message);
		g_error_free(error);
		return;
	}
	g_signal_connect(priv->server, "new-connection",
	                 G_CALLBACK(gdkevent_new_connection), gdkevent);
	g_dbus_server_start(priv->server);

	EXIT;
}

GObject *
gdkevent_new (GError **error)
{
	return g_object_new(GDKEVENT_TYPE_SOURCE, NULL);
}

const PkaPluginInfo pka_plugin_info = {
	.id          = "GdkEvent",
	.name        = "Gtk+ events",
	.description = "This source provides information about the events "
	               "processed by the Gtk+ main loop.",
	.version     = "0.1.1",
	.copyright   = "Christian Hergert",
	.factory     = gdkevent_new,
	.plugin_type = PKA_PLUGIN_SOURCE,
};

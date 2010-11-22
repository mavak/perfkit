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

#include <glib/gstdio.h>
#include <perfkit-agent/perfkit-agent.h>

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
};

static void
gdkevent_started (PkaSource    *source,
                  PkaSpawnInfo *spawn_info)
{
}

static void
gdkevent_stopped (PkaSource *source)
{
}

static gboolean
gdkevent_modify_spawn_info (PkaSource     *source,
                            PkaSpawnInfo  *spawn_info,
                            GError       **error)
{
	return TRUE;
}

static void
gdkevent_finalize (GObject *object)
{
	GdkeventPrivate *priv = GDKEVENT_SOURCE(object)->priv;

	g_unlink(priv->socket);
	g_free(priv->socket);
	g_object_unref(priv->server);

	G_OBJECT_CLASS(gdkevent_parent_class)->finalize(object);
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

	priv = G_TYPE_INSTANCE_GET_PRIVATE(gdkevent, GDKEVENT_TYPE_SOURCE,
	                                   GdkeventPrivate);
	gdkevent->priv = priv;

	/*
	 * Create private IPC socket to communicate with inferior.
	 */
	priv->socket = g_strdup_printf(
			"%s" G_DIR_SEPARATOR_S "gdkevent-%d-%d.socket",
			pka_get_user_runtime_dir(), (gint)getpid(), instance++);
	address = g_strdup_printf("unix:path=%s", priv->socket);
	guid = g_dbus_generate_guid();
	priv->server = g_dbus_server_new_sync(address,
	                                      G_DBUS_SERVER_FLAGS_NONE,
	                                      guid,
	                                      NULL, NULL, &error);
	g_free(guid);
	g_free(address);
	if (!priv->server) {
		CRITICAL(GdkEvent, "Failed to create IPC socket: %s", error->message);
		g_error_free(error);
		return;
	}
	g_dbus_server_start(priv->server);
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

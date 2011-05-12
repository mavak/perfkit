/* ppg-gdk-instrument.c
 *
 * Copyright (C) 2011 Christian Hergert <chris@dronelabs.com>
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

#include <glib/gi18n.h>
#include <perfkit/perfkit.h>

#include "ppg-color.h"
#include "ppg-gdk-instrument.h"
#include "ppg-log.h"
#include "ppg-renderer-event.h"
#include "ppg-util.h"
#include "ppg-visualizer-simple.h"

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "GdkInst"


struct _PpgGdkInstrumentPrivate
{
	GPtrArray *renderers;
	PkModel   *model;
	gint       source;
	gint       subscription;
};


G_DEFINE_TYPE(PpgGdkInstrument, ppg_gdk_instrument, PPG_TYPE_INSTRUMENT)


static GQuark gTimeQuark;


static PpgVisualizer *
ppg_gdk_instrument_events_cb (PpgVisualizerEntry  *entry,
                              PpgGdkInstrument    *instrument,
                              GError             **error)
{
	PpgGdkInstrumentPrivate *priv;
	PpgVisualizer *visualizer = NULL;
	PpgRenderer *renderer;

	ENTRY;

	g_return_val_if_fail(PPG_IS_GDK_INSTRUMENT(instrument), NULL);

	priv = instrument->priv;

	renderer = g_object_new(PPG_TYPE_RENDERER_EVENT, NULL);
	ppg_renderer_event_append(PPG_RENDERER_EVENT(renderer),
	                          priv->model, gTimeQuark);
	g_ptr_array_add(priv->renderers, renderer);

	visualizer = g_object_new(PPG_TYPE_VISUALIZER_SIMPLE,
	                          "name", "events",
	                          "renderer", renderer,
	                          "title", _("Gdk Events"),
	                          NULL);

	RETURN(visualizer);
}


/**
 * ppg_gdk_instrument_manifest_cb:
 * @manifest: (in): An incoming #PkManifest.
 * @user_data: (in): User data for the closure.
 *
 * Handles an incoming manifest from the Perfkit agent. The contents
 * are stored to the instruments data model.
 *
 * Returns: None.
 * Side effects: Data is stored.
 */
static void
ppg_gdk_instrument_manifest_cb (PkManifest *manifest,
                                gpointer    user_data)
{
	PpgGdkInstrumentPrivate *priv;
	PpgGdkInstrument *instrument = (PpgGdkInstrument *)user_data;

	g_return_if_fail(PPG_IS_GDK_INSTRUMENT(instrument));

	priv = instrument->priv;

	pk_model_insert_manifest(priv->model, manifest);
}


/**
 * ppg_gdk_instrument_sample_cb:
 * @manifest: (in): The current #PkManifest.
 * @sample: (in): An incoming #PkSample.
 *
 * Handles an incoming sample from the Perfkit agent. The contents
 * are stored to the instruments data model.
 *
 * Returns: None.
 * Side effects: Data is stored.
 */
static void
ppg_gdk_instrument_sample_cb (PkManifest *manifest,
                              PkSample   *sample,
                              gpointer    user_data)
{
	PpgGdkInstrumentPrivate *priv;
	PpgGdkInstrument *instrument = (PpgGdkInstrument *)user_data;

	g_return_if_fail(PPG_IS_GDK_INSTRUMENT(instrument));

	priv = instrument->priv;

#ifdef PERFKIT_DEBUG
	g_assert_cmpint(pk_sample_get_source_id(sample), ==, priv->source);
	g_assert_cmpint(pk_manifest_get_source_id(manifest), ==, priv->source);
#endif

	pk_model_insert_sample(priv->model, manifest, sample);
}


/**
 * ppg_gdk_instrument_set_handlers_cb:
 * @object: (in): A #PkConnection.
 * @result: (in): A #GAsyncResult.
 * @user_data: (in): User data for closure.
 *
 * Handles an asynchronous request to set the delivery closures for incoming
 * manifests and samples.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_gdk_instrument_set_handlers_cb (GObject      *object,
                                    GAsyncResult *result,
                                    gpointer      user_data)
{
	PpgGdkInstrument *instrument = (PpgGdkInstrument *)user_data;
	PpgGdkInstrumentPrivate *priv;
	PkConnection *conn = (PkConnection *)object;
	GError *error = NULL;

	g_return_if_fail(PPG_IS_GDK_INSTRUMENT(instrument));
	g_return_if_fail(PK_IS_CONNECTION(object));

	priv = instrument->priv;

	if (!pk_connection_subscription_set_handlers_finish(conn, result, &error)) {
		g_critical("Failed to subscribe to subscription: %d", priv->subscription);
	}
}

static gboolean
ppg_gdk_instrument_load (PpgInstrument  *instrument,
                         PpgSession     *session,
                         GError        **error)
{
	PpgGdkInstrumentPrivate *priv;
	PpgGdkInstrument *gdk = (PpgGdkInstrument *)instrument;
	PkConnection *conn;
	gboolean ret = FALSE;
	gint channel;

	g_return_if_fail(PPG_IS_GDK_INSTRUMENT(gdk));

	priv = gdk->priv;

	priv->model = ppg_session_create_model(session);
	g_assert(priv->model);

	g_object_get(session,
	             "channel", &channel,
	             "connection", &conn,
	             NULL);

	RPC_OR_FAILURE(manager_add_source,
	               (conn, "GdkEvent", &priv->source, error));

	RPC_OR_FAILURE(channel_add_source,
	               (conn, channel, priv->source, error));

	RPC_OR_FAILURE(manager_add_subscription,
	               (conn, 0, 0, &priv->subscription, error));

	RPC_OR_FAILURE(subscription_add_source,
	               (conn, priv->subscription, priv->source, error));

	pk_connection_subscription_set_handlers_async(
			conn, priv->subscription,
			ppg_gdk_instrument_manifest_cb, gdk, NULL,
			ppg_gdk_instrument_sample_cb, gdk, NULL,
			NULL,
			ppg_gdk_instrument_set_handlers_cb, gdk);

	ppg_instrument_add_visualizer(instrument, "events");

failure:
	g_object_unref(conn);

	return ret;
}

/**
 * ppg_gdk_instrument_unload:
 * @instrument: (in): A #PpgGdkInstrument.
 * @session: (in): A #PpgSession.
 * @error: (out): A location for a #GError or %NULL.
 *
 * Unloads the instrument and destroys remote resources.
 *
 * Returns: TRUE if successful; otherwise FALSE.
 * Side effects: Agent resources are destroyed.
 */
static gboolean
ppg_gdk_instrument_unload (PpgInstrument  *instrument,
                           PpgSession     *session,
                           GError        **error)
{
	PpgGdkInstrumentPrivate *priv;
	PkConnection *conn;
	gboolean ret = FALSE;
	gboolean removed;

	g_return_val_if_fail(PPG_IS_GDK_INSTRUMENT(instrument), FALSE);
	g_return_val_if_fail(PPG_IS_SESSION(session), FALSE);

	priv = PPG_GDK_INSTRUMENT(instrument)->priv;

	g_object_get(session,
	             "connection", &conn,
	             NULL);

	RPC_OR_FAILURE(manager_remove_subscription,
	               (conn, priv->subscription, &removed, error));
	RPC_OR_FAILURE(manager_remove_source,
	               (conn, priv->source, error));

  failure:
	priv->subscription = 0;
	priv->source = 0;
	g_object_unref(conn);

	return ret;
}

/**
 * ppg_gdk_instrument_finalize:
 * @object: (in): A #PpgGdkInstrument.
 *
 * Finalizer for a #PpgGdkInstrument instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_gdk_instrument_finalize (GObject *object)
{
	PpgGdkInstrumentPrivate *priv = PPG_GDK_INSTRUMENT(object)->priv;

	g_clear_object(&priv->model);
	g_ptr_array_foreach(priv->renderers, (GFunc)g_object_unref, NULL);
	g_ptr_array_free(priv->renderers, TRUE);

	G_OBJECT_CLASS(ppg_gdk_instrument_parent_class)->finalize(object);
}

/**
 * ppg_gdk_instrument_class_init:
 * @klass: (in): A #PpgGdkInstrumentClass.
 *
 * Initializes the #PpgGdkInstrumentClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_gdk_instrument_class_init (PpgGdkInstrumentClass *klass)
{
	GObjectClass *object_class;
	PpgInstrumentClass *instrument_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_gdk_instrument_finalize;
	g_type_class_add_private(object_class, sizeof(PpgGdkInstrumentPrivate));

	instrument_class = PPG_INSTRUMENT_CLASS(klass);
	instrument_class->load = ppg_gdk_instrument_load;
	instrument_class->unload = ppg_gdk_instrument_unload;

	gTimeQuark = g_quark_from_static_string("Time");
}

/**
 * ppg_gdk_instrument_init:
 * @: (in): A #PpgGdkInstrument.
 *
 * Initializes the newly created #PpgGdkInstrument instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_gdk_instrument_init (PpgGdkInstrument *instrument)
{
	static PpgVisualizerEntry visualizer_entries[] = {
		{ "events", N_("Gtk+ Events"), NULL,
		  G_CALLBACK(ppg_gdk_instrument_events_cb) },
	};

	instrument->priv =
		G_TYPE_INSTANCE_GET_PRIVATE(instrument,
		                            PPG_TYPE_GDK_INSTRUMENT,
		                            PpgGdkInstrumentPrivate);

	g_object_set(instrument, "title", _("Gdk"), NULL);

	instrument->priv->renderers = g_ptr_array_new();

	ppg_instrument_register_visualizers(PPG_INSTRUMENT(instrument),
	                                    G_N_ELEMENTS(visualizer_entries),
	                                    visualizer_entries,
	                                    instrument);
}

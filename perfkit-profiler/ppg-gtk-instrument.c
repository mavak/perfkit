/* ppg-gtk-instrument.c
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

#include <glib/gi18n.h>

#include "ppg-color.h"
#include "ppg-gtk-instrument.h"
#include "ppg-line-visualizer.h"
#include "ppg-log.h"
#include "ppg-model.h"

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "GtkInstrument"

#define RPC_OR_GOTO(_rpc, _args, _label)  \
    G_STMT_START {                        \
        ret = pk_connection_##_rpc _args; \
        if (!ret) {                       \
            goto _label;                  \
        }                                 \
    } G_STMT_END

#define RPC_OR_FAILURE(_rpc, _args) RPC_OR_GOTO(_rpc, _args, failure)

G_DEFINE_TYPE(PpgGtkInstrument, ppg_gtk_instrument, PPG_TYPE_INSTRUMENT)

struct _PpgGtkInstrumentPrivate
{
	PpgSession *session;
	PpgModel   *model;
	gint        channel;
	gint        source;
	gint        sub;
};

enum
{
	COLUMN_TYPE,
	COLUMN_TIME,
	COLUMN_LAST
};

static PpgVisualizer*
ppg_gtk_instrument_events_cb (PpgGtkInstrument *instrument)
{
	PpgGtkInstrumentPrivate *priv;
	PpgLineVisualizer *visualizer;
	PpgColorIter color;

	g_return_val_if_fail(PPG_IS_GTK_INSTRUMENT(instrument), NULL);

	priv = instrument->priv;

	visualizer = g_object_new(PPG_TYPE_LINE_VISUALIZER,
	                          "name", "events",
	                          "title", _("Gtk+ Events"),
	                          NULL);

	/*
	 * FIXME: Create PpgColumnVisualizer or something.
	 */
	ppg_color_iter_init(&color);
	ppg_line_visualizer_append(visualizer, "Gtk+ Events", &color.color,
	                           TRUE, NULL, COLUMN_TYPE);

	return PPG_VISUALIZER(visualizer);
}

static PpgVisualizerEntry visualizer_entries[] = {
	{ "events",
	  N_("Gtk+ Events"),
	  NULL,
	  G_CALLBACK(ppg_gtk_instrument_events_cb) },
};

/**
 * ppg_gtk_instrument_sample_cb:
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
ppg_gtk_instrument_sample_cb (PkManifest *manifest,
                              PkSample   *sample,
                              gpointer    user_data)
{
	PpgGtkInstrument *instrument = (PpgGtkInstrument *)user_data;
	PpgGtkInstrumentPrivate *priv;

	g_return_if_fail(PPG_IS_GTK_INSTRUMENT(instrument));

	priv = instrument->priv;

#ifdef PERFKIT_DEBUG
	g_assert_cmpint(pk_sample_get_source_id(sample), ==, priv->source);
	g_assert_cmpint(pk_manifest_get_source_id(manifest), ==, priv->source);
#endif

	ppg_model_insert_sample(priv->model, manifest, sample);
}

/**
 * ppg_gtk_instrument_manifest_cb:
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
ppg_gtk_instrument_manifest_cb (PkManifest *manifest,
                                gpointer    user_data)
{
	PpgGtkInstrument *instrument = (PpgGtkInstrument *)user_data;
	PpgGtkInstrumentPrivate *priv;

	g_return_if_fail(PPG_IS_GTK_INSTRUMENT(instrument));

	priv = instrument->priv;
	ppg_model_insert_manifest(priv->model, manifest);
}

/**
 * ppg_gtk_instrument_set_handlers_cb:
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
ppg_gtk_instrument_set_handlers_cb (GObject      *object,
                                    GAsyncResult *result,
                                    gpointer      user_data)
{
	PpgGtkInstrument *instrument = (PpgGtkInstrument *)user_data;
	PpgGtkInstrumentPrivate *priv;
	PkConnection *conn = (PkConnection *)object;
	GError *error = NULL;

	g_return_if_fail(PPG_IS_GTK_INSTRUMENT(instrument));
	g_return_if_fail(PK_IS_CONNECTION(object));

	priv = instrument->priv;

	if (!pk_connection_subscription_set_handlers_finish(conn, result, &error)) {
		g_critical("Failed to subscribe to subscription: %d", priv->sub);
	}
}

static gboolean
ppg_gtk_instrument_load (PpgInstrument  *instrument,
                         PpgSession     *session,
                         GError        **error)
{
	PpgGtkInstrumentPrivate *priv;
	PkConnection *conn;
	gboolean ret = FALSE;
	gint channel;

	ENTRY;

	g_return_val_if_fail(PPG_IS_GTK_INSTRUMENT(instrument), FALSE);

	priv = PPG_GTK_INSTRUMENT(instrument)->priv;

	g_object_get(session,
	             "channel", &channel,
	             "connection", &conn,
	             NULL);

	priv->model = g_object_new(PPG_TYPE_MODEL,
	                           "session", session,
	                           NULL);
	ppg_model_add_mapping(priv->model, COLUMN_TYPE, "Type", G_TYPE_UINT, PPG_MODEL_RAW);
	ppg_model_add_mapping(priv->model, COLUMN_TIME, "Time", G_TYPE_UINT, PPG_MODEL_RAW);
	ppg_model_set_track_range(priv->model, COLUMN_TYPE, TRUE);
	ppg_model_set_track_range(priv->model, COLUMN_TIME, TRUE);

	RPC_OR_FAILURE(manager_add_source,
	               (conn, "GdkEvent", &priv->source, error));

	RPC_OR_FAILURE(channel_add_source,
	               (conn, channel, priv->source, error));

	RPC_OR_FAILURE(manager_add_subscription,
	               (conn, 0, 0, &priv->sub, error));

	RPC_OR_FAILURE(subscription_add_source,
	               (conn, priv->sub, priv->source, error));

	pk_connection_subscription_set_handlers_async(
			conn, priv->sub,
			ppg_gtk_instrument_manifest_cb, instrument, NULL,
			ppg_gtk_instrument_sample_cb, instrument, NULL,
			NULL,
			ppg_gtk_instrument_set_handlers_cb, instrument);

	ppg_instrument_add_visualizer(instrument, "events");

  failure:
	g_object_unref(conn);
	RETURN(TRUE);
}

static gboolean
ppg_gtk_instrument_unload (PpgInstrument  *instrument,
                           PpgSession     *session,
                           GError        **error)
{
	ENTRY;
	RETURN(TRUE);
}

/**
 * ppg_gtk_instrument_finalize:
 * @object: (in): A #PpgGtkInstrument.
 *
 * Finalizer for a #PpgGtkInstrument instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_gtk_instrument_finalize (GObject *object)
{
	PpgGtkInstrumentPrivate *priv = PPG_GTK_INSTRUMENT(object)->priv;

	if (priv->model) {
		g_object_unref(priv->model);
	}

	G_OBJECT_CLASS(ppg_gtk_instrument_parent_class)->finalize(object);
}

/**
 * ppg_gtk_instrument_class_init:
 * @klass: (in): A #PpgGtkInstrumentClass.
 *
 * Initializes the #PpgGtkInstrumentClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_gtk_instrument_class_init (PpgGtkInstrumentClass *klass)
{
	GObjectClass *object_class;
	PpgInstrumentClass *instrument_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_gtk_instrument_finalize;
	g_type_class_add_private(object_class, sizeof(PpgGtkInstrumentPrivate));

	instrument_class = PPG_INSTRUMENT_CLASS(klass);
	instrument_class->load = ppg_gtk_instrument_load;
	instrument_class->unload = ppg_gtk_instrument_unload;
}

/**
 * ppg_gtk_instrument_init:
 * @instrument: (in): A #PpgGtkInstrument.
 *
 * Initializes the newly created #PpgGtkInstrument instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_gtk_instrument_init (PpgGtkInstrument *instrument)
{
	PpgGtkInstrumentPrivate *priv;

	priv = G_TYPE_INSTANCE_GET_PRIVATE(instrument, PPG_TYPE_GTK_INSTRUMENT,
	                                   PpgGtkInstrumentPrivate);
	instrument->priv = priv;

	ppg_instrument_register_visualizers(PPG_INSTRUMENT(instrument),
	                                    visualizer_entries,
	                                    G_N_ELEMENTS(visualizer_entries),
	                                    instrument);
}

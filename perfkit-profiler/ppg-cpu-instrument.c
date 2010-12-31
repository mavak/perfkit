/* ppg-cpu-instrument.c
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
#include <perfkit/perfkit.h>

#include "ppg-color.h"
#include "ppg-cpu-instrument.h"
#include "ppg-log.h"
#include "ppg-renderer-line.h"
#include "ppg-util.h"
#include "ppg-visualizer-simple.h"

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "Cpu"


struct _PpgCpuInstrumentPrivate
{
	struct {
		GPtrArray  *renderers; /* All renderers in use */
		GHashTable *models;    /* Models (one per cpu) */
		gint        cpu_row;   /* CPU row id */
	} combined;
	gint  source;              /* Perfkit cpu source id. */
	gint  subscription;        /* Perfkit subscription id. */
};


G_DEFINE_TYPE(PpgCpuInstrument, ppg_cpu_instrument, PPG_TYPE_INSTRUMENT)


static GQuark cpu_quark;
static GQuark idle_quark;
static GQuark nice_quark;
static GQuark percent_quark;
static GQuark system_quark;
static GQuark user_quark;


/**
 * ppg_cpu_instrument_calc_cpu:
 * @model: (in): A #PkModel.
 * @iter: (in): A #PkModelIter.
 * @key: (in): The requested key to be built.
 * @value: (out): A location to store the result.
 * @user_data: (closure): User data provided to the builder.
 *
 * Generates the percentage of CPU consumed for a given CPU. All CPU time
 * is tallied except for time in the idle process. The values retrieved
 * from the model should be of "COUNTER" type, meaning they already have
 * the previous value subtracted from them.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_cpu_instrument_calc_cpu (PkModel     *model,
                             PkModelIter *iter,
                             GQuark       key,
                             GValue      *value,
                             gpointer     user_data)
{
	gdouble user;
	gdouble nice_;
	gdouble system;
	gdouble idle;
	gdouble percent;

	user = pk_model_get_double(model, iter, user_quark);
	nice_ = pk_model_get_double(model, iter, nice_quark);
	system = pk_model_get_double(model, iter, system_quark);
	idle = pk_model_get_double(model, iter, idle_quark);

	percent = (user + nice_ + system) /
	          (user + nice_ + system + idle) *
	          100.0;
	g_value_set_double(value, percent);
}


/**
 * ppg_cpu_instrument_renderer_disposed:
 * @user_data: (in): A #PpgCpuInstrument.
 * @renderer: (in): The location of the renderer.
 *
 * Handles notification of a renderer being disposed. The renderer is removed
 * from the instruments list of renderers.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_cpu_instrument_renderer_disposed (gpointer  user_data,
                                      GObject  *renderer)
{
	PpgCpuInstrumentPrivate *priv;
	g_return_if_fail(PPG_IS_CPU_INSTRUMENT(user_data));
	priv = PPG_CPU_INSTRUMENT(user_data)->priv;
	g_ptr_array_remove(priv->combined.renderers, renderer);
}


/**
 * ppg_cpu_instrument_combined_cb:
 * @instrument: (in): A #PpgCpuInstrument.
 *
 * Handles a callback to create the "combined" visualizer.
 *
 * Returns: A newly created #PpgVisualizer.
 * Side effects: None.
 */
static PpgVisualizer*
ppg_cpu_instrument_combined_cb (PpgVisualizerEntry  *entry,
                                PpgCpuInstrument    *instrument,
                                GError             **error)
{
	PpgCpuInstrumentPrivate *priv;
	PpgRendererLine *renderer;
	GHashTableIter iter;
	PpgVisualizer *visualizer;
	PpgColorIter color;
	gpointer key;
	gpointer value;

	ENTRY;

	g_return_val_if_fail(PPG_IS_CPU_INSTRUMENT(instrument), NULL);

	priv = instrument->priv;

	/*
	 * Create renderer to render the cpu lines.
	 */
	renderer = g_object_new(PPG_TYPE_RENDERER_LINE, NULL);

	/*
	 * Store a reference so we can add lines as CPUs are discovered.
	 */
	g_ptr_array_add(priv->combined.renderers, renderer);
	g_object_weak_ref(G_OBJECT(renderer),
	                  ppg_cpu_instrument_renderer_disposed,
	                  instrument);

	/*
	 * Create a basic visualizer and add our renderer to it.
	 */
	visualizer = g_object_new(PPG_TYPE_VISUALIZER_SIMPLE,
	                          "name", "combined",
	                          "renderer", renderer,
	                          "title", _("Combined CPU Usage"),
	                          NULL);

	/*
	 * Add a line for each of the models.
	 */
	g_hash_table_iter_init(&iter, priv->combined.models);
	ppg_color_iter_init(&color);
	while (g_hash_table_iter_next(&iter, &key, &value)) {
		ppg_renderer_line_append(renderer, value, percent_quark);
		ppg_color_iter_next(&color);
	}

	RETURN(PPG_VISUALIZER(visualizer));
}


static PpgVisualizerEntry visualizer_entries[] = {
	{ "combined",
	  N_("Combined CPU Usage"),
	  NULL,
	  G_CALLBACK(ppg_cpu_instrument_combined_cb) },
};


static PkModel *
get_model (PpgCpuInstrument *instrument,
           PkManifest       *manifest,
           gint              cpu)
{
	PpgCpuInstrumentPrivate *priv;
	PpgRendererLine *renderer;
	PpgColorIter color;
	PpgSession *session;
	PkModel *model;
	gint *key;
	gint i;

	g_return_val_if_fail(PPG_IS_CPU_INSTRUMENT(instrument), NULL);

	priv = instrument->priv;

	if (!(model = g_hash_table_lookup(priv->combined.models, &cpu))) {
		session = ppg_instrument_get_session(PPG_INSTRUMENT(instrument));
		model = g_object_new(PK_TYPE_MODEL_MEMORY, NULL);

		/*
		 * Prepare the model for the various values.
		 */
		pk_model_set_field_mode(model, idle_quark, PK_MODEL_COUNTER);
		pk_model_set_field_mode(model, nice_quark, PK_MODEL_COUNTER);
		pk_model_set_field_mode(model, system_quark, PK_MODEL_COUNTER);
		pk_model_set_field_mode(model, user_quark, PK_MODEL_COUNTER);
		pk_model_register_builder(model, percent_quark,
		                          ppg_cpu_instrument_calc_cpu,
		                          instrument, NULL);

		/*
		 * Add the current manifest.
		 */
		pk_model_insert_manifest(model, manifest);

		key = g_new(gint, 1);
		*key = cpu;
		g_hash_table_insert(priv->combined.models, key, model);

		/*
		 * Add new cpu line to renderers.
		 */
		ppg_color_iter_init(&color);
		for (i = 0; i < priv->combined.renderers->len; i++) {
			renderer = g_ptr_array_index(priv->combined.renderers, i);
			ppg_renderer_line_append(renderer, model, percent_quark);
		}
	}

	return model;
}


/**
 * ppg_cpu_instrument_manifest_cb:
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
ppg_cpu_instrument_manifest_cb (PkManifest *manifest,
                                gpointer    user_data)
{
	PpgCpuInstrument *instrument = (PpgCpuInstrument *)user_data;
	PpgCpuInstrumentPrivate *priv;
	GHashTableIter iter;
	gpointer key;
	gpointer value;

	g_return_if_fail(PPG_IS_CPU_INSTRUMENT(instrument));

	priv = instrument->priv;

	priv->combined.cpu_row =
		pk_manifest_get_row_id_from_quark(manifest, cpu_quark);

	g_hash_table_iter_init(&iter, priv->combined.models);
	while (g_hash_table_iter_next(&iter, &key, &value)) {
		pk_model_insert_manifest(value, manifest);
	}
}


/**
 * ppg_cpu_instrument_sample_cb:
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
ppg_cpu_instrument_sample_cb (PkManifest *manifest,
                              PkSample   *sample,
                              gpointer    user_data)
{
	PpgCpuInstrument *instrument = (PpgCpuInstrument *)user_data;
	PpgCpuInstrumentPrivate *priv;
	PkModel *model;
	GValue value = { 0 };
	gint cpu;

	g_return_if_fail(PPG_IS_CPU_INSTRUMENT(instrument));

	priv = instrument->priv;

#ifdef PERFKIT_DEBUG
	g_assert_cmpint(pk_sample_get_source_id(sample), ==, priv->source);
	g_assert_cmpint(pk_manifest_get_source_id(manifest), ==, priv->source);
#endif

	pk_sample_get_value(sample, priv->combined.cpu_row, &value);
	cpu = g_value_get_int(&value);
	model = get_model(instrument, manifest, cpu);
	pk_model_insert_sample(model, manifest, sample);
}


/**
 * ppg_cpu_instrument_set_handlers_cb:
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
ppg_cpu_instrument_set_handlers_cb (GObject      *object,
                                    GAsyncResult *result,
                                    gpointer      user_data)
{
	PpgCpuInstrument *instrument = (PpgCpuInstrument *)user_data;
	PpgCpuInstrumentPrivate *priv;
	PkConnection *conn = (PkConnection *)object;
	GError *error = NULL;

	g_return_if_fail(PPG_IS_CPU_INSTRUMENT(instrument));
	g_return_if_fail(PK_IS_CONNECTION(object));

	priv = instrument->priv;

	if (!pk_connection_subscription_set_handlers_finish(conn, result, &error)) {
		g_critical("Failed to subscribe to subscription: %d", priv->subscription);
	}
}


/**
 * ppg_cpu_instrument_load:
 * @instrument: (in): A #PpgCpuInstrument.
 * @session: (in): A #PpgSession.
 * @error: (error): A location for a #GError or %NULL.
 *
 * Loads the required resources for the PpgCpuInstrument to run within the
 * Perfkit agent. Default visualizers are also created.
 *
 * Returns: %TRUE if successful; otherwise %FALSE.
 * Side effects: Cpu source is added to agent.
 */
static gboolean
ppg_cpu_instrument_load (PpgInstrument  *instrument,
                         PpgSession     *session,
                         GError        **error)
{
	PpgCpuInstrument *cpu = (PpgCpuInstrument *)instrument;
	PpgCpuInstrumentPrivate *priv;
	PkConnection *conn;
	gboolean ret = FALSE;
	gint channel;

	g_return_val_if_fail(PPG_IS_CPU_INSTRUMENT(instrument), FALSE);

	priv = cpu->priv;

	g_object_get(session,
	             "channel", &channel,
	             "connection", &conn,
	             NULL);

	RPC_OR_FAILURE(manager_add_source,
	               (conn, "Cpu", &priv->source, error));

	RPC_OR_FAILURE(channel_add_source,
	               (conn, channel, priv->source, error));

	RPC_OR_FAILURE(manager_add_subscription,
	               (conn, 0, 0, &priv->subscription, error));

	RPC_OR_FAILURE(subscription_add_source,
	               (conn, priv->subscription, priv->source, error));

	pk_connection_subscription_set_handlers_async(
			conn, priv->subscription,
			ppg_cpu_instrument_manifest_cb, cpu, NULL,
			ppg_cpu_instrument_sample_cb, cpu, NULL,
			NULL,
			ppg_cpu_instrument_set_handlers_cb, instrument);

	ppg_instrument_add_visualizer(instrument, "combined");

  failure:
	g_object_unref(conn);
	return ret;
}


/**
 * ppg_cpu_instrument_unload:
 * @instrument: (in): A #PpgCpuInstrument.
 * @session: (in): A #PpgSession.
 * @error: (out): A location for a #GError or %NULL.
 *
 * Unloads the instrument and destroys remote resources.
 *
 * Returns: TRUE if successful; otherwise FALSE.
 * Side effects: Agent resources are destroyed.
 */
static gboolean
ppg_cpu_instrument_unload (PpgInstrument  *instrument,
                           PpgSession     *session,
                           GError        **error)
{
	PpgCpuInstrumentPrivate *priv;
	PkConnection *conn;
	gboolean ret = FALSE;
	gboolean removed;

	g_return_val_if_fail(PPG_IS_CPU_INSTRUMENT(instrument), FALSE);
	g_return_val_if_fail(PPG_IS_SESSION(session), FALSE);

	priv = PPG_CPU_INSTRUMENT(instrument)->priv;

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


static void
ppg_cpu_instrument_finalize (GObject *object)
{
	PpgCpuInstrumentPrivate *priv = PPG_CPU_INSTRUMENT(object)->priv;
	GObject *renderer;

	g_hash_table_destroy(priv->combined.models);

	while (priv->combined.renderers->len) {
		renderer = g_ptr_array_index(priv->combined.renderers, 0);
		ppg_cpu_instrument_renderer_disposed(object, renderer);
	}
	g_ptr_array_free(priv->combined.renderers, TRUE);

	G_OBJECT_CLASS(ppg_cpu_instrument_parent_class)->finalize(object);
}


/**
 * ppg_cpu_instrument_class_init:
 * @klass: (in): A #PpgCpuInstrumentClass.
 *
 * Initializes the #PpgCpuInstrumentClass. Quarks are registered for use by
 * instances of the class.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_cpu_instrument_class_init (PpgCpuInstrumentClass *klass)
{
	GObjectClass *object_class;
	PpgInstrumentClass *instrument_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_cpu_instrument_finalize;
	g_type_class_add_private(object_class, sizeof(PpgCpuInstrumentPrivate));

	instrument_class = PPG_INSTRUMENT_CLASS(klass);
	instrument_class->load = ppg_cpu_instrument_load;
	instrument_class->unload = ppg_cpu_instrument_unload;

	cpu_quark = g_quark_from_static_string("CPU Number");
	idle_quark = g_quark_from_static_string("Idle");
	nice_quark = g_quark_from_static_string("Nice");
	percent_quark = g_quark_from_static_string("Percent");
	system_quark = g_quark_from_static_string("System");
	user_quark = g_quark_from_static_string("User");
}


/**
 * ppg_cpu_instrument_init:
 * @instrument: (in): A #PpgCpuInstrument.
 *
 * Initialize a new #PpgCpuInstrument.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_cpu_instrument_init (PpgCpuInstrument *instrument)
{
	instrument->priv = G_TYPE_INSTANCE_GET_PRIVATE(instrument,
	                                               PPG_TYPE_CPU_INSTRUMENT,
	                                               PpgCpuInstrumentPrivate);

	g_object_set(instrument,
	             "title", _("CPU"),
	             NULL);

	ppg_instrument_register_visualizers(PPG_INSTRUMENT(instrument),
	                                    G_N_ELEMENTS(visualizer_entries),
	                                    visualizer_entries,
	                                    instrument);

	instrument->priv->combined.models =
		g_hash_table_new_full(g_int_hash, g_int_equal,
		                      g_free, g_object_unref);
	instrument->priv->combined.renderers = g_ptr_array_new();
}

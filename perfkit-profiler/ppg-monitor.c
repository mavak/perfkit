/* ppg-monitor.c
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

#include <gtk/gtk.h>
#include <perfkit/perfkit.h>

#include "ppg-renderer-line.h"
#include "ppg-rt-graph.h"


static GQuark gQuarkCpu;
static GQuark gQuarkIdle;
static GQuark gQuarkNice;
static GQuark gQuarkPercent;
static GQuark gQuarkSystem;
static GQuark gQuarkUser;
static GHashTable *gModels = NULL;
static GHashTable *gRenderers = NULL;
static GHashTable *gInitialized = NULL;


#define RPC_OR_FAIL(_r, _a) \
	G_STMT_START { \
		gboolean ret = pk_connection_##_r _a; \
		if (!ret) { \
			g_critical("%s", error->message); \
			g_clear_error(&error); \
			return NULL; \
		} \
	} G_STMT_END


static void
add_model (PkConnection *connection,
           PkModel *model,
           gint cpu)
{
	PpgRendererLine *line;
	GPtrArray *renderers;
	GHashTable *models;
	gint *key;
	gint i;

	if (!(models = g_hash_table_lookup(gModels, connection))) {
		g_assert_not_reached();
		return;
	}

	key = g_new0(gint, 1);
	*key = cpu;
	g_hash_table_insert(models, key, model);

	if (!(renderers = g_hash_table_lookup(gRenderers, connection))) {
		g_assert_not_reached();
		return;
	}

	for (i = 0; i < renderers->len; i++) {
		line = g_ptr_array_index(renderers, i);
		ppg_renderer_line_append(line, model, gQuarkPercent);
	}
}


static void
cpu_percent_builder (PkModel *model,
                     PkModelIter *iter,
                     GQuark key,
                     GValue *value,
                     gpointer user_data)
{
	gdouble user;
	gdouble nice_;
	gdouble system;
	gdouble idle;
	gdouble percent;

	user = pk_model_get_int(model, iter, gQuarkUser);
	nice_ = pk_model_get_int(model, iter, gQuarkNice);
	system = pk_model_get_int(model, iter, gQuarkSystem);
	idle = pk_model_get_int(model, iter, gQuarkIdle);

	percent = (user + nice_ + system) /
	          (user + nice_ + system + idle) *
	          100.0;
	g_value_set_double(value, percent);
}


static PkModel*
get_model_for_cpu (PkConnection *connection,
                   PkManifest *manifest,
                   gint cpu)
{
	GHashTable *models;
	PkModel *model;

	if (!(models = g_hash_table_lookup(gModels, connection))) {
		g_assert_not_reached();
		return NULL;
	}

	if (!(model = g_hash_table_lookup(models, &cpu))) {
		model = g_object_new(PK_TYPE_MODEL_MEMORY, NULL);
		pk_model_set_field_mode(model, gQuarkSystem, PK_MODEL_COUNTER);
		pk_model_set_field_mode(model, gQuarkIdle, PK_MODEL_COUNTER);
		pk_model_set_field_mode(model, gQuarkUser, PK_MODEL_COUNTER);
		pk_model_set_field_mode(model, gQuarkNice, PK_MODEL_COUNTER);
		pk_model_register_builder(model, gQuarkPercent, cpu_percent_builder,
		                          NULL, NULL);
		pk_model_insert_manifest(model, manifest);
		add_model(connection, model, cpu);
	}

	return model;
}


static void
cpu_manifest_cb (PkManifest *manifest,
                 gpointer    user_data)
{
	PkConnection *connection = (PkConnection *)user_data;
	GHashTableIter iter;
	GHashTable *models;
	PkModel *model;

	g_return_if_fail(PK_IS_CONNECTION(connection));

	if ((models = g_hash_table_lookup(gModels, connection))) {
		g_hash_table_iter_init(&iter, models);
		while (g_hash_table_iter_next(&iter, NULL, (gpointer *)&model)) {
			pk_model_insert_manifest(model, manifest);
		}
	}
}


static void
cpu_sample_cb (PkManifest *manifest,
               PkSample   *sample,
               gpointer    user_data)
{
	PkConnection *connection = (PkConnection *)user_data;
	PkModel *model;
	GValue value = { 0 };
	gint cpu;
	gint row;

	if ((row = pk_manifest_get_row_id_from_quark(manifest, gQuarkCpu)) <= 0) {
		return;
	}

	pk_sample_get_value(sample, row, &value);
	cpu = g_value_get_int(&value);
	g_value_unset(&value);

	model = get_model_for_cpu(connection, manifest, cpu);
	pk_model_insert_sample(model, manifest, sample);
}


static void
cpu_set_handlers_cb (GObject *object,
                     GAsyncResult *result,
                     gpointer user_data)
{
	PkConnection *connection = (PkConnection *)object;
	GError *error = NULL;
	gint channel;

	if (!pk_connection_subscription_set_handlers_finish(connection,
	                                                    result, &error)) {
		g_critical("%s", error->message);
		g_clear_error(&error);
		return;
	}

	channel = GPOINTER_TO_INT(user_data);
	pk_connection_channel_start(connection, channel, NULL, NULL);
}


GtkWidget *
ppg_monitor_cpu_new (PkConnection *connection)
{
	PpgRenderer *renderer;
	GHashTable *models;
	PpgRtGraph *graph;
	GPtrArray *renderers;
	GError *error = NULL;
	gint channel;
	gint source;
	gint sub;

	/*
	 * Make sure the models hash exists.
	 */
	if (!(models = g_hash_table_lookup(gModels, connection))) {
		models = g_hash_table_new_full(g_int_hash, g_int_equal, g_free, NULL);
		g_hash_table_insert(gModels, connection, models);
	}

	/*
	 * Make sure the renderers hash exists.
	 */
	if (!(renderers = g_hash_table_lookup(gRenderers, connection))) {
		renderers = g_ptr_array_new();
		g_hash_table_insert(gRenderers, connection, renderers);
	}

	/*
	 * Initialize the cpu source if needed.
	 */
	if (!g_hash_table_lookup(gInitialized, connection)) {
		g_hash_table_insert(gInitialized, connection, GINT_TO_POINTER(1));

		RPC_OR_FAIL(manager_add_channel, (connection, &channel, &error));
		RPC_OR_FAIL(manager_add_source, (connection, "Cpu", &source, &error));
		RPC_OR_FAIL(channel_add_source, (connection, channel, source, &error));
		RPC_OR_FAIL(manager_add_subscription, (connection, 0, 0, &sub, &error));
		RPC_OR_FAIL(subscription_add_source, (connection, sub, source, &error));
		pk_connection_subscription_set_handlers_async(
				connection, sub,
		        cpu_manifest_cb, connection, NULL,
		        cpu_sample_cb, connection, NULL,
		        NULL, cpu_set_handlers_cb, GINT_TO_POINTER(channel));
	}

	renderer = g_object_new(PPG_TYPE_RENDERER_LINE,
	                        NULL);
	graph = g_object_new(PPG_TYPE_RT_GRAPH,
	                     "renderer", renderer,
	                     "max-lines", 9,
	                     "visible", TRUE,
	                     NULL);
	ppg_rt_graph_set_flags(graph, PPG_RT_GRAPH_PERCENT);
	g_ptr_array_add(renderers, renderer);

	return GTK_WIDGET(graph);
}


void
ppg_monitor_init (void)
{
	gInitialized =
		g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, NULL);
	gRenderers =
		g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, NULL);
	gModels =
		g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL,
		                      (GDestroyNotify)g_hash_table_destroy);

	gQuarkCpu = g_quark_from_static_string("CPU Number");
	gQuarkIdle = g_quark_from_static_string("Idle");
	gQuarkPercent = g_quark_from_static_string("Percent");
	gQuarkSystem = g_quark_from_static_string("System");
	gQuarkUser = g_quark_from_static_string("User");
}

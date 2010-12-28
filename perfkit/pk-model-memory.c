/* pk-model-memory.c
 *
 * Copyright (C) 2010 Christian Hergert <chris@dronelabs.com>
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

#include "pk-log.h"
#include "pk-model-memory.h"


struct _PkModelMemoryPrivate
{
	GPtrArray  *manifests;
	GHashTable *manifests_index;
	GPtrArray  *samples;
};


G_DEFINE_TYPE(PkModelMemory, pk_model_memory, G_TYPE_OBJECT)


static void
pk_model_memory_insert_manifest (PkModel    *model,
                                 PkManifest *manifest)
{
	PkModelMemory *memory = (PkModelMemory *)model;
	PkModelMemoryPrivate *priv;
	gint index;

	g_return_if_fail(PK_IS_MODEL_MEMORY(memory));

	priv = memory->priv;

	g_ptr_array_add(priv->manifests, pk_manifest_ref(manifest));
	index = priv->manifests->len;
	g_hash_table_insert(priv->manifests_index, manifest,
	                    GINT_TO_POINTER(index));
}


static void
pk_model_memory_insert_sample (PkModel    *model,
                               PkManifest *manifest,
                               PkSample   *sample)
{
	PkModelMemory *memory = (PkModelMemory *)model;
	PkModelMemoryPrivate *priv;

	g_return_if_fail(PK_IS_MODEL_MEMORY(memory));

	priv = memory->priv;

	g_ptr_array_add(priv->samples, pk_sample_ref(sample));
}


static void
set_iter (PkModelMemory *memory,
          PkModelIter   *iter,
          PkSample      *sample)
{
	g_assert_not_reached();
}


static void
get_iter (PkModelMemory  *memory,
          PkModelIter    *iter,
          PkManifest    **manifest,
          PkSample      **sample)
{
	*manifest = NULL;
	*sample = NULL;

	g_assert_not_reached();
}


static gboolean
pk_model_memory_get_iter_first (PkModel     *model,
                                PkModelIter *iter)
{
	PkModelMemoryPrivate *priv;
	PkModelMemory *memory = (PkModelMemory *)model;
	gboolean ret;

	g_return_val_if_fail(PK_IS_MODEL_MEMORY(memory), FALSE);
	g_return_val_if_fail(iter != NULL, FALSE);

	priv = memory->priv;

	if ((ret = !!priv->samples->len)) {
		set_iter(memory, iter, g_ptr_array_index(priv->samples, 0));
	}
	return ret;
}


static gboolean
pk_model_memory_get_iter_for_range (PkModel     *model,
                                    PkModelIter *iter,
                                    gdouble      begin_time,
                                    gdouble      end_time,
                                    gdouble      aggregate_time)
{
	PkModelMemoryPrivate *priv;
	PkModelMemory *memory = (PkModelMemory *)model;

	g_return_val_if_fail(PK_IS_MODEL_MEMORY(memory), FALSE);
	g_return_val_if_fail(iter != NULL, FALSE);

	priv = memory->priv;

	/*
	 * TODO: Binary search for time ranges.
	 *       Store time range
	 */

	return FALSE;
}


static void
pk_model_memory_get_value (PkModel     *model,
                           PkModelIter *iter,
                           GQuark       key,
                           GValue      *value)
{
	PkModelMemoryPrivate *priv;
	PkModelMemory *memory = (PkModelMemory *)model;
	PkManifest *manifest;
	PkSample *sample;
	gint row_id;

	g_return_if_fail(PK_IS_MODEL_MEMORY(memory));

	priv = memory->priv;

	get_iter(memory, iter, &manifest, &sample);
	row_id = pk_manifest_get_row_id_from_quark(manifest, key);
	pk_sample_get_value(sample, row_id, value);
}


/**
 * pk_model_memory_finalize:
 * @object: (in): A #PkModelMemory.
 *
 * Finalizer for a #PkModelMemory instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_model_memory_finalize (GObject *object)
{
	PkModelMemoryPrivate *priv = PK_MODEL_MEMORY(object)->priv;

	g_hash_table_destroy(priv->manifests_index);
	priv->manifests_index = NULL;

	g_ptr_array_foreach(priv->manifests, (GFunc)pk_manifest_unref, NULL);
	g_ptr_array_free(priv->manifests, TRUE);
	priv->manifests = NULL;

	g_ptr_array_foreach(priv->samples, (GFunc)pk_sample_unref, NULL);
	g_ptr_array_free(priv->samples, TRUE);
	priv->samples = NULL;

	G_OBJECT_CLASS(pk_model_memory_parent_class)->finalize(object);
}


/**
 * pk_model_memory_class_init:
 * @klass: (in): A #PkModelMemoryClass.
 *
 * Initializes the #PkModelMemoryClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_model_memory_class_init (PkModelMemoryClass *klass)
{
	GObjectClass *object_class;
	PkModelClass *model_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = pk_model_memory_finalize;
	g_type_class_add_private(object_class, sizeof(PkModelMemoryPrivate));

	model_class = PK_MODEL_CLASS(klass);
	model_class->get_iter_first = pk_model_memory_get_iter_first;
	model_class->get_iter_for_range = pk_model_memory_get_iter_for_range;
	model_class->get_value = pk_model_memory_get_value;
	model_class->insert_manifest = pk_model_memory_insert_manifest;
	model_class->insert_sample = pk_model_memory_insert_sample;
}


/**
 * pk_model_memory_init:
 * @memory: (in): A #PkModelMemory.
 *
 * Initializes the newly created #PkModelMemory instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_model_memory_init (PkModelMemory *memory)
{
	memory->priv = G_TYPE_INSTANCE_GET_PRIVATE(memory, PK_TYPE_MODEL_MEMORY,
	                                           PkModelMemoryPrivate);

	memory->priv->manifests = g_ptr_array_new();
	memory->priv->manifests_index = g_hash_table_new(g_direct_hash,
	                                                 g_direct_equal);
	memory->priv->samples = g_ptr_array_new();
}

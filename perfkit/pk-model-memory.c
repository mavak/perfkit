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


static inline gint
compare_double (const gdouble *xp,
                const gdouble *yp)
{
	const gdouble x = *xp;
	const gdouble y = *yp;

	if (x < y) return -1;
	else if (x > y) return 1;
	else return 0;
}


static gint
pk_model_memory_find_manifest_for_time (PkModelMemory *memory,
                                        gdouble        target_time)
{
	PkModelMemoryPrivate *priv;
	PkManifest **manifests;
	gint left = 0;
	gint middle = 0;
	gint ret;
	gint right;
	guint n_manifests;

	g_return_val_if_fail(PK_IS_MODEL_MEMORY(memory), -1);

	priv = memory->priv;

	/*
	 * If we have no manifests stored, we can immediately fail.
	 */
	if (!(n_manifests = priv->manifests->len)) {
		return -1;
	}

	/*
	 * Lets use the array of manfiests directly. Makes the code a bit
	 * cleaner and less macros used.
	 */
	manifests = (PkManifest **)priv->manifests->pdata;
	right = n_manifests - 1;

	/*
	 * Binary search through the array of samples until we find a match
	 * or have exhausted our divide-and-conquer (which with a normal
	 * binary search would be a failure).
	 */
	while (left <= right) {
		middle = (left + right) / 2;
		ret = compare_double(&manifests[middle]->time, &target_time);
		switch (ret) {
		case -1:
			left = middle + 1;
			break;
		case 1:
			right = middle - 1;
			break;
		case 0:
			return middle;
		default:
			g_assert_not_reached();
			return -1;
		}
	}

	/*
	 * The item on the left must be the closest to the 
	 */
	return left;
}


static gint
pk_model_memory_find_nearest_sample (PkModelMemory *memory,
                                     gdouble        target_time,
                                     gdouble        other_time,
                                     gboolean       prefer_right)
{
	PkModelMemoryPrivate *priv;
	PkSample **samples;
	gint left = 0;
	gint middle = 0;
	gint ret;
	gint right;
	guint n_samples;

	g_return_val_if_fail(PK_IS_MODEL_MEMORY(memory), -1);

	priv = memory->priv;

	/*
	 * If we have no samples stored, we can immediately fail.
	 */
	if (!(n_samples = priv->samples->len)) {
		return -1;
	}

	/*
	 * Lets use the array of samples directly. Makes the code a bit
	 * cleaner and less macros used.
	 */
	samples = (PkSample **)priv->samples->pdata;
	right = n_samples - 1;

	/*
	 * Binary search through the array of samples until we find a match
	 * or have exhausted our divide-and-conquer (which with a normal
	 * binary search would be a failure).
	 */
	while (left <= right) {
		middle = (left + right) / 2;
		ret = compare_double(&samples[middle]->time, &target_time);
		switch (ret) {
		case -1:
			left = middle + 1;
			break;
		case 1:
			right = middle - 1;
			break;
		case 0:
			goto walk;
		default:
			g_assert_not_reached();
			return -1;
		}
	}

  walk:
	/*
	 * This is a twist on the typical binary search. We didn't find the actual
	 * time that we were looking for (which is going to be pretty common).
	 * However, we'd like either the closest item older, or closest item
	 * newer, than the target time. This walks in either direction based
	 * on @prefer_right.
	 */
	if (!prefer_right) {
		while (middle >= 0 && samples[middle]->time >= target_time) {
			middle--;
		}
		if (middle < 0) {
			if (samples[0]->time < other_time) {
				return 0;
			}
			return -1;
		}
		return middle;
	} else {
		while (middle < n_samples && samples[middle]->time <= target_time) {
			middle++;
		}
		if (middle >= n_samples) {
			if (samples[n_samples-1]->time > other_time) {
				return n_samples - 1;
			}
			return -1;
		}
		return middle;
	}
}


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


static inline void
set_iter (PkModelMemory *memory,
          PkModelIter   *iter,
          PkSample      *sample)
{
	gint manifest_idx;

	manifest_idx =
		pk_model_memory_find_manifest_for_time(memory, sample->time);
	g_assert_cmpint(manifest_idx, >, 0);
	iter->user_data =
		g_ptr_array_index(memory->priv->manifests, manifest_idx);
	iter->user_data2 = sample;
}


static inline void
get_iter (PkModelMemory  *memory,
          PkModelIter    *iter,
          PkManifest    **manifest,
          PkSample      **sample)
{
	*manifest = iter->user_data;
	*sample = iter->user_data2;
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
	PkSample *sample;
	gint begin_idx;
	gint end_idx;

	g_return_val_if_fail(PK_IS_MODEL_MEMORY(memory), FALSE);
	g_return_val_if_fail(iter != NULL, FALSE);

	priv = memory->priv;

	/*
	 * TODO: Store time range
	 */

	begin_idx =
		pk_model_memory_find_nearest_sample(memory, begin_time,
		                                    end_time, FALSE);
	end_idx =
		pk_model_memory_find_nearest_sample(memory, begin_time,
		                                    end_time, TRUE);
	if (begin_idx < 0 || end_idx < 0) {
		return FALSE;
	}

	g_assert_cmpint(begin_idx, <, priv->samples->len);
	g_assert_cmpint(end_idx, <, priv->samples->len);

	/*
	 * Create for index range.
	 */
	sample = g_ptr_array_index(priv->samples, begin_idx);

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

	/*
	 * TODO: This is likely to be a hot method that could use some optimization
	 *       on how we determine the row id for lookup. Alternatively, we could
	 *       use a quark like system for all keys and use them.
	 */

	get_iter(memory, iter, &manifest, &sample);
	row_id = pk_manifest_get_row_id_from_quark(manifest, key);
	pk_sample_get_value(sample, row_id, value);
}


gboolean
pk_model_memory_iter_next (PkModel     *model,
                           PkModelIter *iter)
{
	PkModelMemoryPrivate *priv;
	PkModelMemory *memory = (PkModelMemory *)model;
	PkManifest *manifest;
	PkSample *sample;

	g_return_val_if_fail(PK_IS_MODEL_MEMORY(memory), FALSE);
	g_return_val_if_fail(iter != NULL, FALSE);

	priv = memory->priv;

	get_iter(memory, iter, &manifest, &sample);

	/*
	 * TODO: Get the next sample in the array. Update the manifest
	 *       if necessary.
	 */

	return FALSE;
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
	model_class->iter_next = pk_model_memory_iter_next;
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

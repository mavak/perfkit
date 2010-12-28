/* pk-model.c
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

#include "pk-model.h"


struct _PkModelPrivate
{
	GHashTable *accumulators;
	GHashTable *builders;
};


G_DEFINE_ABSTRACT_TYPE(PkModel, pk_model, G_TYPE_OBJECT)


void
pk_model_accumulate (PkModel     *model,
                     PkModelIter *iter,
                     GQuark       key,
                     GValue      *return_value)
{
	PkModelPrivate *priv;
	GValueArray *values;
	GClosure *closure;
	GValue params[2] = {{ 0 }};
	GValue *value;

	g_return_if_fail(PK_IS_MODEL(model));
	g_return_if_fail(iter != NULL);
	g_return_if_fail(return_value != NULL);

	priv = model->priv;

	if (!(closure = g_hash_table_lookup(priv->accumulators, &key))) {
		g_warning("No accumulator registered for key \"%s\"",
		          g_quark_to_string(key));
		return;
	}

	values = g_value_array_new(8);
	g_value_init(&params[0], G_TYPE_VALUE);
	g_value_init(&params[1], G_TYPE_VALUE_ARRAY);

	/*
	 * Retrieve values from iterator.
	 */
	do {
		g_value_array_append(values, NULL);
		value = g_value_array_get_nth(values, values->n_values - 1);
		pk_model_get_value(model, iter, key, value);
	} while (pk_model_iter_next(model, iter));

	/*
	 * Call accumulator to reduce values.
	 */
	g_value_set_boxed(&params[0], return_value);
	g_value_set_boxed(&params[1], values);
	g_closure_invoke(closure, NULL, 2, params, NULL);

	/*
	 * Clean up allocations.
	 */
	g_value_unset(&params[0]);
	g_value_unset(&params[1]);
	g_value_array_free(values);
}


void
pk_model_insert_manifest (PkModel    *model,
                          PkManifest *manifest)
{
	g_return_if_fail(PK_IS_MODEL(model));
	g_return_if_fail(manifest != NULL);

	PK_MODEL_GET_CLASS(model)->insert_manifest(model, manifest);
}


void
pk_model_insert_sample (PkModel    *model,
                        PkManifest *manifest,
                        PkSample   *sample)
{
	g_return_if_fail(PK_IS_MODEL(model));
	g_return_if_fail(manifest != NULL);
	g_return_if_fail(sample != NULL);

	PK_MODEL_GET_CLASS(model)->insert_sample(model, manifest, sample);
}


gboolean
pk_model_get_iter_first (PkModel     *model,
                         PkModelIter *iter)
{
	g_return_val_if_fail(PK_IS_MODEL(model), FALSE);
	g_return_val_if_fail(iter != NULL, FALSE);

	return PK_MODEL_GET_CLASS(model)->get_iter_first(model, iter);
}


gboolean
pk_model_get_iter_for_range (PkModel     *model,
                             PkModelIter *iter,
                             gdouble      begin_time,
                             gdouble      end_time,
                             gdouble      aggregate_time)
{
	g_return_val_if_fail(PK_IS_MODEL(model), FALSE);
	g_return_val_if_fail(iter != NULL, FALSE);

	return PK_MODEL_GET_CLASS(model)->
		get_iter_for_range(model, iter,
		                   begin_time, end_time,
		                   aggregate_time);
}


gboolean
pk_model_iter_next (PkModel     *model,
                    PkModelIter *iter)
{
	g_return_val_if_fail(PK_IS_MODEL(model), FALSE);
	g_return_val_if_fail(iter != NULL, FALSE);

	return PK_MODEL_GET_CLASS(model)->iter_next(model, iter);
}


void
pk_model_get_value (PkModel     *model,
                    PkModelIter *iter,
                    GQuark       key,
                    GValue      *value)
{
	g_return_if_fail(PK_IS_MODEL(model));
	g_return_if_fail(iter != NULL);
	g_return_if_fail(value != NULL);
	g_return_if_fail(G_VALUE_TYPE(value));

	return PK_MODEL_GET_CLASS(model)->get_value(model, iter, key, value);
}


void
pk_model_register_accumulator (PkModel             *model,
                               GQuark               key,
                               PkModelAccumulator   accumulator,
                               gpointer             user_data,
                               GDestroyNotify       notify)
{
	PkModelPrivate *priv;
	GClosure *closure;
	GQuark *pkey;

	g_return_if_fail(PK_IS_MODEL(model));
	g_return_if_fail(key > 0);
	g_return_if_fail(accumulator != NULL);

	priv = model->priv;

	pkey = g_new(GQuark, 1);
	*pkey = key;
	closure = g_cclosure_new(G_CALLBACK(accumulator), user_data,
	                         (GClosureNotify)notify);

	g_hash_table_insert(priv->accumulators, pkey, closure);
}


void
pk_model_register_builder (PkModel        *model,
                           GQuark          key,
                           PkValueBuilder  builder,
                           gpointer        user_data,
                           GDestroyNotify  notify)
{
	PkModelPrivate *priv;
	GClosure *closure;
	GQuark *pkey;

	g_return_if_fail(PK_IS_MODEL(model));
	g_return_if_fail(key > 0);
	g_return_if_fail(builder != NULL);

	priv = model->priv;

	if (!!g_hash_table_lookup(priv->builders, &key)) {
		g_warning("A builder for \"%s\" is already registered",
		          g_quark_to_string(key));
		return;
	}

	pkey = g_new0(GQuark, 1);
	*pkey = key;
	closure = g_cclosure_new(G_CALLBACK(builder), user_data,
	                         (GClosureNotify)notify);
	g_hash_table_insert(priv->builders, pkey, closure);
}


#define GETTER(_name, _type, _TYPE)               \
_type                                             \
pk_model_get_##_name (PkModel     *model,         \
                      PkModelIter *iter,          \
                      GQuark       key)           \
{                                                 \
    GValue value = { 0 };                         \
    _type ret;                                    \
                                                  \
    g_value_init(&value, _TYPE);                  \
    pk_model_get_value(model, iter, key, &value); \
    ret = g_value_get_##_name(&value);            \
    g_value_unset(&value);                        \
    return ret;                                   \
}


GETTER(double, gdouble, G_TYPE_DOUBLE)
GETTER(float, gfloat, G_TYPE_FLOAT)
GETTER(int, gint32, G_TYPE_INT)
GETTER(uint, guint32, G_TYPE_UINT)
GETTER(int64, gint64, G_TYPE_INT64)
GETTER(uint64, guint64, G_TYPE_UINT64)


#define ACCUMULATOR(_name, _type)                                \
void                                                             \
pk_model_accumulate_##_name (PkModel     *model,                 \
                             GValueArray *values,                \
                             GValue      *return_value,          \
                             gpointer     user_data)             \
{                                                                \
	_type total = 0;                                             \
	GValue *value;                                               \
	gint i;                                                      \
                                                                 \
	for (i = 0; i < values->n_values; i++) {                     \
		value = g_value_array_get_nth(values, i);                \
		total += g_value_get_##_name(value);                     \
	}                                                            \
                                                                 \
	g_value_set_##_name(return_value, total / values->n_values); \
}


ACCUMULATOR(double, gdouble)
ACCUMULATOR(float, gfloat)
ACCUMULATOR(int, gint32)
ACCUMULATOR(int64, gint64)
ACCUMULATOR(uint, guint32)
ACCUMULATOR(uint64, guint64)


/**
 * pk_model_finalize:
 * @object: (in): A #PkModel.
 *
 * Finalizer for a #PkModel instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_model_finalize (GObject *object)
{
	G_OBJECT_CLASS(pk_model_parent_class)->finalize(object);
}


/**
 * pk_model_class_init:
 * @klass: (in): A #PkModelClass.
 *
 * Initializes the #PkModelClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_model_class_init (PkModelClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = pk_model_finalize;
	g_type_class_add_private(object_class, sizeof(PkModelPrivate));
}


/**
 * pk_model_init:
 * @model: (in): A #PkModel.
 *
 * Initializes the newly created #PkModel instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
pk_model_init (PkModel *model)
{
	model->priv = G_TYPE_INSTANCE_GET_PRIVATE(model, PK_TYPE_MODEL,
	                                          PkModelPrivate);

	model->priv->accumulators =
		g_hash_table_new_full(g_int_hash, g_int_equal,
		                      g_free, (GDestroyNotify)g_closure_unref);

	model->priv->builders =
		g_hash_table_new_full(g_int_hash, g_int_equal,
		                      g_free, (GDestroyNotify)g_closure_unref);
}

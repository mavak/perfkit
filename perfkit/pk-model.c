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

#include "pk-marshal.h"
#include "pk-model.h"


struct _PkModelPrivate
{
	GHashTable *accumulators;
	GHashTable *builders;
	gdouble     end_time;
};


enum
{
	ACCUMULATOR_ADDED,
	BUILDER_ADDED,

	LAST_SIGNAL
};


enum
{
	PROP_0,

	PROP_END_TIME,

	LAST_PROP
};


G_DEFINE_ABSTRACT_TYPE(PkModel, pk_model, G_TYPE_OBJECT)


static GParamSpec *pspecs[LAST_PROP] = { NULL };
static guint       signals[LAST_SIGNAL] = { 0 };


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

	if (G_LIKELY(sample->time > model->priv->end_time)) {
		model->priv->end_time = sample->time;
		g_object_notify_by_pspec(G_OBJECT(model), pspecs[PROP_END_TIME]);
	}

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
	PkModelPrivate *priv;
	GClosure *closure;
	GValue values[4] = {{ 0 }};

	g_return_if_fail(PK_IS_MODEL(model));
	g_return_if_fail(iter != NULL);
	g_return_if_fail(value != NULL);
	g_return_if_fail(G_VALUE_TYPE(value));

	priv = model->priv;

	if (G_UNLIKELY((closure = g_hash_table_lookup(priv->builders, &key)))) {
		g_value_init(&values[0], G_TYPE_OBJECT);
		g_value_init(&values[1], G_TYPE_POINTER);
		g_value_init(&values[2], G_TYPE_UINT);
		g_value_init(&values[3], G_TYPE_POINTER);

		g_value_set_object(&values[0], model);
		g_value_set_pointer(&values[1], iter);
		g_value_set_uint(&values[2], key);
		g_value_set_pointer(&values[3], value);

		g_closure_invoke(closure, NULL, 4, values, NULL);

		g_value_unset(&values[0]);
		g_value_unset(&values[1]);
		g_value_unset(&values[2]);
		g_value_unset(&values[3]);
	} else {
		PK_MODEL_GET_CLASS(model)->get_value(model, iter, key, value);
	}
}


void
pk_model_set_field_mode (PkModel     *model,
                         GQuark       key,
                         PkModelMode  mode)
{
	g_return_if_fail(PK_IS_MODEL(model));
	g_return_if_fail(key > 0);

	PK_MODEL_GET_CLASS(model)->set_field_mode(model, key, mode);
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

	if (!!g_hash_table_lookup(priv->accumulators, &key)) {
		g_warning("An accumulator named \"%s\" is already registered",
		          g_quark_to_string(key));
		return;
	}

	pkey = g_new(GQuark, 1);
	*pkey = key;
	closure = g_cclosure_new(G_CALLBACK(accumulator), user_data,
	                         (GClosureNotify)notify);
	g_hash_table_insert(priv->accumulators, pkey, closure);
	g_signal_emit(model, signals[ACCUMULATOR_ADDED], 0, key);
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
	g_closure_set_marshal(closure,
	                      pk_cclosure_marshal_VOID__POINTER_UINT_POINTER);
	g_hash_table_insert(priv->builders, pkey, closure);
	g_signal_emit(model, signals[BUILDER_ADDED], 0, key);
}


gdouble
pk_model_get_end_time (PkModel *model)
{
	g_return_val_if_fail(PK_IS_MODEL(model), 0.0);
	return model->priv->end_time;
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
 * pk_model_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (out): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Get a given #GObject property.
 */
static void
pk_model_get_property (GObject    *object,
                       guint       prop_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
	PkModel *model = PK_MODEL(object);

	switch (prop_id) {
	case PROP_END_TIME:
		g_value_set_double(value, model->priv->end_time);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
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
	object_class->get_property = pk_model_get_property;
	g_type_class_add_private(object_class, sizeof(PkModelPrivate));

	pspecs[PROP_END_TIME] = 
		g_param_spec_double("end-time",
		                    "EndTime",
		                    "The most recent sample time",
		                    0.0,
		                    G_MAXDOUBLE,
		                    0.0,
		                    G_PARAM_READABLE);
	g_object_class_install_property(object_class, PROP_END_TIME,
	                                pspecs[PROP_END_TIME]);

	signals[ACCUMULATOR_ADDED] = g_signal_new("accumulator-added",
	                                          PK_TYPE_MODEL,
	                                          G_SIGNAL_RUN_FIRST,
	                                          G_STRUCT_OFFSET(PkModelClass, accumulator_added),
	                                          NULL,
	                                          NULL,
	                                          g_cclosure_marshal_VOID__INT,
	                                          G_TYPE_NONE,
	                                          1,
	                                          G_TYPE_INT);

	signals[BUILDER_ADDED] = g_signal_new("builder-added",
	                                      PK_TYPE_MODEL,
	                                      G_SIGNAL_RUN_FIRST,
	                                      G_STRUCT_OFFSET(PkModelClass, builder_added),
	                                      NULL,
	                                      NULL,
	                                      g_cclosure_marshal_VOID__INT,
	                                      G_TYPE_NONE,
	                                      1,
	                                      G_TYPE_INT);
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

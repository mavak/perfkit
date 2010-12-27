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
	gpointer dummy;
};


G_DEFINE_ABSTRACT_TYPE(PkModel, pk_model, G_TYPE_OBJECT)


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
}

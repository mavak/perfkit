/* pk-model.h
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

#ifndef PK_MODEL_H
#define PK_MODEL_H

#include <glib-object.h>

#include "pk-manifest.h"
#include "pk-sample.h"

G_BEGIN_DECLS

#define PK_TYPE_MODEL            (pk_model_get_type())
#define PK_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PK_TYPE_MODEL, PkModel))
#define PK_MODEL_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PK_TYPE_MODEL, PkModel const))
#define PK_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PK_TYPE_MODEL, PkModelClass))
#define PK_IS_MODEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PK_TYPE_MODEL))
#define PK_IS_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PK_TYPE_MODEL))
#define PK_MODEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PK_TYPE_MODEL, PkModelClass))

typedef struct _PkModel        PkModel;
typedef struct _PkModelClass   PkModelClass;
typedef struct _PkModelPrivate PkModelPrivate;
typedef struct _PkModelIter    PkModelIter;

typedef void (*PkModelAccumulator) (PkModel     *model,
                                    GValueArray *values,
                                    GValue      *return_value,
                                    gpointer     user_data);

struct _PkModelIter
{
	gdouble time;

	/*< private >*/
	gpointer user_data;
	gpointer user_data2;
	gpointer user_data3;
	gpointer user_data4;
};

struct _PkModel
{
	GObject parent;

	/*< private >*/
	PkModelPrivate *priv;
};

struct _PkModelClass
{
	GObjectClass parent_class;

	gboolean (*get_iter_first)     (PkModel     *model,
	                                PkModelIter *iter);
	gboolean (*get_iter_for_range) (PkModel     *model,
	                                PkModelIter *iter,
	                                gdouble      begin_time,
	                                gdouble      end_time,
	                                gdouble      aggregate_time);
	void     (*get_value)          (PkModel     *model,
	                                PkModelIter *iter,
	                                GQuark       key,
	                                GValue      *value);
	void     (*insert_manifest)    (PkModel     *model,
	                                PkManifest  *manifest);
	void     (*insert_sample)      (PkModel     *model,
	                                PkManifest  *manifest,
	                                PkSample    *sample);
	gboolean (*iter_next)          (PkModel     *model,
	                                PkModelIter *iter);
};

void     pk_model_accumulate           (PkModel             *model,
                                        PkModelIter         *iter,
                                        GQuark               key,
                                        GValue              *return_value);
void     pk_model_accumulate_double    (PkModel             *model,
                                        GValueArray         *values,
                                        GValue              *return_value,
                                        gpointer             user_data);
void     pk_model_accumulate_float     (PkModel             *model,
                                        GValueArray         *values,
                                        GValue              *return_value,
                                        gpointer             user_data);
void     pk_model_accumulate_int       (PkModel             *model,
                                        GValueArray         *values,
                                        GValue              *return_value,
                                        gpointer             user_data);
void     pk_model_accumulate_int64     (PkModel             *model,
                                        GValueArray         *values,
                                        GValue              *return_value,
                                        gpointer             user_data);
void     pk_model_accumulate_uint      (PkModel             *model,
                                        GValueArray         *values,
                                        GValue              *return_value,
                                        gpointer             user_data);
void     pk_model_accumulate_uint64    (PkModel             *model,
                                        GValueArray         *values,
                                        GValue              *return_value,
                                        gpointer             user_data);
gdouble  pk_model_get_double           (PkModel             *model,
                                        PkModelIter         *iter,
                                        GQuark               key);
gfloat   pk_model_get_float            (PkModel             *model,
                                        PkModelIter         *iter,
                                        GQuark               key);
gint32   pk_model_get_int              (PkModel             *model,
                                        PkModelIter         *iter,
                                        GQuark               key);
gint64   pk_model_get_int64            (PkModel             *model,
                                        PkModelIter         *iter,
                                        GQuark               key);
gboolean pk_model_get_iter_first       (PkModel             *model,
                                        PkModelIter         *iter);
gboolean pk_model_get_iter_for_range   (PkModel             *model,
                                        PkModelIter         *iter,
                                        gdouble              begin_time,
                                        gdouble              end_time,
                                        gdouble              aggregate_time);
GType    pk_model_get_type             (void) G_GNUC_CONST;
guint32  pk_model_get_uint             (PkModel             *model,
                                        PkModelIter         *iter,
                                        GQuark               key);
guint64  pk_model_get_uint64           (PkModel             *model,
                                        PkModelIter         *iter,
                                        GQuark               key);
void     pk_model_get_value            (PkModel             *model,
                                        PkModelIter         *iter,
                                        GQuark               key,
                                        GValue              *value);
void     pk_model_insert_manifest      (PkModel             *model,
                                        PkManifest          *manifest);
void     pk_model_insert_sample        (PkModel             *model,
                                        PkManifest          *manifest,
                                        PkSample            *sample);
gboolean pk_model_iter_next            (PkModel             *model,
                                        PkModelIter         *iter);
void     pk_model_register_accumulator (PkModel             *model,
                                        GQuark               key,
                                        PkModelAccumulator   accumulator,
                                        gpointer             user_data,
                                        GDestroyNotify       notify);

G_END_DECLS

#endif /* PK_MODEL_H */

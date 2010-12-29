/* ppg-model.c
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

#include <egg-time.h>
#include <gobject/gvaluecollector.h>
#include <math.h>
#include <perfkit/perfkit.h>
#include <string.h>

#include "ppg-log.h"
#include "ppg-model.h"
#include "ppg-session.h"

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "Model"

/**
 * SECTION:ppg-model
 *
 * #PpgModel will start out as a specific implementation for storing data
 * and accessing it in an iterator based fashion. As this continues, I
 * really hope to start abstracting it into a few backends (such as memory
 * and mmap'd on disk) for particular scenarios.
 *
 * I want to re-iterate, this is a very crappy first attempt to do this
 * so don't hold me to it while I figure this out.
 *
 * -- Christian
 */

G_DEFINE_TYPE(PpgModel, ppg_model, G_TYPE_INITIALLY_UNOWNED)

typedef struct
{
	gint               key;           /* User definable key */
	PpgModelType       type;          /* Model type */
	gchar             *name;          /* Name of field in manifest */
	GType              expected_type; /* Expected GType */
	PpgModelValueFunc  func;          /* Closure callback */
	gpointer           user_data;     /* User data for closure */
	gboolean           track_range;   /* Should we track bounds */
	gdouble            lower;         /* Lower tracked bounds */
	gdouble            upper;         /* Upper tracked bounds */
	gint               current_row;   /* Name's row in current manifest */
} Mapping;

struct _PpgModelPrivate
{
	guint32     stamp;

	PpgSession *session;         /* The profiling session */
	PkManifest *manifest;        /* Current manifest */
	guint32     sample_count;    /* Samples on current manifest */
	GPtrArray  *manifests;       /* All manifests */
	GHashTable *mappings;        /* Key to name mappings */
	GHashTable *next_manifests;  /* Manifest->NextManifest mappings */
	GPtrArray  *samples;         /* All samples */
	GArray     *sample_times;    /* Relative offset of all samples */
};

enum
{
	PROP_0,
	PROP_SESSION,
};

enum
{
	CHANGED,
	LAST_SIGNAL
};

enum
{
	BEFORE = 1,
	AFTER  = 2,
};

static guint signals[LAST_SIGNAL] = { 0 };

static inline void
_g_value_subtract (GValue *left,
                   GValue *right)
{
	g_assert_cmpint(left->g_type, ==, right->g_type);

	switch (left->g_type) {
	case G_TYPE_INT: {
		gint x = g_value_get_int(left);
		gint y = g_value_get_int(right);
		g_value_set_int(left, x - y);
		break;
	}
	case G_TYPE_UINT: {
		guint x = g_value_get_uint(left);
		guint y = g_value_get_uint(right);
		g_value_set_uint(left, x - y);
		break;
	}
	case G_TYPE_LONG: {
		glong x = g_value_get_long(left);
		glong y = g_value_get_long(right);
		g_value_set_long(left, x - y);
		break;
	}
	case G_TYPE_ULONG: {
		gulong x = g_value_get_ulong(left);
		gulong y = g_value_get_ulong(right);
		g_value_set_ulong(left, x - y);
		break;
	}
	case G_TYPE_FLOAT: {
		gfloat x = g_value_get_float(left);
		gfloat y = g_value_get_float(right);
		g_value_set_float(left, x - y);
		break;
	}
	case G_TYPE_DOUBLE: {
		gdouble x = g_value_get_double(left);
		gdouble y = g_value_get_double(right);
		g_value_set_double(left, x - y);
		break;
	}
	default:
		/* XXX: Add support for type. */
		g_assert_not_reached();
	}
}

static gboolean
mapping_get_value (Mapping *mapping,
                   PpgModel *model,
                   PpgModelIter *iter,
                   PkManifest *manifest,
                   PkSample *sample,
                   GValue  *value)
{
	PpgModelPrivate *priv;
	GValue last_value = { 0 };
	PkSample *last;
	GType type;
	gint row;

	g_return_val_if_fail(mapping != NULL, FALSE);
	g_return_val_if_fail(manifest != NULL, FALSE);
	g_return_val_if_fail(sample != NULL, FALSE);
	g_return_val_if_fail(value != NULL, FALSE);

	priv = model->priv;

	if (mapping->func) {
		return mapping->func(model, iter, mapping->key, value,
		                     mapping->user_data);
	}

	/*
	 * FIXME: Probably should cache these somewhere per manifest.
	 */
	row = pk_manifest_get_row_id(manifest, mapping->name);
	if (mapping->expected_type) {
		type = pk_manifest_get_row_type(manifest, row);
		if (type != mapping->expected_type) {
			g_critical("Incoming type %s does not match %s",
			           g_type_name(type),
			           g_type_name(mapping->expected_type));
			return FALSE;
		}
	}

	if (mapping->type == PPG_MODEL_COUNTER) {
		/*
		 * We need to get the last value and subtract it.
		 */
		if (!iter->index) {
			g_value_init(value, mapping->expected_type);
			return FALSE;
		}

		last = g_ptr_array_index(priv->samples, iter->index - 1);
		/*
		 * FIXME: There is a (unlikely) chance that the last sample
		 *        was not part of this manifest and the row id is
		 *        different.
		 */
		if (!pk_sample_get_value(last, row, &last_value) ||
		    !pk_sample_get_value(sample, row, value)) {
			g_value_init(value, pk_manifest_get_row_type(manifest, row));
		} else {
			_g_value_subtract(value, &last_value);
		}

		return TRUE;
	} else {
		if (!pk_sample_get_value(sample, row, value)) {
			g_value_init(value, pk_manifest_get_row_type(manifest, row));
			return FALSE;
		}

		return TRUE;
	}
}

static void
ppg_model_set_session (PpgModel   *model,
                       PpgSession *session)
{
	g_return_if_fail(PPG_IS_MODEL(model));
	g_return_if_fail(PPG_IS_SESSION(session));

	model->priv->session = session;
}

/**
 * ppg_model_finalize:
 * @object: (in): A #PpgModel.
 *
 * Finalizer for a #PpgModel instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_model_finalize (GObject *object)
{
	PpgModelPrivate *priv = PPG_MODEL(object)->priv;

	g_ptr_array_foreach(priv->manifests, (GFunc)pk_manifest_unref, NULL);
	g_ptr_array_unref(priv->manifests);

	g_ptr_array_foreach(priv->samples, (GFunc)pk_sample_unref, NULL);
	g_ptr_array_unref(priv->samples);

	g_array_unref(priv->sample_times);

	g_hash_table_unref(priv->mappings);
	g_hash_table_unref(priv->next_manifests);

	G_OBJECT_CLASS(ppg_model_parent_class)->finalize(object);
}

/**
 * ppg_model_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
static void
ppg_model_set_property (GObject      *object,
                        guint         prop_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
	PpgModel *model = PPG_MODEL(object);

	switch (prop_id) {
	case PROP_SESSION:
		ppg_model_set_session(model, g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * ppg_model_class_init:
 * @klass: (in): A #PpgModelClass.
 *
 * Initializes the #PpgModelClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_model_class_init (PpgModelClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_model_finalize;
	object_class->set_property = ppg_model_set_property;
	g_type_class_add_private(object_class, sizeof(PpgModelPrivate));

	g_object_class_install_property(object_class,
	                                PROP_SESSION,
	                                g_param_spec_object("session",
	                                                    "session",
	                                                    "session",
	                                                    PPG_TYPE_SESSION,
	                                                    G_PARAM_WRITABLE));

	signals[CHANGED] = g_signal_new("changed",
	                                PPG_TYPE_MODEL,
	                                G_SIGNAL_RUN_FIRST,
	                                0,
	                                NULL,
	                                NULL,
	                                g_cclosure_marshal_VOID__VOID,
	                                G_TYPE_NONE,
	                                0);
}

/**
 * ppg_model_init:
 * @model: (in): A #PpgModel.
 *
 * Initializes the newly created #PpgModel instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_model_init (PpgModel *model)
{
	PpgModelPrivate *priv;

	priv = G_TYPE_INSTANCE_GET_PRIVATE(model, PPG_TYPE_MODEL,
	                                   PpgModelPrivate);
	model->priv = priv;

	priv->stamp = g_random_int();
	priv->manifests = g_ptr_array_new();
	priv->samples = g_ptr_array_new();
	priv->sample_times = g_array_new(FALSE, FALSE, sizeof(gdouble));
	priv->mappings = g_hash_table_new_full(g_int_hash, g_int_equal,
	                                       NULL, g_free);
	priv->next_manifests = g_hash_table_new(g_direct_hash, g_direct_equal);
}

void
ppg_model_insert_manifest (PpgModel   *model,
                           PkManifest *manifest)
{
	PpgModelPrivate *priv;
	GHashTableIter iter;
	Mapping *mapping;

	g_return_if_fail(PPG_IS_MODEL(model));
	g_return_if_fail(manifest != NULL);

	priv = model->priv;

	/*
	 * Add the mapping from the current sample to the new sample; but only
	 * if we actually received a sample on this manifest.
	 */
	if (priv->manifest && priv->sample_count) {
		g_hash_table_insert(priv->next_manifests, priv->manifest, manifest);
	}

	/*
	 * FIXME: Use insertion sort to order based on timing.
	 */
	priv->manifest = manifest;
	priv->sample_count = 0;
	g_ptr_array_add(priv->manifests, pk_manifest_ref(manifest));

	/*
	 * Update each mapping with current row id. This allows us to avoid a
	 * lookup when a new sample is delivered while determine bounds.
	 */
	g_hash_table_iter_init(&iter, priv->mappings);
	while (g_hash_table_iter_next(&iter, NULL, (gpointer *)&mapping)) {
		if (mapping->name) {
			mapping->current_row =
				pk_manifest_get_row_id(manifest, mapping->name);
		}
	}
}

void
ppg_model_insert_sample (PpgModel   *model,
                         PkManifest *manifest,
                         PkSample   *sample)
{
	PpgModelPrivate *priv;
	GHashTableIter iter;
	Mapping *mapping;
	GValue value = { 0 };
	gdouble dval = 0;
	gdouble time_;

	g_return_if_fail(PPG_IS_MODEL(model));
	g_return_if_fail(manifest != NULL);

	priv = model->priv;
	g_assert(manifest == priv->manifest);

	/*
	 * Track bounds if needed.
	 */
	g_hash_table_iter_init(&iter, priv->mappings);
	while (g_hash_table_iter_next(&iter, NULL, (gpointer *)&mapping)) {
		if (mapping->track_range) {
			if (mapping->current_row > 0) {
				if (pk_sample_get_value(sample, mapping->current_row, &value)) {
					switch (value.g_type) {
					case G_TYPE_DOUBLE:
						dval = g_value_get_double(&value);
						break;
					case G_TYPE_INT:
						dval = g_value_get_int(&value);
						break;
					case G_TYPE_UINT:
						dval = g_value_get_uint(&value);
						break;
					case G_TYPE_FLOAT:
						dval = g_value_get_float(&value);
						break;
					default:
						/* TODO */
						g_assert_not_reached();
					}
					if (dval > mapping->upper) {
						mapping->upper = dval;
					} else if (dval < mapping->lower) {
						mapping->lower = dval;
					}
					g_value_unset(&value);
				}
			}
		}
	}

	/*
	 * FIXME: Use insertion sort to order based on timestamp.
	 */
	priv->sample_count++;
	g_ptr_array_add(priv->samples, pk_sample_ref(sample));

	/*
	 * Get the relative time to the start of the session and store it
	 * so we can binary search when looking for a sample based on time.
	 */
	time_ = ppg_session_convert_time(priv->session, sample->time);
	g_array_append_val(priv->sample_times, time_);

	g_signal_emit(model, signals[CHANGED], 0);
}

void
ppg_model_add_mapping (PpgModel     *model,
                       gint          key,
                       const gchar  *field,
                       GType         expected_type,
                       PpgModelType  type)
{
	PpgModelPrivate *priv;
	Mapping *mapping;

	g_return_if_fail(PPG_IS_MODEL(model));

	priv = model->priv;

	mapping = g_new0(Mapping, 1);
	mapping->key = key;
	mapping->name = g_strdup(field);
	mapping->expected_type = expected_type;
	mapping->type = type;

	g_hash_table_insert(priv->mappings, &mapping->key, mapping);
}

void
ppg_model_add_mapping_func (PpgModel          *model,
                            gint               key,
                            PpgModelValueFunc  func,
                            gpointer           user_data)
{
	PpgModelPrivate *priv;
	Mapping *mapping;

	g_return_if_fail(PPG_IS_MODEL(model));

	priv = model->priv;

	mapping = g_new0(Mapping, 1);
	mapping->key = key;
	mapping->func = func;
	mapping->user_data = user_data;

	g_hash_table_insert(priv->mappings, &mapping->key, mapping);
}

void
ppg_model_get_valist (PpgModel     *model,
                      PpgModelIter *iter,
                      gint          first_key,
                      va_list       args)
{
	PpgModelPrivate *priv;
	Mapping *mapping;
	GValue value = { 0 };
	gchar *error = NULL;
	gint key;

	g_return_if_fail(PPG_IS_MODEL(model));
	g_return_if_fail(iter != NULL);

	priv = model->priv;
	key = first_key;

	do {
		mapping = g_hash_table_lookup(priv->mappings, &key);

		if (!mapping) {
			g_critical("Cannot find mapping for key %d. "
			           "Make sure to include a -1 sentinel.",
			           key);
			return;
		}

		g_assert(iter->manifest);
		g_assert(iter->sample);
		g_assert(mapping);

		mapping_get_value(mapping, model, iter, iter->manifest,
		                  iter->sample, &value);
		G_VALUE_LCOPY(&value, args, 0, &error);
		if (error) {
			g_warning("%s:%s", G_STRFUNC, error);
			g_free(error);
			error = NULL;
		}
		if (value.g_type) {
			g_value_unset(&value);
		}
	} while (-1 != (key = va_arg(args, gint)));
}

void
ppg_model_get (PpgModel     *model,
               PpgModelIter *iter,
               gint          first_key,
               ...)
{
	va_list args;

	va_start(args, first_key);
	ppg_model_get_valist(model, iter, first_key, args);
	va_end(args);
}

gboolean
ppg_model_get_value (PpgModel     *model,
                     PpgModelIter *iter,
                     gint          key,
                     GValue       *value)
{
	PpgModelPrivate *priv;
	Mapping *mapping;

	g_return_val_if_fail(PPG_IS_MODEL(model), FALSE);
	g_return_val_if_fail(iter != NULL, FALSE);
	g_return_val_if_fail(value != NULL, FALSE);

	priv = model->priv;

	mapping = g_hash_table_lookup(priv->mappings, &key);

	g_assert(iter->manifest);
	g_assert(iter->sample);
	g_assert(mapping);

	return mapping_get_value(mapping, model, iter, iter->manifest,
	                         iter->sample, value);
}

gboolean
ppg_model_iter_next (PpgModel     *model,
                     PpgModelIter *iter)
{
	PpgModelPrivate *priv;
	PkManifest *next_manifest;

	g_return_val_if_fail(PPG_IS_MODEL(model), FALSE);
	g_return_val_if_fail(iter != NULL, FALSE);

	priv = model->priv;

	/*
	 * Move to the next sample in the array.
	 */

	iter->index++;
	if (iter->index > iter->end) {
		return FALSE;
	}

	iter->sample = g_ptr_array_index(priv->samples, iter->index);

	/*
	 * Update the time position of the sample.
	 */
	iter->time = ppg_session_convert_time(priv->session, iter->sample->time);

	/*
	 * The sample is guaranteed to be within either the current manifest
	 * or the following manifest since we discard manifests that contained
	 * no samples.
	 */
	if ((next_manifest = g_hash_table_lookup(priv->next_manifests,
	                                         iter->manifest))) {
		if (next_manifest->time <= iter->sample->time) {
			iter->manifest = next_manifest;
		}
	}

	return TRUE;
}

gboolean
ppg_model_get_iter_first (PpgModel     *model,
                          PpgModelIter *iter)
{
	PpgModelPrivate *priv;

	g_return_val_if_fail(PPG_IS_MODEL(model), FALSE);
	g_return_val_if_fail(iter != NULL, FALSE);

	priv = model->priv;

	if (priv->manifests->len) {
		if (priv->samples->len) {
			memset(iter, 0, sizeof *iter);
			iter->manifest = g_ptr_array_index(priv->manifests, 0);
			iter->sample = g_ptr_array_index(priv->samples, 0);
			iter->index = 0;
			iter->begin = 0;
			iter->end = priv->samples->len - 1;
			iter->time = ppg_session_convert_time(priv->session,
			                                      iter->sample->time);
			return TRUE;
		}
	}

	return FALSE;
}

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
ppg_model_binary_search (PpgModel *model,
                         gdouble   target,
                         gdouble   other,
                         gint      mode)
{
	PpgModelPrivate *priv;
	GArray *ar;
	gdouble *dar;
	gint left;
	gint right;
	gint middle = 0;

	g_return_val_if_fail(PPG_IS_MODEL(model), -1);

	priv = model->priv;
	ar = priv->sample_times;
	dar = (gdouble *)ar->data;

	if (!ar->len) {
		return -1;
	}

	left = 0;
	right = ar->len - 1;

	while (left <= right) {
		middle = (left + right) / 2;
		switch (compare_double(&dar[middle], &target)) {
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
		}
	}

walk:
	switch (mode) {
	case BEFORE:
		while (middle >= 0 && dar[middle] >= target) {
			middle--;
		}
		if (middle < 0) {
			if (dar[0] < other) {
				return 0;
			}
			return -1;
		}
		return middle;
	case AFTER:
		while (middle < ar->len && dar[middle] <= target) {
			middle++;
		}
		if (middle >= ar->len) {
			if (dar[ar->len - 1] > other) {
				return ar->len - 1;
			}
			return -1;
		}
		return middle;
	default:
		g_assert_not_reached();
		return -1;
	}
}

gboolean
ppg_model_get_iter_at (PpgModel      *model,
                       PpgModelIter  *iter,
                       gdouble        begin,
                       gdouble        end,
                       PpgResolution  resolution) /* UNUSED */
{
	PpgModelPrivate *priv;
	gint begin_idx;
	gint end_idx;

	g_return_val_if_fail(PPG_IS_MODEL(model), FALSE);
	g_return_val_if_fail(iter != NULL, FALSE);
	g_return_val_if_fail(begin >= 0.0, FALSE);
	g_return_val_if_fail(end >= 0.0, FALSE);

	priv = model->priv;

	/*
	 * TODO:
	 *
	 * It would be very smart for us to aggregate values together in an index
	 * and then iterate the index rather than every value if @resolution
	 * allows for that.
	 */

	if ((begin == 0.0) && (end == 0.0)) {
		return ppg_model_get_iter_first(model, iter);
	}

	/*
	 * XXX:
	 *
	 * If we keep an index of all the relative timestamps (as doubles) for the
	 * samples we store, we can have an easy way to binary search through the
	 * items for our desired first sample. It will cost 8-bytes * N_SAMPLES
	 * memory. However, this seems like a reasonable index once we move to
	 * file-based storage.
	 */

	begin_idx = ppg_model_binary_search(model, begin, end, BEFORE);
	end_idx = ppg_model_binary_search(model, end, begin, AFTER);

	/*
	 * There are no items that fall within the range.
	 */
	if (begin_idx == -1 || end_idx == -1) {
		return FALSE;
	}

	g_assert_cmpint(begin_idx, !=, -1);
	g_assert_cmpint(end_idx, !=, -1);

	/*
	 * Build the iter for the given range.
	 */
	memset(iter, 0, sizeof *iter);
	iter->manifest = g_ptr_array_index(priv->manifests, 0); /* FIXME */
	iter->sample = g_ptr_array_index(priv->samples, begin_idx);
	iter->begin = begin_idx;
	iter->index = begin_idx;
	iter->end = end_idx;
	iter->time = ppg_session_convert_time(priv->session,
	                                      iter->sample->time);

	return TRUE;
}

void
ppg_model_set_track_range (PpgModel *model,
                           gint      key,
                           gboolean  track_range)
{
	PpgModelPrivate *priv;
	Mapping *mapping;

	g_return_if_fail(PPG_IS_MODEL(model));

	priv = model->priv;

	mapping = g_hash_table_lookup(priv->mappings, &key);
	if (!mapping) {
		CRITICAL(Model, "Must call ppg_model_add_mapping() before %s()",
		         G_STRFUNC);
		return;
	}

	/*
	 * XXX: Should we retroactively determine the range for all samples?
	 */
	mapping->track_range = track_range;
}

void
ppg_model_get_range (PpgModel *model,
                     gint      key,
                     gdouble  *lower,
                     gdouble  *upper)
{
	PpgModelPrivate *priv;
	Mapping *mapping;

	g_return_if_fail(PPG_IS_MODEL(model));

	priv = model->priv;

	mapping = g_hash_table_lookup(priv->mappings, &key);
	if (!mapping) {
		CRITICAL(Model, "No mapping found for %d", key);
	}

	if (lower) {
		*lower = mapping->lower;
	}
	if (upper) {
		*upper = mapping->upper;
	}
}

gdouble
ppg_model_get_last_time (PpgModel *model)
{
	PpgModelPrivate *priv;
	PkSample *sample;

	g_return_val_if_fail(PPG_IS_MODEL(model), 0.0);

	priv = model->priv;

	if (!priv->samples->len) {
		return 0.0;
	}

	sample = g_ptr_array_index(priv->samples, priv->samples->len - 1);
	return ppg_session_convert_time(priv->session, sample->time);
}

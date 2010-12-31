/* ppg-instrument.c
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

#include "ppg-instrument.h"
#include "ppg-log.h"
#include "ppg-session.h"


struct _PpgInstrumentPrivate
{
	GArray     *entries;
	PpgSession *session;
	gchar      *title;
	GPtrArray  *visualizers;
};


enum
{
	PROP_0,

	PROP_SESSION,
	PROP_TITLE,
};


enum
{
	VISUALIZER_ADDED,
	VISUALIZER_REMOVED,

	LAST_SIGNAL
};


G_DEFINE_ABSTRACT_TYPE(PpgInstrument, ppg_instrument, G_TYPE_INITIALLY_UNOWNED)


/*
 * Globals.
 */
static guint signals[LAST_SIGNAL] = { 0 };


PpgVisualizerEntry*
ppg_instrument_get_entries (PpgInstrument *instrument,
                            gsize         *n_entries)
{
	PpgInstrumentPrivate *priv;

	g_return_val_if_fail(PPG_IS_INSTRUMENT(instrument), NULL);

	priv = instrument->priv;

	*n_entries = priv->entries->len;
	return (PpgVisualizerEntry *)priv->entries->data;
}


GtkWidget*
ppg_instrument_create_data_view (PpgInstrument *instrument)
{
	PpgInstrumentClass *klass;
	GtkWidget *ret = NULL;

	g_return_val_if_fail(PPG_IS_INSTRUMENT(instrument), NULL);

	klass = PPG_INSTRUMENT_GET_CLASS(instrument);
	if (klass->create_data_view) {
		ret = klass->create_data_view(instrument);
	}
	return ret;
}


PpgSession*
ppg_instrument_get_session (PpgInstrument *instrument)
{
	g_return_val_if_fail(PPG_IS_INSTRUMENT(instrument), NULL);
	return instrument->priv->session;
}


static void
ppg_instrument_set_session (PpgInstrument *instrument,
                            PpgSession    *session)
{
	PpgInstrumentPrivate *priv;
	GError *error = NULL;

	g_return_if_fail(PPG_IS_INSTRUMENT(instrument));
	g_return_if_fail(PPG_IS_SESSION(session));

	priv = instrument->priv;

	priv->session = g_object_ref(session);

	if (PPG_INSTRUMENT_GET_CLASS(instrument)->load) {
		if (!PPG_INSTRUMENT_GET_CLASS(instrument)->load(instrument,
		                                                session,
		                                                &error)) {
			CRITICAL(Instrument, "%s", error->message);
			g_clear_error(&error);
		}
	}
}


/**
 * ppg_instrument_get_title:
 * @instrument: (in): A #PpgInstrument.
 *
 * Retrieves the title of the instrument.
 *
 * Returns: A string which should not be modified or freed.
 * Side effects: None.
 */
const gchar *
ppg_instrument_get_title (PpgInstrument *instrument)
{
	g_return_val_if_fail(PPG_IS_INSTRUMENT(instrument), NULL);
	return instrument->priv->title;
}


/**
 * ppg_instrument_set_title:
 * @instrument: (in): A #PpgInstrument.
 * @title: (in): A string.
 *
 * Sets the title for an instrument.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_instrument_set_title (PpgInstrument *instrument,
                          const gchar   *title)
{
	g_return_if_fail(PPG_IS_INSTRUMENT(instrument));
	g_return_if_fail(instrument->priv->title == NULL);

	instrument->priv->title = g_strdup(title);
	g_object_notify(G_OBJECT(instrument), "title");
}


/**
 * ppg_instrument_register_visualizers:
 * @instrument: (in): A #PpgInstrument.
 *
 * Register a set of visualizers that can be created by this instrument.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_instrument_register_visualizers (PpgInstrument      *instrument,
                                     guint               n_entries,
                                     PpgVisualizerEntry *entries,
                                     gpointer            user_data)
{
	PpgInstrumentPrivate *priv;
	PpgVisualizerEntry entry;
	gint i;

	g_return_if_fail(PPG_IS_INSTRUMENT(instrument));
	g_return_if_fail(entries != NULL);

	priv = instrument->priv;

	for (i = 0; i < n_entries; i++) {
		memset(&entry, 0, sizeof entry);
		entry.name = g_strdup(entries[i].name);
		entry.title = g_strdup(entries[i].title);
		entry.icon_name = g_strdup(entries[i].icon_name);
		entry.callback = entries[i].callback;
		entry.user_data = user_data;
		g_array_append_val(priv->entries, entry);
	}
}


/**
 * ppg_instrument_add_visualizer:
 * @instrument: (in): A #PpgInstrument.
 * @name: (in): The name of the visualizer.
 *
 * Requests that a visualizer by the name of @name be created.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_instrument_add_visualizer (PpgInstrument *instrument,
                               const gchar   *name)
{
	PpgInstrumentPrivate *priv;
	PpgVisualizerEntry *entry;
	PpgVisualizerFactory factory;
	PpgVisualizer *visualizer;
	GError *error = NULL;
	gint i;

	g_return_if_fail(PPG_IS_INSTRUMENT(instrument));
	g_return_if_fail(name != NULL);

	priv = instrument->priv;

	for (i = 0; i < priv->entries->len; i++) {
		entry = &g_array_index(priv->entries, PpgVisualizerEntry, i);
		if (!g_strcmp0(name, entry->name)) {
			factory = (gpointer)entry->callback;
			if (!(visualizer = factory(entry, entry->user_data, &error))) {
				/*
				 * TODO: Show error message.
				 */
				g_assert_not_reached();
			}
			g_signal_emit(instrument, signals[VISUALIZER_ADDED],
			              0, visualizer);
			break;
		}
	}
}


/**
 * ppg_instrument_visualizer_added:
 * @instrument: (in): A #PpgInstrument.
 *
 * Handle the "visualizer-added" event and store a reference to the visualizer.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_instrument_visualizer_added (PpgInstrument *instrument,
                                 PpgVisualizer *visualizer)
{
	PpgInstrumentPrivate *priv;

	g_return_if_fail(PPG_IS_INSTRUMENT(instrument));
	g_return_if_fail(PPG_IS_VISUALIZER(visualizer));

	priv = instrument->priv;

	g_ptr_array_add(priv->visualizers, g_object_ref(visualizer));
}


/**
 * ppg_instrument_get_visualizers:
 * @instrument: (in): A #PpgInstrument.
 *
 * Retrieves a list of #PpgVisualizer<!-- -->'s. The reference count is not
 * increased, so the caller only needs to call g_list_free() on the returned
 * list.
 *
 * Returns: A #GList of visualizers.
 * Side effects: None.
 */
GList*
ppg_instrument_get_visualizers (PpgInstrument *instrument)
{
	PpgInstrumentPrivate *priv;
	GList *list = NULL;
	gint i;

	g_return_val_if_fail(PPG_IS_INSTRUMENT(instrument), NULL);

	priv = instrument->priv;

	for (i = 0; i < priv->visualizers->len; i++) {
		list = g_list_prepend(list, g_ptr_array_index(priv->visualizers, i));
	}
	return list;
}


/**
 * ppg_instrument_finalize:
 * @object: (in): A #PpgInstrument.
 *
 * Finalizer for a #PpgInstrument instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_instrument_finalize (GObject *object)
{
	ENTRY;
	G_OBJECT_CLASS(ppg_instrument_parent_class)->finalize(object);
	EXIT;
}


/**
 * ppg_instrument_dispose:
 * @instrument: (in): A #PpgInstrument.
 *
 * Handle the "dispose" signal on the object. Clean any references.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_instrument_dispose (GObject *object)
{
	PpgInstrumentPrivate *priv = PPG_INSTRUMENT(object)->priv;
	PpgVisualizer *visualizer;
	GPtrArray *visualizers;
	gint i;

	ENTRY;

	if ((visualizers = priv->visualizers)) {
		priv->visualizers = NULL;
		for (i = visualizers->len - 1; i >= 0; i--) {
			visualizer = g_ptr_array_index(visualizers, i);
			g_ptr_array_remove(visualizers, visualizer);
			g_object_unref(visualizer);
		}
		g_ptr_array_free(visualizers, TRUE);
	}

	G_OBJECT_CLASS(ppg_instrument_parent_class)->dispose(object);

	EXIT;
}


/**
 * ppg_instrument_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (out): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Get a given #GObject property.
 */
static void
ppg_instrument_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
	PpgInstrument *instrument = PPG_INSTRUMENT(object);

	switch (prop_id) {
	case PROP_SESSION:
		g_value_set_object(value, ppg_instrument_get_session(instrument));
		break;
	case PROP_TITLE:
		g_value_set_string(value, ppg_instrument_get_title(instrument));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}


/**
 * ppg_instrument_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
static void
ppg_instrument_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
	PpgInstrument *instrument = PPG_INSTRUMENT(object);

	switch (prop_id) {
	case PROP_TITLE:
		ppg_instrument_set_title(instrument, g_value_get_string(value));
		break;
	case PROP_SESSION:
		ppg_instrument_set_session(instrument, g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}


/**
 * ppg_instrument_class_init:
 * @klass: (in): A #PpgInstrumentClass.
 *
 * Initializes the #PpgInstrumentClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_instrument_class_init (PpgInstrumentClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->dispose = ppg_instrument_dispose;
	object_class->finalize = ppg_instrument_finalize;
	object_class->get_property = ppg_instrument_get_property;
	object_class->set_property = ppg_instrument_set_property;
	g_type_class_add_private(object_class, sizeof(PpgInstrumentPrivate));

	klass->visualizer_added = ppg_instrument_visualizer_added;

	g_object_class_install_property(object_class,
	                                PROP_SESSION,
	                                g_param_spec_object("session",
	                                                    "session",
	                                                    "session",
	                                                    PPG_TYPE_SESSION,
	                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property(object_class,
	                                PROP_TITLE,
	                                g_param_spec_string("title",
	                                                    "title",
	                                                    "title",
	                                                    _("Unknown"),
	                                                    G_PARAM_READWRITE));

	signals[VISUALIZER_ADDED] = g_signal_new("visualizer-added",
	                                         PPG_TYPE_INSTRUMENT,
	                                         G_SIGNAL_RUN_FIRST,
	                                         G_STRUCT_OFFSET(PpgInstrumentClass, visualizer_added),
	                                         NULL, NULL,
	                                         g_cclosure_marshal_VOID__OBJECT,
	                                         G_TYPE_NONE,
	                                         1,
	                                         PPG_TYPE_VISUALIZER);
}


/**
 * ppg_instrument_init:
 * @instrument: (in): A #PpgInstrument.
 *
 * Initializes the newly created #PpgInstrument instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_instrument_init (PpgInstrument *instrument)
{
	PpgInstrumentPrivate *priv;

	priv = G_TYPE_INSTANCE_GET_PRIVATE(instrument, PPG_TYPE_INSTRUMENT,
	                                   PpgInstrumentPrivate);
	instrument->priv = priv;

	priv->entries = g_array_new(FALSE, FALSE, sizeof(PpgVisualizerEntry));
	priv->visualizers = g_ptr_array_new();
}

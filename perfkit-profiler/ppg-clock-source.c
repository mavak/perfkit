/* ppg-clock-source.c
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

#include "ppg-clock-source.h"
#include "ppg-log.h"
#include "ppg-util.h"


G_DEFINE_TYPE(PpgClockSource, ppg_clock_source, G_TYPE_INITIALLY_UNOWNED)


struct _PpgClockSourcePrivate
{
	PkConnection *connection; /* Connection to synchronize clocks with */
	GTimeVal start_tv;        /* Remote time when session was started */
	GTimer *timer;            /* Timer for generating times between syncs */
	guint interval;           /* Interval between signals */
	gboolean started;         /* Have we been started */
	gboolean frozen;          /* Are we currently frozen from updates */
	guint handler;            /* GTimeout handler */
};


enum
{
	PROP_0,

	PROP_CONNECTION,
	PROP_FROZEN,
	PROP_INTERVAL,
};


enum
{
	CLOCK_SYNC,
	LAST_SIGNAL
};


/*
 * Globals.
 */
static guint signals[LAST_SIGNAL] = { 0 };


gdouble
ppg_clock_source_get_elapsed (PpgClockSource *source)
{
	PpgClockSourcePrivate *priv;

	g_return_val_if_fail(PPG_IS_CLOCK_SOURCE(source), 0.0);

	priv = source->priv;
	/*
	 * TODO: Handle remote connections.
	 */
	return priv->timer ? g_timer_elapsed(priv->timer, NULL) : 0.0;
}


static gboolean
ppg_clock_source_interval_timeout (gpointer data)
{
	g_signal_emit(data, signals[CLOCK_SYNC], 0);
	return TRUE;
}


static void
ppg_clock_source_set_connection (PpgClockSource *source,
                                 PkConnection   *connection)
{
	PpgClockSourcePrivate *priv;

	g_return_if_fail(PPG_IS_CLOCK_SOURCE(source));
	g_return_if_fail(PK_IS_CONNECTION(connection));
	g_return_if_fail(source->priv->connection == NULL);

	priv = source->priv;

	priv->connection = connection;
	g_object_add_weak_pointer(G_OBJECT(connection),
	                          (gpointer *)&priv->connection);
}


void
ppg_clock_source_set_interval (PpgClockSource *source,
                               guint           interval)
{
	PpgClockSourcePrivate *priv;

	g_return_if_fail(PPG_IS_CLOCK_SOURCE(source));
	g_return_if_fail(interval > 0);

	priv = source->priv;

	priv->interval = interval;
	if (priv->handler) {
		priv->handler = g_timeout_add(priv->interval,
		                              ppg_clock_source_interval_timeout,
		                              source);
	}
}


void
ppg_clock_source_set_frozen (PpgClockSource *source,
                             gboolean        frozen)
{
	PpgClockSourcePrivate *priv;

	g_return_if_fail(PPG_IS_CLOCK_SOURCE(source));

	priv = source->priv;

	if (frozen != priv->frozen) {
		if (frozen) {
			if (priv->handler) {
				ppg_clear_source(&priv->handler);
			}
		} else {
			if (priv->started && !priv->handler) {
				priv->handler = g_timeout_add(priv->interval,
				                              ppg_clock_source_interval_timeout,
				                              source);
			}
		}
	}
}


void
ppg_clock_source_start (PpgClockSource *source,
                        const GTimeVal *tv)
{
	PpgClockSourcePrivate *priv;

	g_return_if_fail(PPG_IS_CLOCK_SOURCE(source));
	g_return_if_fail(tv != NULL);

	priv = source->priv;

	priv->timer = g_timer_new();
	priv->started = TRUE;
	priv->start_tv = *tv;

	if (!priv->frozen) {
		priv->handler = g_timeout_add(priv->interval,
		                              ppg_clock_source_interval_timeout,
		                              source);
	}

	if (!pk_connection_is_local(priv->connection)) {
		/*
		 * TODO: If connection is not local, we need to setup the time
		 *       synchronizer.
		 */
	}
}


void
ppg_clock_source_stop (PpgClockSource *source)
{
	PpgClockSourcePrivate *priv;

	g_return_if_fail(PPG_IS_CLOCK_SOURCE(source));

	priv = source->priv;

	if (priv->timer) {
		g_timer_stop(priv->timer);
	}
	ppg_clear_source(&priv->handler);
}


/**
 * ppg_clock_source_finalize:
 * @object: (in): A #PpgClockSource.
 *
 * Finalizer for a #PpgClockSource instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_clock_source_finalize (GObject *object)
{
	PpgClockSourcePrivate *priv = PPG_CLOCK_SOURCE(object)->priv;

	ENTRY;

	if (priv->timer) {
		g_timer_destroy(priv->timer);
		priv->timer = NULL;
	}

	if (priv->connection) {
		g_object_remove_weak_pointer(G_OBJECT(priv->connection),
		                             (gpointer *)&priv->connection);
		priv->connection = NULL;
	}

	G_OBJECT_CLASS(ppg_clock_source_parent_class)->finalize(object);

	EXIT;
}


/**
 * ppg_clock_source_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (out): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Get a given #GObject property.
 */
static void
ppg_clock_source_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
	PpgClockSource *source = PPG_CLOCK_SOURCE(object);

	switch (prop_id) {
	case PROP_CONNECTION:
		g_value_set_object(value, source->priv->connection);
		break;
	case PROP_FROZEN:
		g_value_set_boolean(value, source->priv->frozen);
		break;
	case PROP_INTERVAL:
		g_value_set_uint(value, source->priv->interval);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}


/**
 * ppg_clock_source_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
static void
ppg_clock_source_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
	PpgClockSource *source = PPG_CLOCK_SOURCE(object);

	switch (prop_id) {
	case PROP_CONNECTION:
		ppg_clock_source_set_connection(source, g_value_get_object(value));
		break;
	case PROP_FROZEN:
		ppg_clock_source_set_frozen(source, g_value_get_boolean(value));
		break;
	case PROP_INTERVAL:
		ppg_clock_source_set_interval(source, g_value_get_uint(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}


/**
 * ppg_clock_source_class_init:
 * @klass: (in): A #PpgClockSourceClass.
 *
 * Initializes the #PpgClockSourceClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_clock_source_class_init (PpgClockSourceClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_clock_source_finalize;
	object_class->get_property = ppg_clock_source_get_property;
	object_class->set_property = ppg_clock_source_set_property;
	g_type_class_add_private(object_class, sizeof(PpgClockSourcePrivate));

	g_object_class_install_property(object_class,
	                                PROP_CONNECTION,
	                                g_param_spec_object("connection",
	                                                    "connection",
	                                                    "connection",
	                                                    PK_TYPE_CONNECTION,
	                                                    G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property(object_class,
	                                PROP_FROZEN,
	                                g_param_spec_boolean("frozen",
	                                                     "frozen",
	                                                     "frozen",
	                                                     FALSE,
	                                                     G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_INTERVAL,
	                                g_param_spec_uint("interval",
	                                                  "interval",
	                                                  "interval",
	                                                  0,
	                                                  G_MAXUINT,
	                                                  133,
	                                                  G_PARAM_READWRITE));

	signals[CLOCK_SYNC] = g_signal_new("clock-sync",
	                                   PPG_TYPE_CLOCK_SOURCE,
	                                   G_SIGNAL_RUN_FIRST,
	                                   0,
	                                   NULL,
	                                   NULL,
	                                   g_cclosure_marshal_VOID__VOID,
	                                   G_TYPE_NONE,
	                                   0);
}


/**
 * ppg_clock_source_init:
 * @source: (in): A #PpgClockSource.
 *
 * Initializes the newly created #PpgClockSource instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_clock_source_init (PpgClockSource *source)
{
	source->priv = G_TYPE_INSTANCE_GET_PRIVATE(source, PPG_TYPE_CLOCK_SOURCE,
	                                           PpgClockSourcePrivate);
	source->priv->interval = 250;
}

/* ppg-instrument-view.c
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

#include "ppg-instrument-view.h"
#include "ppg-log.h"
#include "ppg-util.h"
#include "ppg-visualizer.h"


#define HEADER_WIDTH       200.0
#define DEFAULT_HEIGHT     45.0
#define ZOOM_ADJUST        10.0
#define ROW_SPACING        1.0


G_DEFINE_TYPE(PpgInstrumentView, ppg_instrument_view, GOO_TYPE_CANVAS_TABLE)


struct _PpgInstrumentViewPrivate
{
	PpgInstrument  *instrument;   /* What instrument are we viewing */
	GtkStateType    state;        /* The state of the instrument */
	GtkStyle       *style;        /* Our GtkStyle for rendering */
	GPtrArray      *visualizers;  /* Array of visualizers in view */
	GooCanvasItem  *header_bg;    /* Background gradient for header */
	GooCanvasItem  *header_text;  /* Text for header */
	GooCanvasItem  *bar;          /* Separator bar between header and data */
	GooCanvasItem  *table;        /* Table containing visualizers */
	gint            zoom;         /* What is the current zoom level */
	gboolean        frozen;       /* Updates are frozen to visualizers */
	gboolean        compact;      /* Are we in compact mode (important only) */
};


enum
{
	PROP_0,

	PROP_COMPACT,
	PROP_FROZEN,
	PROP_INSTRUMENT,
	PROP_STATE,
	PROP_STYLE,
	PROP_ZOOM,
};


/**
 * ppg_instrument_view_transform_width:
 * @view: (in): A #PpgInstrumentView.
 *
 * Converts the source value to the target value by subtracting HEADER_WIDTH
 * from the source. This allows g_object_bind_property() to be easily used
 * for tracking properties of the instrument view to child canvas items.
 *
 * Returns: %TRUE always.
 * Side effects: None.
 */
static gboolean
ppg_instrument_view_transform_width (GBinding     *binding,
                                     const GValue *source_value,
                                     GValue       *target_value,
                                     gpointer      user_data)
{
	gdouble src;

	src = g_value_get_double(source_value);
	g_value_set_double(target_value, src - HEADER_WIDTH);
	return TRUE;
}


/**
 * ppg_instrument_view_relayout:
 * @view: (in): A #PpgInstrumentView.
 *
 * Relayout the visualizer items within the instrument view, taking into
 * account zooms and natural-heights as well as our minimum heights.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_instrument_view_relayout (PpgInstrumentView *view)
{
	PpgInstrumentViewPrivate *priv;
	GooCanvasItemVisibility visibility;
	GooCanvasBounds b;
	PpgVisualizer *visualizer;
	gdouble natural_height;
	gdouble height = 0.0;
	gdouble extra;
	gint n_visible = 0;
	gint i;

	g_return_if_fail(PPG_IS_INSTRUMENT_VIEW(view));

	priv = view->priv;

	/*
	 * Determine how much vertical space we think we need.
	 */
	for (i = 0; i < priv->visualizers->len; i++) {
		visualizer = g_ptr_array_index(priv->visualizers, i);
		g_object_get(visualizer, "visibility", &visibility, NULL);
		if (visibility != GOO_CANVAS_ITEM_HIDDEN) {
			n_visible++;
			natural_height = ppg_visualizer_get_natural_height(visualizer);
			natural_height *= priv->zoom;
			height += natural_height + 1.0;
			g_object_set(visualizer, "height", natural_height, NULL);
		}
	}

	if (height == 0.0) {
		/*
		 * No more items to visualize, lets shrink down extra small.
		 */
		goo_canvas_item_get_bounds(priv->header_text, &b);
		natural_height = b.y2 - b.y1;
		g_object_set(view, "height", natural_height + 2.0, NULL);
		g_object_set(priv->header_bg, "height", natural_height + 2.0, NULL);
	} else  if (height < DEFAULT_HEIGHT) {
		/*
		 * We need less than the header column requires, so lets grow to space
		 * them all out evenly.
		 */
		extra = (DEFAULT_HEIGHT - height) / (gdouble)n_visible;
		for (i = 0; i < priv->visualizers->len; i++) {
			visualizer = g_ptr_array_index(priv->visualizers, i);
			natural_height = ppg_visualizer_get_natural_height(visualizer);
			natural_height *= priv->zoom;
			natural_height += extra;
			g_object_set(visualizer, "height", natural_height, NULL);
		}
		g_object_set(view, "height", DEFAULT_HEIGHT, NULL);
	} else {
		g_object_set(view, "height", height, NULL);
	}

	goo_canvas_item_set_child_properties(GOO_CANVAS_ITEM(view), priv->header_text,
	                                     "top-padding", height == 0.0 ? 1.0 : 15.0,
	                                     NULL);

	g_object_notify(G_OBJECT(view), "height");
}


/**
 * ppg_instrument_view_get_instrument:
 * @view: (in): A #PpgInstrumentView.
 *
 * Retrieves the instrument being viewed by this #PpgInstrumentView.
 *
 * Returns: A #PpgInstrument.
 * Side effects: None.
 */
PpgInstrument*
ppg_instrument_view_get_instrument (PpgInstrumentView *view)
{
	g_return_val_if_fail(PPG_IS_INSTRUMENT_VIEW(view), NULL);
	return view->priv->instrument;
}


/**
 * ppg_instrument_view_notify_natural_height:
 * @view: (in): A #PpgInstrumentView.
 * @pspec: (in): A #GParamSpec.
 * @visualizer: (in): The visualizer signaling.
 *
 * Handle "notify::natural-height" from a visualizer and force a relayout
 * of the items in the table.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_instrument_view_notify_natural_height (PpgInstrumentView *view,
                                           GParamSpec        *pspec,
                                           PpgVisualizer     *visualizer)
{
	ppg_instrument_view_relayout(view);
}


/**
 * ppg_instrument_view_visualizer_added:
 * @view: (in): A #PpgInstrumentView.
 *
 * Handle the "visualizer-added" event for the #PpgInstrument. Add the
 * visualizer to our table of visualizers.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_instrument_view_visualizer_added (PpgInstrumentView *view,
                                      PpgVisualizer     *visualizer,
                                      PpgInstrument     *instrument)
{
	PpgInstrumentViewPrivate *priv;
	GooCanvasItem *item = (GooCanvasItem *)visualizer;
	gdouble width;

	g_return_if_fail(PPG_IS_INSTRUMENT_VIEW(view));
	g_return_if_fail(PPG_IS_VISUALIZER(visualizer));
	g_return_if_fail(PPG_IS_INSTRUMENT(instrument));

	priv = view->priv;

	g_object_get(view, "width", &width, NULL);
	width = MAX(200.0, width - HEADER_WIDTH);
	g_object_set(item,
	             "parent", priv->table,
	             "width", width,
	             "visibility", GOO_CANVAS_ITEM_VISIBLE,
	             NULL);
	g_object_bind_property_full(view, "width", item, "width", 0,
	                            ppg_instrument_view_transform_width,
	                            NULL, NULL, NULL);
	goo_canvas_item_set_child_properties(priv->table, item,
	                                     "bottom-padding", ROW_SPACING,
	                                     "column", 0,
	                                     "row", priv->visualizers->len,
	                                     NULL);
	g_ptr_array_add(priv->visualizers, visualizer);
	g_signal_connect_swapped(visualizer, "notify::natural-height",
	                         G_CALLBACK(ppg_instrument_view_notify_natural_height),
	                         view);
	ppg_instrument_view_relayout(view);
}


/**
 * ppg_instrument_view_set_instrument:
 * @view: (in): A #PpgInstrumentView.
 * @instrument: (in): A #PpgInstrument.
 *
 * Set the instrument for the instrument view. This may only be called
 * once.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_instrument_view_set_instrument (PpgInstrumentView *view,
                                    PpgInstrument     *instrument)
{
	PpgInstrumentViewPrivate *priv;
	GList *list;
	GList *iter;

	g_return_if_fail(PPG_IS_INSTRUMENT_VIEW(view));
	g_return_if_fail(PPG_IS_INSTRUMENT(instrument));

	priv = view->priv;

	/*
	 * Sink instrument into our view and setup canvas items.
	 */
	priv->instrument = g_object_ref_sink(instrument);
	g_object_bind_property(instrument, "title", priv->header_text, "text",
	                       G_BINDING_SYNC_CREATE);
	g_signal_connect_swapped(instrument, "visualizer-added",
	                         G_CALLBACK(ppg_instrument_view_visualizer_added),
	                         view);
	list = ppg_instrument_get_visualizers(instrument);
	for (iter = list; iter; iter = iter->next) {
		ppg_instrument_view_visualizer_added(view, iter->data, instrument);
	}
}


/**
 * ppg_instrument_view_set_style:
 * @view: (in): A #PpgInstrumentView.
 * @style: (in): A #GtkStyle.
 *
 * Sets the style for the instrument and redraws associated canvas items.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_instrument_view_set_style (PpgInstrumentView *view,
                               GtkStyle          *style)
{
	PpgInstrumentViewPrivate *priv;
	cairo_pattern_t *p;
	GtkStateType state;
	GdkColor begin;
	GdkColor end;
	gdouble height;
	guint text_color;
	guint dark_color;
	guint bg_color;

	g_return_if_fail(PPG_IS_INSTRUMENT_VIEW(view));
	g_return_if_fail(GTK_IS_STYLE(style));

	priv = view->priv;

	/*
	 * Lose existing weak reference if needed.
	 */
	if (priv->style) {
		g_object_remove_weak_pointer(G_OBJECT(priv->style),
		                             (gpointer *)&priv->style);
	}

	/*
	 * Store a weak reference to the style.
	 */
	priv->style = style;
	g_object_add_weak_pointer(G_OBJECT(priv->style),
	                          (gpointer *)&priv->style);


	state = priv->state;
	begin = style->mid[state];
	end = style->dark[state];

	/*
	 * Redraw header gradient.
	 */
	g_object_get(priv->header_bg, "height", &height, NULL);
	p = cairo_pattern_create_linear(0, 0, 0, height);
	ppg_cairo_add_color_stop(p, 0, &begin);
	ppg_cairo_add_color_stop(p, 1, &end);
	g_object_set(priv->header_bg, "pattern", p, NULL);
	cairo_pattern_destroy(p);

	/*
	 * Update separator bar color.
	 */
	dark_color = ppg_color_to_uint(&style->dark[state], 0xFF);
	g_object_set(priv->bar, "fill-color-rgba", dark_color, NULL);

	/*
	 * Update text font color.
	 */
	text_color = ppg_color_to_uint(&style->text[state], 0xFF);
	g_object_set(priv->header_text, "fill-color-rgba", text_color, NULL);

	/*
	 * Set general background color.
	 */
	bg_color = ppg_color_to_uint(&style->mid[state], 0xFF);
	g_object_set(priv->table, "fill-color-rgba", bg_color, NULL);
}


/**
 * ppg_instrument_view_set_state:
 * @view: (in): A #PpgInstrumentView.
 *
 * Set the state of the instrument view. This is mostly useful for
 * @GTK_STATE_NORMAL and @GTK_STATE_SELECTED.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_instrument_view_set_state (PpgInstrumentView *view,
                               GtkStateType       state)
{
	g_return_if_fail(PPG_IS_INSTRUMENT_VIEW(view));

	view->priv->state = state;
	ppg_instrument_view_set_style(view, view->priv->style);
}


/**
 * ppg_instrument_view_get_state:
 * @view: (in): A #PpgInstrumentView.
 *
 * Get the state of the instrument view.
 *
 * Returns: A #GtkStateType.
 * Side effects: None.
 */
GtkStateType
ppg_instrument_view_get_state (PpgInstrumentView *view)
{
	g_return_val_if_fail(PPG_IS_INSTRUMENT_VIEW(view), 0);
	return view->priv->state;
}


/**
 * ppg_instrument_view_set_time:
 * @view: (in): A #PpgInstrumentView.
 *
 * Sets the time range to be visualized by the visualizers.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_instrument_view_set_time (PpgInstrumentView *view,
                              gdouble            begin_time,
                              gdouble            end_time)
{
	PpgInstrumentViewPrivate *priv;
	PpgVisualizer *visualizer;
	gint i;

	g_return_if_fail(PPG_IS_INSTRUMENT_VIEW(view));

	priv = view->priv;

	for (i = 0; i < priv->visualizers->len; i++) {
		visualizer = g_ptr_array_index(priv->visualizers, i);
		ppg_visualizer_set_time(visualizer, begin_time, end_time);
	}
}


/**
 * ppg_instrument_view_zoom_in:
 * @view: (in): A #PpgInstrumentView.
 *
 * Tells an instrument view to zoom in to the next level. This causes the
 * height of the instrument view to grow.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_instrument_view_zoom_in (PpgInstrumentView *view)
{
	g_return_if_fail(PPG_IS_INSTRUMENT_VIEW(view));

	view->priv->zoom++;
	ppg_instrument_view_relayout(view);
}


/**
 * ppg_instrument_view_zoom_out:
 * @view: (in): A #PpgInstrumentView.
 *
 * Tells an instrument view to zoom out to the next level. This causes the
 * height of the instrument view to shrink.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_instrument_view_zoom_out (PpgInstrumentView *view)
{
	PpgInstrumentViewPrivate *priv;
	gint zoom;

	g_return_if_fail(PPG_IS_INSTRUMENT_VIEW(view));

	priv = view->priv;

	zoom = MAX(1, priv->zoom - 1);
	if (zoom != priv->zoom) {
		priv->zoom = zoom;
		ppg_instrument_view_relayout(view);
	}
}


/**
 * ppg_instrument_view_set_frozen:
 * @view: (in): A #PpgInstrumentView.
 * @frozen: (in): If the visualizer updates should be frozen.
 *
 * Freeze updates to the visualizers.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_instrument_view_set_frozen (PpgInstrumentView *view,
                                gboolean           frozen)
{
	PpgInstrumentViewPrivate *priv;
	PpgVisualizer *visualizer;
	gint i;

	g_return_if_fail(PPG_IS_INSTRUMENT_VIEW(view));

	priv = view->priv;

	priv->frozen = frozen;

	for (i = 0; i < priv->visualizers->len; i++) {
		visualizer = g_ptr_array_index(priv->visualizers, i);
		if (frozen) {
			ppg_visualizer_freeze(visualizer);
		} else {
			ppg_visualizer_thaw(visualizer);
		}
	}

	g_object_notify(G_OBJECT(view), "frozen");
}


static void
ppg_instrument_view_set_compact (PpgInstrumentView *view,
                                 gboolean           compact)
{
	PpgInstrumentViewPrivate *priv;
	PpgVisualizer *visualizer;
	gint i;

	g_return_if_fail(PPG_IS_INSTRUMENT_VIEW(view));

	priv = view->priv;

	priv->compact = compact;
	for (i = 0; i < priv->visualizers->len; i++) {
		visualizer = g_ptr_array_index(priv->visualizers, i);
		if (!ppg_visualizer_get_is_important(visualizer)) {
			g_object_set(visualizer,
			             "visibility", compact ?
			                           GOO_CANVAS_ITEM_HIDDEN:
			                           GOO_CANVAS_ITEM_VISIBLE,
			             NULL);
		}
	}
	ppg_instrument_view_relayout(view);
	g_object_notify(G_OBJECT(view), "compact");
}


/**
 * ppg_instrument_view_dispose:
 * @object: (in): A #GObject.
 *
 * Dispose callback for @object.  This method releases references held
 * by the #GObject instance.
 *
 * Returns: None.
 * Side effects: Plenty.
 */
static void
ppg_instrument_view_dispose (GObject *object)
{
	PpgInstrumentViewPrivate *priv = PPG_INSTRUMENT_VIEW(object)->priv;

	ENTRY;
	ppg_clear_object(&priv->instrument);
	G_OBJECT_CLASS(ppg_instrument_view_parent_class)->dispose(object);
	EXIT;
}


/**
 * ppg_instrument_view_finalize:
 * @object: (in): A #PpgInstrumentView.
 *
 * Finalizer for a #PpgInstrumentView instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_instrument_view_finalize (GObject *object)
{
	PpgInstrumentViewPrivate *priv = PPG_INSTRUMENT_VIEW(object)->priv;

	ENTRY;
	if (priv->style) {
		g_object_remove_weak_pointer(G_OBJECT(priv->style),
		                             (gpointer *)&priv->style);
	}
	g_ptr_array_free(priv->visualizers, TRUE);
	G_OBJECT_CLASS(ppg_instrument_view_parent_class)->finalize(object);
	EXIT;
}


/**
 * ppg_instrument_view_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (out): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Get a given #GObject property.
 */
static void
ppg_instrument_view_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
	PpgInstrumentView *view = PPG_INSTRUMENT_VIEW(object);

	switch (prop_id) {
	case PROP_COMPACT:
		g_value_set_boolean(value, view->priv->compact);
		break;
	case PROP_FROZEN:
		g_value_set_boolean(value, view->priv->frozen);
		break;
	case PROP_INSTRUMENT:
		g_value_set_object(value, view->priv->instrument);
		break;
	case PROP_STATE:
		g_value_set_enum(value, view->priv->state);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}


/**
 * ppg_instrument_view_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
static void
ppg_instrument_view_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
	PpgInstrumentView *view = PPG_INSTRUMENT_VIEW(object);

	switch (prop_id) {
	case PROP_COMPACT:
		ppg_instrument_view_set_compact(view, g_value_get_boolean(value));
		break;
	case PROP_FROZEN:
		ppg_instrument_view_set_frozen(view, g_value_get_boolean(value));
		break;
	case PROP_INSTRUMENT:
		ppg_instrument_view_set_instrument(view, g_value_get_object(value));
		break;
	case PROP_STATE:
		ppg_instrument_view_set_state(view, g_value_get_enum(value));
		break;
	case PROP_STYLE:
		ppg_instrument_view_set_style(view, g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}


/**
 * ppg_instrument_view_class_init:
 * @klass: (in): A #PpgInstrumentViewClass.
 *
 * Initializes the #PpgInstrumentViewClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_instrument_view_class_init (PpgInstrumentViewClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->dispose = ppg_instrument_view_dispose;
	object_class->finalize = ppg_instrument_view_finalize;
	object_class->get_property = ppg_instrument_view_get_property;
	object_class->set_property = ppg_instrument_view_set_property;
	g_type_class_add_private(object_class, sizeof(PpgInstrumentViewPrivate));

	g_object_class_install_property(object_class,
	                                PROP_COMPACT,
	                                g_param_spec_boolean("compact",
	                                                     "compact",
	                                                     "compact",
	                                                     FALSE,
	                                                     G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_FROZEN,
	                                g_param_spec_boolean("frozen",
	                                                     "Frozen",
	                                                     "Freeze visualizers",
	                                                     FALSE,
	                                                     G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_INSTRUMENT,
	                                g_param_spec_object("instrument",
	                                                    "instrument",
	                                                    "instrument",
	                                                    PPG_TYPE_INSTRUMENT,
	                                                    G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_STATE,
	                                g_param_spec_enum("state",
	                                                   "state",
	                                                   "state",
	                                                   GTK_TYPE_STATE_TYPE,
	                                                   GTK_STATE_NORMAL,
	                                                   G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_STYLE,
	                                g_param_spec_object("style",
	                                                    "style",
	                                                    "style",
	                                                    GTK_TYPE_STYLE,
	                                                    G_PARAM_WRITABLE));
}


/**
 * ppg_instrument_view_init:
 * @view: (in): A #PpgInstrumentView.
 *
 * Initializes the newly created #PpgInstrumentView instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_instrument_view_init (PpgInstrumentView *view)
{
	PpgInstrumentViewPrivate *priv;
	GooCanvasItem *parent = (GooCanvasItem *)view;

	priv = G_TYPE_INSTANCE_GET_PRIVATE(view, PPG_TYPE_INSTRUMENT_VIEW,
	                                   PpgInstrumentViewPrivate);
	view->priv = priv;

	g_object_set(view,
	             "height", DEFAULT_HEIGHT,
	             NULL);

	priv->state = GTK_STATE_NORMAL;
	priv->visualizers = g_ptr_array_new();
	priv->zoom = 1;

	priv->header_bg = g_object_new(GOO_TYPE_CANVAS_IMAGE,
	                               "width", HEADER_WIDTH - 1.0,
	                               "height", DEFAULT_HEIGHT,
	                               "parent", parent,
	                               NULL);
	goo_canvas_item_set_child_properties(parent, priv->header_bg,
	                                     "row", 0,
	                                     "column", 0,
	                                     NULL);
	g_object_bind_property(view, "height", priv->header_bg, "height", 0);

	priv->header_text = g_object_new(GOO_TYPE_CANVAS_TEXT,
	                                 "alignment", PANGO_ALIGN_LEFT,
	                                 "font", "Sans 10",
	                                 "parent", parent,
	                                 "text", "XXX",
	                                 NULL);
	goo_canvas_item_set_child_properties(parent, priv->header_text,
	                                     "left-padding", 15.0,
	                                     "column", 0,
	                                     "row", 0,
	                                     "top-padding", 15.0,
	                                     "x-fill", TRUE,
	                                     "y-fill", TRUE,
	                                     NULL);

	priv->bar = g_object_new(GOO_TYPE_CANVAS_RECT,
	                         "fill-color", "#000000",
	                         "height", DEFAULT_HEIGHT,
	                         "line-width", 0.0,
	                         "parent", parent,
	                         "width", 1.0,
	                         NULL);
	goo_canvas_item_set_child_properties(parent, priv->bar,
	                                     "column", 1,
	                                     "row", 0,
	                                     NULL);
	g_object_bind_property(view, "height", priv->bar, "height", 0);

	priv->table = g_object_new(GOO_TYPE_CANVAS_TABLE,
	                           "parent", parent,
	                           "width", 200.0,
	                           NULL);
	goo_canvas_item_set_child_properties(parent, priv->table,
	                                     "row", 0,
	                                     "column", 2,
	                                     "y-expand", TRUE,
	                                     "y-fill", TRUE,
	                                     NULL);
	g_object_bind_property_full(view, "width", priv->table, "width", 0,
	                            ppg_instrument_view_transform_width,
	                            NULL, NULL, NULL);
}

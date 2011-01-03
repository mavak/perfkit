/* ppg-session-view.c
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
#include <goocanvas.h>

#include "icons/pointer.h"
#include "icons/selection.h"

#include "ppg-header.h"
#include "ppg-instrument.h"
#include "ppg-instrument-menu.h"
#include "ppg-instrument-view.h"
#include "ppg-log.h"
#include "ppg-ruler.h"
#include "ppg-session-view.h"
#include "ppg-util.h"


#define ANIMATION_DURATION 500
#define ANIMATION_MODE     PPG_ANIMATION_EASE_IN_OUT_QUAD
#define SHADOW_HEIGHT      10.0
#define ZOOM_MAX           200.0
#define ZOOM_MIN           0.005
#define PIXELS_PER_SECOND  10


G_DEFINE_TYPE(PpgSessionView, ppg_session_view, GTK_TYPE_ALIGNMENT)


struct _PpgSessionViewPrivate
{
	PpgSession *session;         /* Current session */
	GPtrArray *instrument_views; /* Array of PpgInstrumentViews */
	gdouble last_width;          /* Last widget resize width */
	gdouble last_height;         /* Last widget resize height */
	gdouble zoom;                /* Current zoom level; 1.0 is default */
	guint tool;                  /* Active tool from palette */
	gboolean changing_tool;      /* Re-entrancy helper for tool changes */
	struct {
		GooCanvasItem *item;
		gboolean active;
		gdouble begin_time;
		gdouble end_time;
	} selection;
	GtkAdjustment *vadj;         /* Adjustment for visible rows */
	GtkAdjustment *hadj;         /* Adjustment for time */
	GtkAdjustment *zadj;         /* Adjustment for zoom */
	GtkWidget *canvas;           /* Canvas containing instruments */
	GtkWidget *data_container;   /* Container for extended data */
	GtkWidget *palette;          /* VBox of palette items */
	GtkWidget *paned;            /* VPaned for canvas vs extended data */
	GtkWidget *ruler;            /* Ruler containing time info */
	GtkWidget *table;            /* Master table for all widgets */
	GtkWidget *pointer_tool;     /* Button for pointer tool */
	GtkWidget *time_tool;        /* Button for time selection tool */
	GtkWidget *zoom_tool;        /* Button for zoom tool */
	GooCanvasItem *all_content;  /* Table containing rows+shadow */
	GooCanvasItem *background;   /* Colored background of widget */
	GooCanvasItem *header_bg;    /* Background for header column */
	GooCanvasItem *rows_table;   /* Table containing rows only */
	GooCanvasItem *bar;          /* Separator bar between column/data */
	GooCanvasItem *top_shadow;   /* Shadow at top of canvas */
	GooCanvasItem *selected;     /* Currently selected PpgInstrumentView */
	GooCanvasItem *spinner;      /* Spinner widget canvas item */
};


enum
{
	PROP_0,
	PROP_SELECTED_ITEM,
	PROP_SESSION,
	PROP_SHOW_DATA,
	PROP_ZOOM,
};


/**
 * ppg_session_view_move_down:
 * @view: (in): A #PpgSessionView.
 *
 * Moves the currently selected row to the next row in the list. If there is
 * no currently selected row, the first row in the view is selected.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_session_view_move_down (PpgSessionView *view)
{
	PpgSessionViewPrivate *priv;
	PpgInstrumentView *inst_view;
	gint i;

	g_return_if_fail(PPG_IS_SESSION_VIEW(view));

	priv = view->priv;

	/*
	 * If no row is currently selected, select the first row.
	 */
	if (!priv->selected) {
		if (priv->instrument_views->len) {
			inst_view = g_ptr_array_index(priv->instrument_views, 0);
			ppg_session_view_set_selected_item(view, inst_view);
		}
		return;
	}

	/*
	 * Iterate through the list of rows to move to the next row. This should
	 * probably use an index instead of iteration.
	 */
	for (i = 0; i < priv->instrument_views->len - 1; i++) {
		inst_view = g_ptr_array_index(priv->instrument_views, i);
		if (inst_view == (PpgInstrumentView *)priv->selected) {
			inst_view = g_ptr_array_index(priv->instrument_views, i + 1);
			ppg_session_view_set_selected_item(view, inst_view);
			break;
		}
	}
}


void
ppg_session_view_move_forward (PpgSessionView *view)
{
	PpgSessionViewPrivate *priv;
	gdouble lower;
	gdouble page_size;
	gdouble step_increment;
	gdouble upper;
	gdouble value;

	g_return_if_fail(PPG_IS_SESSION_VIEW(view));

	priv = view->priv;

	g_object_get(priv->hadj,
	             "lower", &lower,
	             "page-size", &page_size,
	             "step-increment", &step_increment,
	             "upper", &upper,
	             "value", &value,
	             NULL);

	value += step_increment;

	g_object_set(priv->hadj,
	             "value", CLAMP(value, lower, upper - page_size),
	             NULL);
}


void
ppg_session_view_move_backward (PpgSessionView *view)
{
	PpgSessionViewPrivate *priv;
	gdouble lower;
	gdouble page_size;
	gdouble step_increment;
	gdouble upper;
	gdouble value;

	g_return_if_fail(PPG_IS_SESSION_VIEW(view));

	priv = view->priv;

	g_object_get(priv->hadj,
	             "lower", &lower,
	             "page-size", &page_size,
	             "step-increment", &step_increment,
	             "upper", &upper,
	             "value", &value,
	             NULL);

	value -= step_increment;

	g_object_set(priv->hadj,
	             "value", CLAMP(value, lower, upper - page_size),
	             NULL);
}


/**
 * ppg_session_view_move_up:
 * @view: (in): A #PpgSessionView.
 *
 * Moves the currently selected row to the previous row in the list. If there
 * is no currently selected row, the first row in the view is selected.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_session_view_move_up (PpgSessionView *view)
{
	PpgSessionViewPrivate *priv;
	PpgInstrumentView *inst_view;
	gint i;

	g_return_if_fail(PPG_IS_SESSION_VIEW(view));

	priv = view->priv;

	/*
	 * If no row is currently selected, select the first row.
	 */
	if (!priv->selected) {
		if (priv->instrument_views->len) {
			inst_view = g_ptr_array_index(priv->instrument_views, 0);
			ppg_session_view_set_selected_item(view, inst_view);
		}
		return;
	}

	/*
	 * Iterate through the list of rows (backwards) to move to the previous
	 * row. This should probably use an index instead of iteration.
	 */
	for (i = priv->instrument_views->len - 1; i > 0; i--) {
		inst_view = g_ptr_array_index(priv->instrument_views, i);
		if (inst_view == (PpgInstrumentView *)priv->selected) {
			inst_view = g_ptr_array_index(priv->instrument_views, i - 1);
			ppg_session_view_set_selected_item(view, inst_view);
			break;
		}
	}
}


/**
 * ppg_session_view_get_selected_item:
 * @view: (in): A #PpgSessionView.
 *
 * Retrieves the currently selected #PpgInstrumentView if there is one.
 *
 * Returns: A #PpgInstrumentView or %NULL.
 * Side effects: None.
 */
PpgInstrumentView*
ppg_session_view_get_selected_item (PpgSessionView *view)
{
	g_return_val_if_fail(PPG_IS_SESSION_VIEW(view), NULL);
	return (PpgInstrumentView *)view->priv->selected;
}


/**
 * ppg_session_view_set_selected_item:
 * @view: (in): A #PpgSessionView.
 * @instrument_view: (in): A #PpgInstrumentView.
 *
 * Sets the currently selected instrument within the view. If the instrument
 * is not currently within view, it will be animated into view.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_session_view_set_selected_item (PpgSessionView    *view,
                                    PpgInstrumentView *instrument_view)
{
	PpgSessionViewPrivate *priv;
	GooCanvasBounds bounds;
	GooCanvasBounds visible;
	PpgInstrument *instrument;
	GtkWidget *child;
	gdouble page_size;
	gdouble value;

	g_return_if_fail(PPG_IS_SESSION_VIEW(view));
	g_return_if_fail(!instrument_view ||
	                 PPG_IS_INSTRUMENT_VIEW(instrument_view));

	priv = view->priv;

	/*
	 * Ingore if the view is already selected.
	 */
	if (priv->selected == (GooCanvasItem *)instrument_view) {
		return;
	}

	/*
	 * Unselect the current row if needed.
	 */
	if (priv->selected) {
		g_object_set(priv->selected, "state", GTK_STATE_NORMAL, NULL);
	}

	/*
	 * Configure the new selection.
	 */
	if ((priv->selected = (GooCanvasItem *)instrument_view)) {
		g_object_set(priv->selected, "state", GTK_STATE_SELECTED, NULL);
		goo_canvas_item_get_bounds(priv->selected, &bounds);
		value = gtk_adjustment_get_value(priv->vadj);
		page_size = gtk_adjustment_get_page_size(priv->vadj);
		bounds.y1 += value;
		bounds.y2 += value;
		visible.y1 = gtk_adjustment_get_value(priv->vadj);
		visible.y2 = visible.y1 + page_size;
		if (bounds.y1 < visible.y1) {
			g_object_animate(priv->vadj, ANIMATION_MODE, 250,
			                 "value", bounds.y1,
			                 NULL);
		} else if (bounds.y2 > visible.y2) {
			g_object_animate(priv->vadj, ANIMATION_MODE, 250,
			                 "value", bounds.y2 - page_size,
			                 NULL);
		}

		/*
		 * Update the extended data view if we need to.
		 */
		if (gtk_widget_get_visible(priv->data_container)) {
			if ((child = gtk_bin_get_child(GTK_BIN(priv->data_container)))) {
				gtk_container_remove(GTK_CONTAINER(priv->data_container),
				                     child);
			}
			instrument = ppg_instrument_view_get_instrument(instrument_view);
			if ((child = ppg_instrument_create_data_view(instrument))) {
				gtk_container_add(GTK_CONTAINER(priv->data_container), child);
			}
		}
	}

	g_object_notify(G_OBJECT(view), "selected-item");
}


static gboolean
ppg_session_view_instrument_button_press_event (PpgSessionView    *view,
                                                GooCanvasItem     *target,
                                                GdkEventButton    *button,
                                                PpgInstrumentView *inst_view)
{
	PpgSessionViewPrivate *priv;
	GtkMenu *menu;

	g_return_val_if_fail(PPG_IS_SESSION_VIEW(view), FALSE);
	g_return_val_if_fail(PPG_IS_INSTRUMENT_VIEW(inst_view), FALSE);

	priv = view->priv;

	/*
	 * Show the context menu on right click.
	 */
	if (button->button == 3) {
		if (inst_view != (PpgInstrumentView *)priv->selected) {
			ppg_session_view_set_selected_item(view, inst_view);
		}
		menu = g_object_new(PPG_TYPE_INSTRUMENT_MENU,
		                    "instrument-view", inst_view,
		                    NULL);
		gtk_menu_popup(menu, NULL, NULL, NULL, NULL,
		               button->button, button->time);
		return TRUE;
	}

	/*
	 * Show the data area on double click.
	 */
	if (button->type == GDK_2BUTTON_PRESS) {
		if (priv->selected) {
			ppg_session_view_set_show_data(
					view,
					!ppg_session_view_get_show_data(view));
			return TRUE;
		}
	}

	/*
	 * Handle event for given tool.
	 */
	switch (priv->tool) {
	case PPG_SESSION_VIEW_POINTER:
		ppg_session_view_set_selected_item(view, inst_view);
		return TRUE;
	default:
		break;
	}

	return FALSE;
}


/**
 * ppg_session_view_vadj_value_changed:
 * @view: (in): A #PpgSessionView.
 *
 * Handle the "value-changed" signal for the vertical scrollbar adjustment.
 * The canvas item containing the instruments has it's y coordinate adjusted
 * to reflect the current adjustment value.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_view_vadj_value_changed (PpgSessionView *view,
                                     GtkAdjustment  *vadj)
{
	gdouble value;

	g_return_if_fail(PPG_IS_SESSION_VIEW(view));

	value = gtk_adjustment_get_value(vadj);
	g_object_set(view->priv->all_content, "y", -value, NULL);
}


/**
 * ppg_session_view_update_vadj:
 * @view: (in): A #PpgSessionView.
 *
 * Update the vertical GtkAdjustment based on the sizings of the various
 * canvas items.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_view_update_vadj (PpgSessionView *view)
{
	PpgSessionViewPrivate *priv;
	GooCanvasBounds bounds;
	GtkAllocation alloc;
	GooCanvasItem *root;
	gdouble page_size;
	gdouble upper;

	g_return_if_fail(PPG_IS_SESSION_VIEW(view));

	priv = view->priv;

	/*
	 * Adjust the adjustments "page-size". This is the number of pixels
	 * that are visible on the screen at any give time (veritcally).
	 */
	gtk_widget_get_allocation(priv->canvas, &alloc);
	page_size = alloc.height;
	gtk_adjustment_set_page_size(priv->vadj, page_size);
	gtk_adjustment_set_page_increment(priv->vadj, page_size / 2.0);

	/*
	 * Adjust the upper bounds of the adjustment. This is the total height
	 * of all the rows combined plus a little space.
	 */
	root = goo_canvas_get_root_item(GOO_CANVAS(priv->canvas));
	goo_canvas_item_get_bounds(priv->all_content, &bounds);
	upper = bounds.y2 - bounds.y1 - SHADOW_HEIGHT + (page_size / 2.0);
	gtk_adjustment_set_upper(priv->vadj, upper);

	/*
	 * Clamp the value.
	 */
	if ((bounds.y2 - SHADOW_HEIGHT) < (page_size / 2.0)) {
		gtk_adjustment_set_value(priv->vadj, upper - page_size);
	}
}


/**
 * ppg_session_view_instrument_notify_height:
 * @view: (in): A #PpgSessionView.
 *
 * Handle the "notify::height" signal from the #PpgInstrumentView.
 * Force an update of the height calculation.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_view_instrument_notify_height (PpgSessionView    *view,
                                           GParamSpec        *pspec,
                                           PpgInstrumentView *instrument_view)
{
	g_return_if_fail(PPG_IS_SESSION_VIEW(view));
	ppg_session_view_update_vadj(view);
}


/**
 * ppg_session_view_instrument_added:
 * @view: (in): A #PpgSessionView.
 * @instrument: (in): A #PpgInstrument.
 *
 * Handles the "instrument-added" event on the #PpgSession.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_view_instrument_added (PpgSessionView *view,
                                   PpgInstrument  *instrument)
{
	PpgSessionViewPrivate *priv;
	GtkAllocation a;
	GooCanvasItem *inst_view;
	GtkStyle *style;

	g_return_if_fail(PPG_IS_SESSION_VIEW(view));
	g_return_if_fail(PPG_IS_INSTRUMENT(instrument));

	priv = view->priv;

	/*
	 * Enable the items in the tool palette.
	 */
	g_object_set(priv->pointer_tool, "sensitive", TRUE, NULL);
	g_object_set(priv->time_tool, "sensitive", TRUE, NULL);
	g_object_set(priv->zoom_tool, "sensitive", TRUE, NULL);

	/*
	 * Create a PpgInstrumentView to visualize the instrument and set the
	 * GtkStyle to that of the PpgSessionView.
	 */
	style = gtk_widget_get_style(GTK_WIDGET(view));
	inst_view = g_object_new(PPG_TYPE_INSTRUMENT_VIEW,
	                         "instrument", instrument,
	                         "parent", priv->rows_table,
	                         "style", style,
	                         NULL);
	goo_canvas_item_set_child_properties(GOO_CANVAS_ITEM(priv->rows_table),
	                                     inst_view,
	                                     "row", priv->instrument_views->len,
	                                     "column", 0,
	                                     NULL);
	g_signal_connect_swapped(inst_view, "button-press-event",
	                         G_CALLBACK(ppg_session_view_instrument_button_press_event),
	                         view);
	g_signal_connect_swapped(inst_view, "notify::height",
	                         G_CALLBACK(ppg_session_view_instrument_notify_height),
	                         view);
	g_ptr_array_add(priv->instrument_views, inst_view);

	/*
	 * Let the view know what its width should be.
	 */
	gtk_widget_get_allocation(GTK_WIDGET(priv->canvas), &a);
	g_object_set(inst_view, "width", (gdouble)a.width, NULL);
	g_object_notify(G_OBJECT(inst_view), "width");
}


/**
 * ppg_session_view_create_shadow_pattern:
 * @gravity: (in): The direction to draw the shadow.
 *
 * Creates a #cairo_pattern_t containing a shadow.
 *
 * Returns: A #cairo_pattern_t.
 * Side effects: None.
 */
static cairo_pattern_t *
ppg_session_view_create_shadow_pattern (GdkGravity gravity)
{
	cairo_pattern_t *p;

	g_return_val_if_fail(gravity == GDK_GRAVITY_NORTH ||
	                     gravity == GDK_GRAVITY_SOUTH,
	                     NULL);

	p = cairo_pattern_create_linear(0, 0, 0, SHADOW_HEIGHT);
	switch ((int)gravity) {
	case GDK_GRAVITY_NORTH:
		cairo_pattern_add_color_stop_rgba(p, 0, .3, .3, .3, .4);
		cairo_pattern_add_color_stop_rgba(p, 1, .3, .3, .3, 0);
		break;
	case GDK_GRAVITY_SOUTH:
		cairo_pattern_add_color_stop_rgba(p, 0, .3, .3, .3, 0);
		cairo_pattern_add_color_stop_rgba(p, 1, .3, .3, .3, .4);
		break;
	default:
		g_assert_not_reached();
		return NULL;
	}
	return p;
}


/**
 * ppg_session_view_style_set_timeout:
 * @data: (in): A #PpgSessionView.
 *
 * Update the styling on various components of the #PpgSessionView. This
 * function is designed to be called from a main loop idle callback.
 *
 * Returns: %FALSE always.
 * Side effects: None.
 */
static gboolean
ppg_session_view_style_set_timeout (gpointer data)
{
	PpgSessionView *view = (PpgSessionView *)data;
	GtkWidget *widget = (GtkWidget *)data;
	PpgSessionViewPrivate *priv;
	PpgInstrumentView *inst_view;
	GtkStateType state;
	GtkStyle *style;
	guint bg_color;
	guint mid_color;
	guint dark_color;
	guint select_color;
	gint i;

	g_return_val_if_fail(PPG_IS_SESSION_VIEW(widget), FALSE);

	priv = view->priv;

	style = gtk_widget_get_style(widget);
	state = gtk_widget_get_state(widget);

	/*
	 * Convert GdkColor stylings to 0xRRGGBBAA 32-bit unsigned integers.
	 */
	bg_color = ppg_color_to_uint(&style->bg[state], 0xFF);
	mid_color = ppg_color_to_uint(&style->mid[state], 0xFF);
	dark_color = ppg_color_to_uint(&style->dark[state], 0xFF);
	select_color = ppg_color_to_uint(&style->dark[GTK_STATE_SELECTED], 0x66);

	/*
	 * Update styling of canvas items.
	 */
	g_object_set(priv->background, "fill-color-rgba", bg_color, NULL);
	g_object_set(priv->header_bg, "fill-color-rgba", mid_color, NULL);
	g_object_set(priv->bar, "fill-color-rgba", dark_color, NULL);
	g_object_set(priv->rows_table, "fill-color-rgba", mid_color, NULL);
	g_object_set(priv->selection.item, "fill-color-rgba", select_color, NULL);

	/*
	 * Update styling of all PpgInstrumentViews.
	 */
	for (i = 0; i < priv->instrument_views->len; i++) {
		inst_view = g_ptr_array_index(priv->instrument_views, i);
		ppg_instrument_view_set_style(inst_view, style);
	}

	return FALSE;
}


/**
 * ppg_session_view_style_set:
 * @view: (in): A #PpgSessionView.
 *
 * Handle the "style-set" signal for the GtkWidget. The styling of the various
 * canvas items will be updated from a main-loop idle callback.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_view_style_set (GtkWidget *widget,
                            GtkStyle  *old_style)
{
	GTK_WIDGET_CLASS(ppg_session_view_parent_class)->
		style_set(widget, old_style);

	/*
	 * BUG: Black colored canvas items.
	 *
	 * Setting the style immediately causes some interesting issues with the
	 * canvas. (Basically it just ends up drawing black).  We had this issue
	 * with Clutter as well, but it mostly went away and so we forgot about
	 * actually determining the problem.
	 */
	g_timeout_add(0, ppg_session_view_style_set_timeout, widget);
}


static gboolean
ppg_session_view_layout_spinner_timeout (gpointer data)
{
	PpgSessionViewPrivate *priv;
	PpgSessionView *view = (PpgSessionView *)data;
	GtkAllocation a;
	gdouble height;
	gdouble width;

	g_return_val_if_fail(PPG_IS_SESSION_VIEW(view), FALSE);

	priv = view->priv;

	/*
	 * Make sure the widget hasn't been destroyed.
	 */
	if (gtk_widget_is_drawable(GTK_WIDGET(view))) {
		gtk_widget_get_allocation(priv->canvas, &a);
		width = a.width;
		height = a.height;
		g_object_set(priv->spinner,
		             "x", 200.0 + (width - 200.0) / 2.0,
		             "y", height / 2.0,
		             NULL);
		g_object_unref(view);
	}

	return FALSE;
}


/**
 * ppg_session_view_size_allocate:
 * @view: (in): A #PpgSessionView.
 *
 * Handle the "size-allocate" signal for the GtkWidget. Update the sizing of
 * various canvas elements in addition to properties on various child
 * widgets that are size based in nature (such as the ruler).
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_view_size_allocate (GtkWidget     *widget,
                                GtkAllocation *alloc)
{
	PpgSessionViewPrivate *priv;
	PpgInstrumentView *instrument_view;
	PpgSessionView *view = (PpgSessionView *)widget;
	GtkAllocation canvas_alloc;
	gboolean width_changed;
	gboolean height_changed;
	gdouble height;
	gdouble width;
	gint i;

	g_return_if_fail(PPG_IS_SESSION_VIEW(view));
	g_return_if_fail(alloc != NULL);

	priv = view->priv;

	GTK_WIDGET_CLASS(ppg_session_view_parent_class)->
		size_allocate(widget, alloc);

	/*
	 * Determine if our allocation has actually changed horizontally
	 * or vertically. This helps us avoid overworking various elements
	 * unless we really need to.
	 */
	gtk_widget_get_allocation(priv->canvas, &canvas_alloc);
	width = canvas_alloc.width;
	height = canvas_alloc.height;
	width_changed = (width != priv->last_width);
	height_changed = (height != priv->last_height);

	if (width_changed) {
		priv->last_width = width;

		/*
		 * Update width of canvas items.
		 */
		g_object_set(priv->background, "width", width, NULL);
		g_object_set(priv->all_content, "width", width, NULL);
		g_object_set(priv->rows_table, "width", width, NULL);
		g_object_set(priv->top_shadow, "width", width, NULL);

		/*
		 * XXX: It might be a good idea to freeze the visualizers before
		 *      emitting "value-changed" on the adjustment since we will need
		 *      to also resize the width of the instruments (which also causes
		 *      another draw event to occur).
		 */

		/*
		 * Force an update of components that rely on the zoom value.
		 */
		gtk_adjustment_value_changed(priv->zadj);

		/*
		 * Update the width of the instrument views.
		 */
		for (i = 0; i < priv->instrument_views->len; i++) {
			instrument_view = g_ptr_array_index(priv->instrument_views, i);
			g_object_set(instrument_view, "width", width, NULL);
			g_object_notify(G_OBJECT(instrument_view), "width");
		}
	}

	if (height_changed) {
		priv->last_height = height;

		/*
		 * Update height of canvas items.
		 */
		g_object_set(priv->background, "height", height, NULL);
		g_object_set(priv->header_bg, "height", height, NULL);
		g_object_set(priv->bar, "height", height, NULL);
		g_object_set(priv->selection.item, "height", height, NULL);
		g_object_set(priv->spinner,
		             "y", height / 2.0,
		             NULL);

		/*
		 * Update the vertical scroll adjustment.
		 */
		ppg_session_view_update_vadj(PPG_SESSION_VIEW(widget));
	}

	/*
	 * Relayout the spinner widget if needed.
	 */
	if (!gtk_widget_is_sensitive(GTK_WIDGET(view))) {
		g_timeout_add(0, ppg_session_view_layout_spinner_timeout,
		              g_object_ref(view));
	}
}


/**
 * ppg_session_view_canvas_size_allocate:
 * @view: (in): A #PpgSessionView.
 *
 * Handle the "size-allocate" signal for the canvas and adjust the vadj.
 *
 * Returns: None.
 * Side effects: None.
 */
static gboolean
ppg_session_view_canvas_size_allocate (PpgSessionView *view)
{
	g_return_val_if_fail(PPG_IS_SESSION_VIEW(view), FALSE);
	ppg_session_view_update_vadj(view);
	return FALSE;
}


static void
ppg_session_view_scroll_to (PpgSessionView    *view,
                            PpgInstrumentView *instrument_view)
{
	PpgSessionViewPrivate *priv;
	GooCanvasItem *item = (GooCanvasItem *)instrument_view;
	GooCanvasBounds b;
	gdouble upper;
	gdouble value;

	g_return_if_fail(PPG_IS_SESSION_VIEW(view));
	g_return_if_fail(GOO_IS_CANVAS_ITEM(item));

	priv = view->priv;

	goo_canvas_item_get_bounds(item, &b);
	g_object_get(priv->vadj,
	             "value", &value,
	             "upper", &upper,
	             NULL);
	if (b.y1 != 0.0) {
		value += b.y1;
		g_object_animate(priv->vadj, ANIMATION_MODE, ANIMATION_DURATION,
		                 "value", value,
		                 NULL);
	}
}


/**
 * ppg_session_view_set_show_data:
 * @view: (in): A #PpgSessionView.
 *
 * Set whether or not the extended data section should be visible. The
 * extended data section contains extra information that an instrument
 * may want to make available.
 *
 * Returns: %TRUE if the extended data is visible; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
ppg_session_view_get_show_data (PpgSessionView *view)
{
	g_return_val_if_fail(PPG_IS_SESSION_VIEW(view), FALSE);
	return gtk_widget_get_visible(view->priv->data_container);
}


static void
ppg_session_view_complete_data_hide (gpointer data)
{
	PpgSessionViewPrivate *priv;
	PpgSessionView *view = (PpgSessionView *)data;
	GtkWidget *child;

	g_return_if_fail(PPG_IS_SESSION_VIEW(view));

	priv = view->priv;

	if ((child = gtk_bin_get_child(GTK_BIN(priv->data_container)))) {
		gtk_container_remove(GTK_CONTAINER(priv->data_container), child);
	}
	gtk_widget_hide(priv->data_container);
	g_object_notify(G_OBJECT(view), "show-data");
}


/**
 * ppg_session_view_set_show_data:
 * @view: (in): A #PpgSessionView.
 *
 * Sets whether or not the extended data section should be visible. See
 * ppg_session_view_get_show_data() for more information.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_session_view_set_show_data (PpgSessionView *view,
                                gboolean        show_data)
{
	PpgSessionViewPrivate *priv;
	PpgInstrumentView *selected;
	PpgInstrument *instrument;
	GtkAllocation a;
	GtkWidget *child;

	g_return_if_fail(PPG_IS_SESSION_VIEW(view));

	priv = view->priv;

	/*
	 * Get the allocation of the paned so that we can animation the position
	 * to either half of its height in the case of showing, or its height
	 * in the case of hiding.
	 */
	gtk_widget_get_allocation(priv->paned, &a);
	if (show_data) {
		if (!gtk_widget_get_visible(priv->data_container)) {
			g_object_set(priv->paned, "position", a.height, NULL);
			if ((selected = (PpgInstrumentView *)priv->selected)) {
				instrument = ppg_instrument_view_get_instrument(selected);
				if ((child = ppg_instrument_create_data_view(instrument))) {
					gtk_container_add(GTK_CONTAINER(priv->data_container),
					                  child);
				}
				ppg_session_view_scroll_to(view, selected);
			}
			gtk_widget_show(priv->data_container);
			g_object_animate(priv->paned, ANIMATION_MODE, ANIMATION_DURATION,
			                 "position", (a.height / 2),
			                 NULL);
		}
		g_object_notify(G_OBJECT(view), "show-data");
	} else {
		g_object_animate_full(priv->paned, ANIMATION_MODE,
		                      ANIMATION_DURATION, 0,
		                      ppg_session_view_complete_data_hide,
		                      view,
		                      "position", a.height,
		                      NULL);
	}
}


/**
 * ppg_session_view_get_time_at_coords:
 * @view: (in): A #PpgSessionView.
 *
 * Retrieves the time within the session that position "x" represents
 * using the canvases coordinate space.
 *
 * Returns: A double containing the time.
 * Side effects: None.
 */
static gdouble
ppg_session_view_get_time_at_coords (PpgSessionView *view,
                                     gdouble         x)
{
	PpgSessionViewPrivate *priv;
	GtkAllocation alloc;
	gdouble lower;
	gdouble upper;
	gdouble offset;

	g_return_val_if_fail(PPG_IS_SESSION_VIEW(view), 0.0);

	priv = view->priv;

	ppg_ruler_get_range(PPG_RULER(priv->ruler), &lower, &upper, NULL);
	gtk_widget_get_allocation(priv->canvas, &alloc);
	offset = (MAX(x, 200.0) - 200.0) / (alloc.width - 200.0);
	return lower + ((upper - lower) * offset);
}


/**
 * ppg_session_view_get_coords_at_time:
 * @view: (in): A #PpgSessionView.
 *
 * Retrieves the X coordinate in canvas space for a given time within the
 * session.
 *
 * Returns: A gdouble containing the X coordinate.
 * Side effects: None.
 */
static gdouble
ppg_session_view_get_coords_at_time (PpgSessionView *view,
                                     gdouble         time_)
{
	PpgSessionViewPrivate *priv;
	GtkAllocation alloc;
	gdouble lower;
	gdouble offset;
	gdouble upper;

	g_return_val_if_fail(PPG_IS_SESSION_VIEW(view), 0.0);

	priv = view->priv;

	ppg_ruler_get_range(PPG_RULER(priv->ruler), &lower, &upper, NULL);
	gtk_widget_get_allocation(priv->canvas, &alloc);
	offset = CLAMP(time_, lower, upper) / (upper - lower);
	return 200.0 + ((alloc.width - 200.0) * offset);
}


/**
 * ppg_session_view_begin_selection:
 * @view: (in): A #PpgSessionView.
 * @time: (in): The begin time of the selection.
 * @grab: (in): If we should do a gtk_widget_grab().
 *
 * Begin a selection operation. A selection canvas highlight item is added
 * to the canvas beginning from the specified time. If @grab is true, the
 * canvas item will have a gtk_widget_grab() added so that all events are
 * delivered to the canvas until the selection completes via
 * ppg_session_view_end_selection().
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_view_begin_selection (PpgSessionView *view,
                                  gdouble         time_,
                                  gboolean        grab)
{
	PpgSessionViewPrivate *priv;
	gdouble x;

	g_return_if_fail(PPG_IS_SESSION_VIEW(view));

	priv = view->priv;

	priv->selection.active = TRUE;
	priv->selection.begin_time = time_;
	priv->selection.end_time = time_;
	x = ppg_session_view_get_coords_at_time(view, time_);
	g_object_set(priv->selection.item,
	             "visibility", GOO_CANVAS_ITEM_VISIBLE,
	             "width", 0.0,
	             "x", x,
	             NULL);

	/*
	 * Grabbing the widget prevents Gtk from delivering events to other
	 * widgets until the grab finishes. This means that if the pointer moves
	 * outside the window, we still get those motion notify events.
	 */
	if (grab) {
		gtk_grab_add(priv->canvas);
	}
}


/**
 * ppg_session_view_end_selection:
 * @view: (in): A #PpgSessionView.
 * @ungrab: (in): If we should ungrab the widget.
 *
 * Ends a selection operation. If @ungrab is set, and it should match
 * corresponding to ppg_session_view_begin_selection(), then the canvas will
 * be ungrabbed allowing events to flow to other widgets.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_view_end_selection (PpgSessionView *view,
                                gboolean        ungrab)
{
	PpgSessionViewPrivate *priv;
	gdouble tmp;

	g_return_if_fail(PPG_IS_SESSION_VIEW(view));

	priv = view->priv;

	/*
	 * Update selection range and make sure it goes from low to high.
	 */
	if (priv->selection.end_time < priv->selection.begin_time) {
		tmp = priv->selection.begin_time;
		priv->selection.begin_time = priv->selection.end_time;
		priv->selection.end_time = tmp;
	} else if (priv->selection.begin_time == priv->selection.end_time) {
		priv->selection.begin_time = 0.0;
		priv->selection.end_time = 0.0;
	}
	priv->selection.active = FALSE;

	/*
	 * Ungrab the widget and allow Gtk events to flow to other widgets again.
	 */
	if (ungrab) {
		gtk_grab_remove(priv->canvas);
	}
}


/**
 * ppg_session_view_extend_selection:
 * @view: (in): A #PpgSessionView.
 *
 * Extends the selection to the given time. This should be called after
 * a call to ppg_session_view_begin_selection() has been made. It is safe
 * to call this multiple times though. Once the selection covers the right
 * amount of time, ppg_session_view_end_selection() should be called.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_view_extend_selection (PpgSessionView *view,
                                   gdouble         time_)
{
	PpgSessionViewPrivate *priv;
	gdouble begin;
	gdouble end;
	gdouble x1;
	gdouble x2;

	g_return_if_fail(PPG_IS_SESSION_VIEW(view));

	priv = view->priv;

	priv->selection.end_time = time_;
	if (priv->selection.begin_time < priv->selection.end_time) {
		begin = priv->selection.begin_time;
		end = priv->selection.end_time;
	} else {
		begin = priv->selection.end_time;
		end = priv->selection.begin_time;
	}

	x1 = ppg_session_view_get_coords_at_time(view, begin);
	x2 = ppg_session_view_get_coords_at_time(view, end);

	g_object_set(priv->selection.item,
	             "width", x2 - x1,
	             "x", x1,
	             NULL);
}


/**
 * ppg_session_view_get_selection:
 * @view: (in): A #PpgSessionView.
 * @begin_time: (out) (allow-none): A location for a #gdouble or %NULL.
 * @end_time: (out) (allow-none): A location for a #gdouble or %NULL.
 *
 * Retrieves the currently selected time range in the session.
 *
 * Returns: %TRUE if there is a selection; otherwise %FALSE.
 * Side effects: None.
 */
gboolean
ppg_session_view_get_selection (PpgSessionView *view,
                                gdouble        *begin_time,
                                gdouble        *end_time)
{
	PpgSessionViewPrivate *priv;

	g_return_val_if_fail(PPG_IS_SESSION_VIEW(view), FALSE);
	g_return_val_if_fail(begin_time != NULL, FALSE);
	g_return_val_if_fail(end_time != NULL, FALSE);

	priv = view->priv;

	if (*begin_time) {
		*begin_time = priv->selection.begin_time;
	}

	if (end_time) {
		*end_time = priv->selection.end_time;
	}

	return (priv->selection.begin_time != 0.0 ||
	        priv->selection.end_time != 0.0);
}


/**
 * ppg_session_view_set_selection:
 * @view: (in): A #PpgSessionView.
 * @begin_time: (in): The beginning of the time range.
 * @end_time: (in): The end of the time range.
 *
 * Set the selected time range for the session. A value of 0.0 for both
 * @begin_time and @end_time is equivalent to clearing the selection.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_session_view_set_selection (PpgSessionView *view,
                                gdouble         begin_time,
                                gdouble         end_time)
{
	g_return_if_fail(PPG_IS_SESSION_VIEW(view));

	ppg_session_view_begin_selection(view, begin_time, FALSE);
	ppg_session_view_extend_selection(view, end_time);
	ppg_session_view_end_selection(view, FALSE);
}


/**
 * ppg_session_view_button_press_event:
 * @view: (in): A #PpgSessionView.
 * @target_item: (in): The target #GooCanvasItem.
 * @button: (in): A #GdkEventButton.
 * @root: (in): A #GooCanvasItem.
 *
 * Handles a "button-press-event" on the root canvas item. Based on the current
 * tool, various operations may be performed.
 *
 * Returns: None.
 * Side effects: None.
 */
static gboolean
ppg_session_view_button_press_event (PpgSessionView *view,
                                     GooCanvasItem  *target_item,
                                     GdkEventButton *button,
                                     GooCanvasItem  *root)
{
	PpgSessionViewPrivate *priv;
	PpgInstrumentView *inst;
	gboolean zoom_out;
	gboolean zoom_inst;
	gdouble time_;

	g_return_val_if_fail(PPG_IS_SESSION_VIEW(view), FALSE);
	g_return_val_if_fail(root != NULL, FALSE);

	priv = view->priv;

	if (!gtk_widget_has_focus(priv->canvas)) {
		gtk_widget_grab_focus(priv->canvas);
	}

	switch (priv->tool) {
	case PPG_SESSION_VIEW_TIME_SELECTION:
		time_ = ppg_session_view_get_time_at_coords(view, button->x_root);
		ppg_session_view_begin_selection(view, time_, TRUE);
		break;
	case PPG_SESSION_VIEW_ZOOM:
		zoom_out = (button->state & GDK_CONTROL_MASK) != 0;
		zoom_inst = (button->state & GDK_SHIFT_MASK) != 0;
		if (zoom_inst) {
			inst = ppg_session_view_get_selected_item(view);
			if (inst) {
				if (zoom_out) {
					ppg_instrument_view_zoom_out(inst);
				} else {
					ppg_instrument_view_zoom_in(inst);
				}
				ppg_session_view_set_selected_item(view, inst);
			}
		} else {
			if (zoom_out) {
				ppg_session_view_zoom_out(view);
			} else {
				ppg_session_view_zoom_in(view);
			}
		}
		break;
	default:
		break;
	}

	return FALSE;
}


/**
 * ppg_session_view_button_release_event:
 * @view: (in): A #PpgSessionView.
 * @target_item: (in): The target #GooCanvasItem.
 * @button: (in): A #GdkEventButton.
 * @root: (in): A #GooCanvasItem.
 *
 * Handles the "button-release-event" on the root canvas item. Based on the
 * current tool, various actions are performed.
 *
 * Returns: %TRUE if the event was handled; otherwise %FALSE.
 * Side effects: None.
 */
static gboolean
ppg_session_view_button_release_event (PpgSessionView *view,
                                       GooCanvasItem  *target_item,
                                       GdkEventButton *button,
                                       GooCanvasItem  *root)
{
	PpgSessionViewPrivate *priv;

	g_return_val_if_fail(root != NULL, FALSE);
	g_return_val_if_fail(PPG_IS_SESSION_VIEW(view), FALSE);

	priv = view->priv;

	switch (priv->tool) {
	case PPG_SESSION_VIEW_TIME_SELECTION:
		if (priv->selection.active) {
			ppg_session_view_end_selection(view, TRUE);
			gtk_widget_set_tooltip_text(priv->canvas, NULL);
		}
		return TRUE;
	default:
		break;
	}

	return FALSE;
}


/**
 * ppg_session_view_motion_notify_event:
 * @view: (in): A #PpgSessionView.
 * @target_item: (in): The target #GooCanvasItem.
 * @motion: (in): A #GdkEventMotion.
 * @root: (in): A #GooCanvasItem.
 *
 * Handles the "motion-notify-event" for the root canvas item. Various
 * actions are performed based on the current tool.
 *
 * Returns: %TRUE if the event was handled; otherwise %FALSE.
 * Side effects: None.
 */
static gboolean
ppg_session_view_motion_notify_event (PpgSessionView *view,
                                      GooCanvasItem  *target_item,
                                      GdkEventMotion *motion,
                                      GooCanvasItem  *root)
{
	PpgSessionViewPrivate *priv;
	gdouble time_;

	g_return_val_if_fail(PPG_IS_SESSION_VIEW(view), FALSE);

	priv = view->priv;

	/*
	 * Update the arrow position in the PpgRuler widget.
	 */
	time_ = ppg_session_view_get_time_at_coords(view, motion->x_root);
	ppg_ruler_set_position(PPG_RULER(view->priv->ruler), time_);

	switch (priv->tool) {
	case PPG_SESSION_VIEW_TIME_SELECTION:
		if (priv->selection.active) {
			ppg_session_view_extend_selection(view, time_);
		}
		break;
	default:
		break;
	}

	return FALSE;
}


/**
 * ppg_session_view_scroll_event:
 * @view: (in): A #PpgSessionView.
 * @target_item: (in): The target #GooCanvasItem.
 * @scroll: (in): A #GdkEventScroll.
 * @root: (in): A #GooCanvasItem.
 *
 * Handle a "scroll-event" for the canvas root item. The vertical adjustment
 * is updated to a new value based on the scroll direction.
 *
 * Returns: TRUE if the event was handled; otherwise %FALSE.
 * Side effects: None.
 */
static gboolean
ppg_session_view_scroll_event (PpgSessionView *view,
                               GooCanvasItem  *target_item,
                               GdkEventScroll *scroll,
                               GooCanvasItem  *item)
{
	PpgSessionViewPrivate *priv;
	gdouble increment;
	gdouble lower;
	gdouble page_size;
	gdouble upper;
	gdouble value;

	g_return_val_if_fail(GOO_IS_CANVAS_ITEM(item), FALSE);
	g_return_val_if_fail(scroll != NULL, FALSE);
	g_return_val_if_fail(PPG_IS_SESSION_VIEW(view), FALSE);

	priv = view->priv;

	g_object_get(priv->vadj,
	             "lower", &lower,
	             "page-size", &page_size,
	             "upper", &upper,
	             "value", &value,
	             NULL);
	increment = page_size / 5.0;

	switch (scroll->direction) {
	case GDK_SCROLL_UP:
		value = CLAMP(value - increment, lower, upper);
		gtk_adjustment_set_value(priv->vadj, value);
		return TRUE;
	case GDK_SCROLL_DOWN:
		value = CLAMP(value + increment, lower, upper - page_size);
		gtk_adjustment_set_value(priv->vadj, value);
		return TRUE;
	case GDK_SCROLL_LEFT:
	case GDK_SCROLL_RIGHT:
	default:
		break;
	}

	return FALSE;
}


/**
 * ppg_session_view_get_zoom:
 * @view: (in): A #PpgSessionView.
 *
 * Retrieves the current zoom level for the session. A value of 1.0 means no
 * zooming is occuring. A value of 2.0 means a two-times (2x) zoom. A value
 * of 0.5 would indicate a 50% zoom.
 *
 * Returns: A gdouble containing the current zoom.
 * Side effects: None.
 */
gdouble
ppg_session_view_get_zoom (PpgSessionView *view)
{
	g_return_val_if_fail(PPG_IS_SESSION_VIEW(view), 0.0);
	return view->priv->zoom;
}


/**
 * ppg_session_view_hadj_value_changed:
 * @view: (in): A #PpgSessionView.
 * @hadj: (in): A #GtkAdjustment for the horizontal scrollbar.
 *
 * Handle a change in the horizontal adjustment. This will update the visible
 * range of the instruments and the ruler widget.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_view_hadj_value_changed (PpgSessionView *view,
                                     GtkAdjustment  *hadj)
{
	PpgSessionViewPrivate *priv;
	PpgInstrumentView *inst_view;
	gdouble start_;
	gdouble begin;
	gdouble end;
	gdouble page_size;
	gdouble pos;
	gint i;

	g_return_if_fail(PPG_IS_SESSION_VIEW(view));

	priv = view->priv;

	/*
	 * Get the values of the horizontal adjustment.
	 */
	begin = gtk_adjustment_get_value(hadj);
	page_size = gtk_adjustment_get_page_size(hadj);
	end = begin + page_size;

	/*
	 * Update the ruler position.
	 */
	pos = ppg_ruler_get_position(PPG_RULER(priv->ruler));
	ppg_ruler_set_range(PPG_RULER(priv->ruler), begin, end,
	                    CLAMP(pos, begin, end));

	/*
	 * Update the PpgInstrumentViews to show the given time range.
	 */
	start_ = ppg_session_get_started_at(priv->session);
	for (i = 0; i < priv->instrument_views->len; i++) {
		inst_view = g_ptr_array_index(priv->instrument_views, i);
		ppg_instrument_view_set_time(inst_view,
		                             start_ + begin,
		                             start_ + end);
	}

	/*
	 * TODO: Update the selection if needed.
	 */
}


/**
 * ppg_session_view_zadj_to_zoom:
 * @view: (in): A #PpgSessionView.
 *
 * Converts the current value for the zoom adjustment to a double representing
 * the zoom. They are separate coordinates as GtkAdjustment doesn't provide
 * features such as quadratic scales.
 *
 * Returns: A #gdouble containing the zoom.
 * Side effects: None.
 */
static gdouble
ppg_session_view_zadj_to_zoom (PpgSessionView *view)
{
	PpgSessionViewPrivate *priv;
	gdouble zadj;

	g_return_val_if_fail(PPG_IS_SESSION_VIEW(view), 1.0);

	priv = view->priv;

	zadj = gtk_adjustment_get_value(priv->zadj);
	if (zadj > 1.0) {
		zadj -= 1.0;
		zadj = 1.0 + (zadj * zadj) * (ZOOM_MAX - 1.0);
	} else if (zadj < 1.0) {
		zadj = (zadj * zadj);
	}

	return CLAMP(zadj, ZOOM_MIN, ZOOM_MAX);
}


/**
 * ppg_session_view_zoom_in:
 * @view: (in): A #PpgSessionView.
 *
 * Zooms in the the view to the next level using the zoom adjustments
 * "step-increment" as the level of increment.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_session_view_zoom_in (PpgSessionView *view)
{
	PpgSessionViewPrivate *priv;
	gdouble step_increment;
	gdouble value;

	g_return_if_fail(PPG_IS_SESSION_VIEW(view));

	priv = view->priv;

	g_object_get(priv->zadj,
	              "value", &value,
	              "step-increment", &step_increment,
	              NULL);
	g_object_set(priv->zadj,
	             "value", value + step_increment,
	             NULL);
}


/**
 * ppg_session_view_zoom_out:
 * @view: (in): A #PpgSessionView.
 *
 * Zooms out the the view to the next level using the zoom adjustments
 * "step-increment" as the level of decrement.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_session_view_zoom_out (PpgSessionView *view)
{
	PpgSessionViewPrivate *priv;
	gdouble step_increment;
	gdouble value;

	g_return_if_fail(PPG_IS_SESSION_VIEW(view));

	priv = view->priv;

	g_object_get(priv->zadj,
	              "value", &value,
	              "step-increment", &step_increment,
	              NULL);
	g_object_set(priv->zadj,
	             "value", value - step_increment,
	             NULL);
}


/**
 * ppg_session_view_zoom_normal:
 * @view: (in): A #PpgSessionView.
 *
 * Resets the instruments to a zoom of 1.0.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_session_view_zoom_normal (PpgSessionView *view)
{
	g_return_if_fail(PPG_IS_SESSION_VIEW(view));
	g_object_set(view->priv->zadj, "value", 1.0, NULL);
}


/**
 * ppg_session_view_zadj_value_changed:
 * @view: (in): A #PpgSessionView.
 *
 * Handles a "value-changed" signal for the zoom adjustment. The ruler
 * and instrument views are updated accordingly.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_view_zadj_value_changed (PpgSessionView *view,
                                     GtkAdjustment  *zadj)
{
	PpgSessionViewPrivate *priv;
	PpgInstrumentView *inst_view;
	GtkAllocation a;
	gdouble lower;
	gdouble upper;
	gint i;

	g_return_if_fail(PPG_IS_SESSION_VIEW(view));

	priv = view->priv;

	/*
	 * Recalculate the zoom level.
	 */
	priv->zoom = ppg_session_view_zadj_to_zoom(view);

	gtk_widget_get_allocation(priv->canvas, &a);
	if (a.width > 200) {
		/*
		 * Update the rulers visible time range.
		 */
		ppg_ruler_get_range(PPG_RULER(priv->ruler), &lower, NULL, NULL);
		upper = lower + (a.width - 200.0) / (priv->zoom * PIXELS_PER_SECOND);
		ppg_ruler_set_range(PPG_RULER(priv->ruler), lower, upper, lower);

		/*
		 * Update the page size for the horizontal scroller.
		 */
		gtk_adjustment_set_page_size(priv->hadj, upper - lower);

		/*
		 * Update each of the instrument views to show the proper time.
		 */
		for (i = 0; i < priv->instrument_views->len; i++) {
			inst_view = g_ptr_array_index(priv->instrument_views, i);
			ppg_instrument_view_set_time(inst_view, lower, upper);
		}

		/*
		 * Update a current selection if needed.
		 */
		if (priv->selection.begin_time != 0.0 &&
		    priv->selection.end_time != 0.0) {
			ppg_session_view_set_selection(view, priv->selection.begin_time,
			                               priv->selection.end_time);
		}
	}

	g_object_notify(G_OBJECT(view), "zoom");
}


/**
 * ppg_session_view_set_tool:
 * @view: (in): A #PpgSessionView.
 * @tool: (in): The tool to enable.
 *
 * Set the active tool for the session view. The cursor may be changed based
 * on the given tool.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_session_view_set_tool (PpgSessionView     *view,
                           PpgSessionViewTool  tool)
{
	PpgSessionViewPrivate *priv;
	GdkCursor *cursor;
	GdkWindow *window;
	GdkPixbuf *pixbuf;
	GtkWidget *button;

	g_return_if_fail(PPG_IS_SESSION_VIEW(view));
	g_return_if_fail(tool <= PPG_SESSION_VIEW_ZOOM);

	priv = view->priv;

	if (priv->changing_tool) {
		return;
	}

	priv->changing_tool = TRUE;
	priv->selection.active = FALSE;

	switch ((priv->tool = tool)) {
	case PPG_SESSION_VIEW_POINTER:
		cursor = gdk_cursor_new(GDK_LEFT_PTR);
		button = priv->pointer_tool;
		break;
	case PPG_SESSION_VIEW_TIME_SELECTION:
		cursor = gdk_cursor_new(GDK_XTERM);
		button = priv->time_tool;
		break;
	case PPG_SESSION_VIEW_ZOOM:
		pixbuf = gtk_widget_render_icon(GTK_WIDGET(view), GTK_STOCK_FIND,
		                                GTK_ICON_SIZE_MENU, "");
		cursor = gdk_cursor_new_from_pixbuf(gdk_display_get_default(),
		                                    pixbuf, 0, 0);
		g_object_unref(pixbuf);
		button = priv->zoom_tool;
		break;
	default:
		g_assert_not_reached();
		return;
	}

	/*
	 * Update the cursor for the canvas window.
	 */
	window = gtk_widget_get_window(priv->canvas);
	gdk_window_set_cursor(window, cursor);
	gdk_cursor_unref(cursor);

	/*
	 * Update tool buttons.
	 */
	g_object_set(button, "active", TRUE, NULL);
	priv->changing_tool = FALSE;
}


/**
 * ppg_session_view_pointer_clicked:
 * @view: (in): A #PpgSessionView.
 *
 * Handle the "clicked" signal for the Pointer tool palette button. Sets
 * the current tool to the pointer.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_view_pointer_clicked (PpgSessionView *view,
                                  GtkWidget      *widget)
{
	ppg_session_view_set_tool(view, PPG_SESSION_VIEW_POINTER);
}


/**
 * ppg_session_view_selection_clicked:
 * @view: (in): A #PpgSessionView.
 *
 * Handle the "clicked" signal for the Selection tool palette button. Sets
 * the current tool to selection.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_view_selection_clicked (PpgSessionView *view,
                                    GtkWidget      *widget)
{
	ppg_session_view_set_tool(view, PPG_SESSION_VIEW_TIME_SELECTION);
}


/**
 * ppg_session_view_zoom_clicked:
 * @view: (in): A #PpgSessionView.
 *
 * Handles the "clicked" signal for the zoom tool palette button. Sets the
 * current tool to zoom.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_view_zoom_clicked (PpgSessionView *view,
                               GtkWidget      *widget)
{
	ppg_session_view_set_tool(view, PPG_SESSION_VIEW_ZOOM);
}


/**
 * ppg_session_view_notify_elapsed:
 * @view: (in): A #PpgSessionView.
 *
 * Handle the "notify::elapsed" signal for the session. Update the visible
 * range if needed.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_view_notify_elapsed (PpgSessionView *view,
                                 GParamSpec     *pspec,
                                 PpgSession     *session)
{
	PpgSessionViewPrivate *priv;
	gdouble elapsed;

	priv = view->priv;

	g_object_get(session, "elapsed", &elapsed, NULL);
	gtk_adjustment_set_upper(priv->hadj, elapsed);
	/*
	 * TODO: Update the visible region if necessary.
	 */
	gtk_adjustment_value_changed(priv->hadj);
}


/**
 * ppg_session_view_set_session:
 * @view: (in): A #PpgSessionView.
 *
 * Sets the session for the #PpgSessionView. This may only be called once
 * during object construction.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_view_set_session (PpgSessionView *view,
                              PpgSession     *session)
{
	PpgSessionViewPrivate *priv;

	g_return_if_fail(PPG_IS_SESSION_VIEW(view));
	g_return_if_fail(PPG_IS_SESSION(session));
	g_return_if_fail(view->priv->session == NULL);

	priv = view->priv;

	priv->session = g_object_ref_sink(session);
	g_signal_connect_swapped(priv->session, "notify::elapsed",
	                         G_CALLBACK(ppg_session_view_notify_elapsed),
	                         view);
	g_signal_connect_swapped(priv->session, "instrument-added",
	                         G_CALLBACK(ppg_session_view_instrument_added),
	                         view);
}


/**
 * ppg_session_view_palette_draw:
 * @widget: (in): A #GtkWidget.
 * @cr: (in): A #cairo_t.
 *
 * Handles drawing the background of the tool palette. This is a slight
 * horizontal gradient that blends well into the data view.
 *
 * Returns: %FALSE always.
 * Side effects: None.
 */
static gboolean
ppg_session_view_palette_draw (GtkWidget *palette,
                               cairo_t   *cr)
{
	cairo_pattern_t *p;
	GtkStateType state;
	GtkAllocation a;
	GtkStyle *style;

	g_return_val_if_fail(GTK_IS_WIDGET(palette), FALSE);
	g_return_val_if_fail(cr != NULL, FALSE);

	state = GTK_STATE_NORMAL;
	style = gtk_widget_get_style(palette);
	gtk_widget_get_allocation(palette, &a);

	cairo_rectangle(cr, 0, 0, a.width, a.height);
	p = cairo_pattern_create_linear(0, 0, a.width, 0);
	ppg_cairo_add_color_stop(p, 0, &style->dark[state]);
	ppg_cairo_add_color_stop(p, 1, &style->mid[state]);
	cairo_set_source(cr, p);
	cairo_fill(cr);
	cairo_set_line_width(cr, 1.0);
	cairo_move_to(cr, a.width - .5, 0);
	cairo_line_to(cr, a.width - .5, a.height);
	gdk_cairo_set_source_color(cr, &style->dark[state]);
	cairo_stroke(cr);
	cairo_pattern_destroy(p);

	return FALSE;
}


/**
 * ppg_session_view_palette_expose_event:
 * @widget: (in): A #GtkWidget.
 * @expose: (in): A #GdkEventExpose.
 *
 * Handles the "expose-event" signal for the #GtkWidget when Gtk+ 2.X is used.
 * This simply creates a new cairo context and calls the Gtk+ 3.X version
 * of exposing, ppg_session_view_palette_draw().
 *
 * Returns: %TRUE if signal emission should stop.
 * Side effects: None.
 */
#if !GTK_CHECK_VERSION(2, 91, 0)
static gboolean
ppg_session_view_palette_expose_event (GtkWidget      *widget,
                                       GdkEventExpose *expose)
{
	gboolean ret = FALSE;
	cairo_t *cr;

	g_return_val_if_fail(GTK_IS_WIDGET(widget), FALSE);
	g_return_val_if_fail(expose != NULL, FALSE);

	if (gtk_widget_is_drawable(widget)) {
		cr = gdk_cairo_create(expose->window);
		gdk_cairo_rectangle(cr, &expose->area);
		cairo_clip(cr);
		ret = ppg_session_view_palette_draw(widget, cr);
		cairo_destroy(cr);
	}

	return ret;
}
#endif


/**
 * ppg_session_view_dispose:
 * @view: (in): A #PpgSessionView.
 *
 * Handles the "dispose" signal for the PpgSessionView. References to other
 * objects are released.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_view_dispose (GObject *object)
{
	PpgSessionViewPrivate *priv = PPG_SESSION_VIEW(object)->priv;
	GPtrArray *views;
	gpointer instance;

	ENTRY;

	if ((instance = priv->session)) {
		priv->session = NULL;
		g_object_unref(instance);
	}

	if ((views = priv->instrument_views)) {
		priv->instrument_views = NULL;
		g_ptr_array_foreach(views, (GFunc)g_object_unref, NULL);
		g_ptr_array_free(views, TRUE);
	}

	ppg_clear_object(&priv->all_content);
	ppg_clear_object(&priv->background);
	ppg_clear_object(&priv->header_bg);
	ppg_clear_object(&priv->rows_table);
	ppg_clear_object(&priv->bar);
	ppg_clear_object(&priv->top_shadow);

	G_OBJECT_CLASS(ppg_session_view_parent_class)->dispose(object);

	EXIT;
}


static void
ppg_session_view_state_changed (GtkWidget    *widget,
                                GtkStateType  state)
{
	PpgSessionViewPrivate *priv;
	PpgSessionView *view = (PpgSessionView *)widget;
	GtkWidget *child;

	g_return_if_fail(PPG_IS_SESSION_VIEW(view));

	priv = view->priv;

	if (gtk_widget_is_sensitive(widget)) {
		g_object_get(priv->spinner, "widget", &child, NULL);
		g_object_set(priv->spinner,
		             "visibility", GOO_CANVAS_ITEM_INVISIBLE,
		             NULL);
		gtk_spinner_stop(GTK_SPINNER(child));
		g_object_unref(child);
	}
}


/**
 * ppg_session_view_finalize:
 * @object: (in): A #PpgSessionView.
 *
 * Finalizer for a #PpgSessionView instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_view_finalize (GObject *object)
{
	ENTRY;
	G_OBJECT_CLASS(ppg_session_view_parent_class)->finalize(object);
	EXIT;
}


/**
 * ppg_session_view_get_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (out): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Get a given #GObject property.
 */
static void
ppg_session_view_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
	PpgSessionView *view = PPG_SESSION_VIEW(object);

	switch (prop_id) {
	case PROP_SELECTED_ITEM:
		g_value_set_object(value, view->priv->selected);
		break;
	case PROP_SHOW_DATA:
		g_value_set_boolean(value, ppg_session_view_get_show_data(view));
		break;
	case PROP_ZOOM:
		g_value_set_double(value, view->priv->zoom);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}


/**
 * ppg_session_view_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
static void
ppg_session_view_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
	PpgSessionView *view = PPG_SESSION_VIEW(object);

	switch (prop_id) {
	case PROP_SELECTED_ITEM:
		ppg_session_view_set_selected_item(view, g_value_get_object(value));
		break;
	case PROP_SESSION:
		ppg_session_view_set_session(view, g_value_get_object(value));
		break;
	case PROP_SHOW_DATA:
		ppg_session_view_set_show_data(view, g_value_get_boolean(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}


/**
 * ppg_session_view_class_init:
 * @klass: (in): A #PpgSessionViewClass.
 *
 * Initializes the #PpgSessionViewClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_view_class_init (PpgSessionViewClass *klass)
{
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->dispose = ppg_session_view_dispose;
	object_class->finalize = ppg_session_view_finalize;
	object_class->get_property = ppg_session_view_get_property;
	object_class->set_property = ppg_session_view_set_property;
	g_type_class_add_private(object_class, sizeof(PpgSessionViewPrivate));

	widget_class = GTK_WIDGET_CLASS(klass);
	widget_class->style_set = ppg_session_view_style_set;
	widget_class->size_allocate = ppg_session_view_size_allocate;
	widget_class->state_changed = ppg_session_view_state_changed;

	g_object_class_install_property(object_class,
	                                PROP_SELECTED_ITEM,
	                                g_param_spec_object("selected-item",
	                                                    "SelectedItem",
	                                                    "The selected item",
	                                                    PPG_TYPE_INSTRUMENT_VIEW,
	                                                    G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_SESSION,
	                                g_param_spec_object("session",
	                                                    "Session",
	                                                    "The views session",
	                                                    PPG_TYPE_SESSION,
	                                                    G_PARAM_WRITABLE));

	g_object_class_install_property(object_class,
	                                PROP_SHOW_DATA,
	                                g_param_spec_boolean("show-data",
	                                                     "show-data",
	                                                     "show-data",
	                                                     FALSE,
	                                                     G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_ZOOM,
	                                g_param_spec_double("zoom",
	                                                    "zoom",
	                                                    "zoom",
	                                                    ZOOM_MIN,
	                                                    ZOOM_MAX,
	                                                    1.0,
	                                                    G_PARAM_READABLE));
}


/**
 * ppg_session_view_init:
 * @view: (in): A #PpgSessionView.
 *
 * Initializes the newly created #PpgSessionView instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_session_view_init (PpgSessionView *view)
{
	PpgSessionViewPrivate *priv;
	cairo_pattern_t *p;
	GooCanvasItem *root;
	GtkWidget *a;
	GtkWidget *e;
	GtkWidget *h;
	GtkWidget *img;
	GtkWidget *s;
	GtkWidget *spinner;

	priv = G_TYPE_INSTANCE_GET_PRIVATE(view, PPG_TYPE_SESSION_VIEW,
	                                   PpgSessionViewPrivate);
	view->priv = priv;

	priv->instrument_views = g_ptr_array_new();
	priv->zoom = 1.0;

	priv->paned = g_object_new(GTK_TYPE_VPANED,
	                           "visible", TRUE,
	                           NULL);
	gtk_container_add(GTK_CONTAINER(view), priv->paned);

	priv->table = g_object_new(GTK_TYPE_TABLE,
	                           "n-columns", 4,
	                           "n-rows", 4,
	                           "visible", TRUE,
	                           NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(priv->paned), priv->table,
	                                  "shrink", FALSE,
	                                  "resize", TRUE,
	                                  NULL);

	e = g_object_new(GTK_TYPE_EVENT_BOX,
	                 "app-paintable", TRUE,
	                 "visible", TRUE,
	                 NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(priv->table), e,
	                                  "bottom-attach", 2,
	                                  "left-attach", 0,
	                                  "right-attach", 1,
	                                  "top-attach", 0,
	                                  "x-options", 0,
	                                  "y-options", GTK_FILL,
	                                  NULL);
#if GTK_CHECK_VERSION(2, 91, 0)
	g_signal_connect(e, "draw",
	                 G_CALLBACK(ppg_session_view_palette_draw),
	                 NULL);
#else
	g_signal_connect(e, "expose-event",
	                 G_CALLBACK(ppg_session_view_palette_expose_event),
	                 NULL);
#endif

	a = g_object_new(GTK_TYPE_ALIGNMENT,
	                 "visible", TRUE,
	                 "yalign", 0.0,
	                 "yscale", 0.0,
	                 NULL);
	gtk_container_add(GTK_CONTAINER(e), a);

	priv->palette = g_object_new(GTK_TYPE_VBOX,
	                             "spacing", 3,
	                             "visible", TRUE,
	                             "border-width", 3,
	                             NULL);
	gtk_container_add(GTK_CONTAINER(a), priv->palette);

	img = g_object_new(GTK_TYPE_IMAGE,
	                   "visible", TRUE,
	                   "pixbuf", LOAD_INLINE_PIXBUF(pointer_pixbuf),
	                   NULL);
	priv->pointer_tool = g_object_new(GTK_TYPE_RADIO_BUTTON,
	                                  "active", TRUE,
	                                  "child", img,
	                                  "draw-indicator", FALSE,
	                                  "relief", GTK_RELIEF_NONE,
	                                  "sensitive", FALSE,
	                                  "tooltip-markup", _("Row selection tool: <b>P</b>"),
	                                  "visible", TRUE,
	                                  VEXPAND(FALSE)
	                                  NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(priv->palette),
	                                  priv->pointer_tool,
	                                  "expand", FALSE,
	                                  NULL);
	g_signal_connect_swapped(priv->pointer_tool, "clicked",
	                         G_CALLBACK(ppg_session_view_pointer_clicked),
	                         view);

	img = g_object_new(GTK_TYPE_IMAGE,
	                   "pixbuf", LOAD_INLINE_PIXBUF(selection_pixbuf),
	                   "visible", TRUE,
	                   NULL);
	priv->time_tool = g_object_new(GTK_TYPE_RADIO_BUTTON,
	                               "active", FALSE,
	                               "child", img,
	                               "draw-indicator", FALSE,
	                               "group", priv->pointer_tool,
	                               "relief", GTK_RELIEF_NONE,
	                               "sensitive", FALSE,
	                               "tooltip-markup", _("Range selection tool: <b>R</b>"),
	                               "visible", TRUE,
	                               VEXPAND(FALSE)
	                               NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(priv->palette),
	                                  priv->time_tool,
	                                  "expand", FALSE,
	                                  NULL);
	g_signal_connect_swapped(priv->time_tool, "clicked",
	                         G_CALLBACK(ppg_session_view_selection_clicked),
	                         view);

	img = g_object_new(GTK_TYPE_IMAGE,
	                   "visible", TRUE,
	                   "icon-name", GTK_STOCK_FIND,
	                   "icon-size", GTK_ICON_SIZE_MENU,
	                   NULL);
	priv->zoom_tool = g_object_new(GTK_TYPE_RADIO_BUTTON,
	                               "active", TRUE,
	                               "child", img,
	                               "draw-indicator", FALSE,
	                               "group", priv->pointer_tool,
	                               "relief", GTK_RELIEF_NONE,
	                               "sensitive", FALSE,
	                               "tooltip-markup", _("Zoom tool: <b>Z</b>"),
	                               "visible", TRUE,
	                               VEXPAND(FALSE)
	                               NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(priv->palette),
	                                  priv->zoom_tool,
	                                  "expand", FALSE,
	                                  NULL);
	g_signal_connect_swapped(priv->zoom_tool, "clicked",
	                         G_CALLBACK(ppg_session_view_zoom_clicked),
	                         view);

	h = g_object_new(PPG_TYPE_HEADER,
	                 "visible", TRUE,
	                 "width-request", 200,
	                 HEXPAND(FALSE)
	                 NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(priv->table), h,
	                                  "bottom-attach", 1,
	                                  "left-attach", 1,
	                                  "right-attach", 2,
	                                  "top-attach", 0,
	                                  "x-options", GTK_FILL,
	                                  "y-options", GTK_FILL,
	                                  NULL);

	priv->ruler = g_object_new(PPG_TYPE_RULER,
	                           "visible", TRUE,
	                           "lower", 0.0,
	                           "upper", 500.0,
	                           NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(priv->table), priv->ruler,
	                                  "bottom-attach", 1,
	                                  "left-attach", 2,
	                                  "right-attach", 3,
	                                  "top-attach", 0,
	                                  "x-options", GTK_FILL | GTK_EXPAND,
	                                  "y-options", GTK_FILL,
	                                  NULL);

	priv->canvas = g_object_new(GOO_TYPE_CANVAS,
	                            "automatic-bounds", TRUE,
	                            "can-focus", TRUE,
	                            "visible", TRUE,
	                            NULL);
	root = goo_canvas_get_root_item(GOO_CANVAS(priv->canvas));
	gtk_container_add_with_properties(GTK_CONTAINER(priv->table), priv->canvas,
	                                  "bottom-attach", 2,
	                                  "left-attach", 1,
	                                  "right-attach", 3,
	                                  "top-attach", 1,
	                                  "x-options", GTK_FILL | GTK_EXPAND,
	                                  "y-options", GTK_FILL | GTK_EXPAND,
	                                  NULL);
	g_signal_connect_swapped(root, "button-press-event",
	                         G_CALLBACK(ppg_session_view_button_press_event),
	                         view);
	g_signal_connect_swapped(root, "button-release-event",
	                         G_CALLBACK(ppg_session_view_button_release_event),
	                         view);
	g_signal_connect_swapped(root, "motion-notify-event",
	                         G_CALLBACK(ppg_session_view_motion_notify_event),
	                         view);
	g_signal_connect_swapped(root, "scroll-event",
	                         G_CALLBACK(ppg_session_view_scroll_event),
	                         view);
	g_signal_connect_swapped(priv->canvas, "size-allocate",
	                         G_CALLBACK(ppg_session_view_canvas_size_allocate),
	                         view);

	priv->background = g_object_new(GOO_TYPE_CANVAS_RECT,
	                                "fill-color", "#000000",
	                                "line-width", 0.0,
	                                "parent", root,
	                                "width", 300.0,
	                                "height", 300.0,
	                                NULL);

	priv->header_bg = g_object_new(GOO_TYPE_CANVAS_RECT,
	                               "fill-color", "#000000",
	                               "line-width", 0.0,
	                               "parent", root,
	                               "width", 200.0,
	                               "x", 0.0,
	                               "y", 0.0,
	                               NULL);

	priv->bar = g_object_new(GOO_TYPE_CANVAS_RECT,
	                         "fill-color", "#ffffff",
	                         "height", 100.0,
	                         "line-width", 0.0,
	                         "parent", root,
	                         "width", 1.0,
	                         "x", 199.0,
	                         NULL);

	priv->all_content = g_object_new(GOO_TYPE_CANVAS_TABLE,
	                                 "parent", root,
	                                 "width", 300.0,
	                                 NULL);

	priv->rows_table = g_object_new(GOO_TYPE_CANVAS_TABLE,
	                                "parent", priv->all_content,
	                                "fill-color", "#000000",
	                                NULL);
	goo_canvas_item_set_child_properties(priv->all_content, priv->rows_table,
	                                     "row", 0,
	                                     "column", 0,
	                                     NULL);

	p = ppg_session_view_create_shadow_pattern(GDK_GRAVITY_NORTH);
	priv->top_shadow = g_object_new(GOO_TYPE_CANVAS_IMAGE,
	                                "height", SHADOW_HEIGHT,
	                                "pattern", p,
	                                "parent", priv->all_content,
	                                "width", 1.0,
	                                "x", 0.0,
	                                "y", 0.0,
	                                NULL);
	goo_canvas_item_set_child_properties(priv->all_content, priv->top_shadow,
	                                     "row", 1,
	                                     "column", 0,
	                                     NULL);
	cairo_pattern_destroy(p);

	priv->selection.item = g_object_new(GOO_TYPE_CANVAS_RECT,
	                                    "height", 10.0,
	                                    "line-width", 0.0,
	                                    "parent", root,
	                                    "visibility", GOO_CANVAS_ITEM_HIDDEN,
	                                    "width", 200.0,
	                                    "x", 300.0,
	                                    NULL);

	spinner = g_object_new(GTK_TYPE_SPINNER,
	                       "visible", TRUE,
	                       NULL);
	gtk_spinner_start(GTK_SPINNER(spinner));
	priv->spinner = g_object_new(GOO_TYPE_CANVAS_WIDGET,
	                             "height", 100.0,
	                             "anchor", GOO_CANVAS_ANCHOR_CENTER,
	                             "parent", root,
	                             "widget", spinner,
	                             "width", 100.0,
	                             "x", 100.0,
	                             "y", 100.0,
	                             NULL);

	priv->vadj = g_object_new(GTK_TYPE_ADJUSTMENT,
	                          "page-size", 1.0,
	                          "step-increment", 1.0,
	                          "upper", 1.0,
	                          NULL);
	s = g_object_new(GTK_TYPE_VSCROLLBAR,
	                 "adjustment", priv->vadj,
	                 "visible", TRUE,
	                 NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(priv->table), s,
	                                  "bottom-attach", 2,
	                                  "left-attach", 3,
	                                  "right-attach", 4,
	                                  "top-attach", 0,
	                                  "x-options", GTK_FILL,
	                                  "y-options", GTK_FILL | GTK_EXPAND,
	                                  NULL);
	g_signal_connect_swapped(priv->vadj, "value-changed",
	                         G_CALLBACK(ppg_session_view_vadj_value_changed),
	                         view);

	a = g_object_new(GTK_TYPE_ALIGNMENT,
	                 "visible", TRUE,
	                 "xscale", 1.0,
	                 "yalign", 0.0,
	                 "yscale", 0.0,
	                 NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(priv->table), a,
	                                  "bottom-attach", 3,
	                                  "left-attach", 2,
	                                  "right-attach", 3,
	                                  "top-attach", 2,
	                                  "x-options", GTK_FILL | GTK_EXPAND,
	                                  "y-options", GTK_FILL,
	                                  NULL);

	priv->hadj = g_object_new(GTK_TYPE_ADJUSTMENT,
	                          "page-size", 1.0,
	                          "step-increment", 1.0,
	                          "upper", 1.0,
	                          NULL);
	s = g_object_new(GTK_TYPE_HSCROLLBAR,
	                 "adjustment", priv->hadj,
	                 "visible", TRUE,
	                 NULL);
	gtk_container_add(GTK_CONTAINER(a), s);
	g_signal_connect_swapped(priv->hadj, "value-changed",
	                         G_CALLBACK(ppg_session_view_hadj_value_changed),
	                         view);

	h = g_object_new(GTK_TYPE_HBOX,
	                 "visible", TRUE,
	                 "width-request", 200,
	                 HEXPAND(FALSE)
					 NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(priv->table), h,
	                                  "bottom-attach", 3,
	                                  "left-attach", 0,
	                                  "right-attach", 2,
	                                  "top-attach", 2,
	                                  "x-options", GTK_FILL,
	                                  "y-options", GTK_FILL,
	                                  NULL);

	img = g_object_new(GTK_TYPE_IMAGE,
	                   "icon-name", GTK_STOCK_ZOOM_OUT,
	                   "icon-size", GTK_ICON_SIZE_MENU,
	                   "visible", TRUE,
	                   "yalign", 0.0,
	                   "ypad", 2,
	                   NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(h), img,
	                                  "expand", FALSE,
	                                  "padding", 6,
	                                  NULL);

	priv->zadj = g_object_new(GTK_TYPE_ADJUSTMENT,
	                          "lower", 0.005,
	                          "upper", 2.0,
	                          "value", 1.0,
	                          "step-increment", 0.01,
	                          "page-increment", 0.1,
	                          NULL);
	s = g_object_new(GTK_TYPE_HSCALE,
	                 "adjustment", priv->zadj,
	                 "draw-value", FALSE,
	                 "visible", TRUE,
	                 NULL);
	gtk_scale_add_mark(GTK_SCALE(s), 1.0, GTK_POS_BOTTOM, NULL);
	gtk_container_add(GTK_CONTAINER(h), s);
	g_signal_connect_swapped(priv->zadj, "value-changed",
	                         G_CALLBACK(ppg_session_view_zadj_value_changed),
	                         view);

	img = g_object_new(GTK_TYPE_IMAGE,
	                   "icon-name", GTK_STOCK_ZOOM_IN,
	                   "icon-size", GTK_ICON_SIZE_MENU,
	                   "visible", TRUE,
	                   "yalign", 0.0,
	                   "ypad", 2,
	                   NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(h), img,
	                                  "expand", FALSE,
	                                  "padding", 6,
	                                  NULL);

	priv->data_container = g_object_new(GTK_TYPE_ALIGNMENT,
	                                    "visible", FALSE,
	                                    NULL);
	gtk_container_add(GTK_CONTAINER(priv->paned), priv->data_container);

	gtk_adjustment_value_changed(priv->zadj);
}

/* ppg-rt-graph.c
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

#include <math.h>
#include <time.h>

#include "ppg-frame-source.h"
#include "ppg-log.h"
#include "ppg-marshal.h"
#include "ppg-renderer.h"
#include "ppg-rt-graph.h"
#include "ppg-util.h"


#define FRAMES_PER_SECOND      30
#define LABEL_XPAD             3
#define LABEL_YPAD             3


G_DEFINE_TYPE(PpgRtGraph, ppg_rt_graph, GTK_TYPE_DRAWING_AREA)


struct _PpgRtGraphPrivate
{
	PpgRtGraphFlags flags;           /* Various flags */
	cairo_surface_t *foreground;     /* forground surface for gdk window */
	cairo_surface_t *background;     /* background surface for gdk window */
	PangoFontDescription *font_desc; /* Font description for labels */
	PpgRenderer *renderer;           /* Renderer used for async rendering */
	GdkRectangle content_area;       /* Visible data area of widget */
	GdkRectangle data_area;          /* Data and buffer area of foreground */
	guint frame_handler;             /* Frame renderering handler */
	guint invalidate_handler;        /* Renderer invalidate handler */
	guint adjustment_handler;        /* Renderer adjustment range handler */
	gdouble n_seconds;               /* Number of seconds to display */
	gdouble n_buffered;              /* Number of seconds to buffer */
	gdouble offset_time;             /* Ring buffer offset in seconds */
	gdouble begin_time;              /* Beginning time of stored range */
	gdouble end_time;                /* End time of stored range */
	gdouble lower_value;             /* Lower value visualized */
	gdouble upper_value;             /* Upper value visualized */
	guint32 frame_count;             /* Debugging frame counter */
	gint label_height;               /* cached sizing of label height */
	gint label_width;                /* cached sizing of label width */
	gint max_lines;                  /* Maximum number of lines to draw */
	gint min_lines;                  /* Minimum number of lines to draw */
};


enum
{
	PROP_0,

	PROP_MAX_LINES,
	PROP_MIN_LINES,
	PROP_RENDERER,
};


enum
{
	FORMAT_VALUE,

	SIGNAL_LAST
};


static void ppg_rt_graph_render_background (PpgRtGraph *graph);
static void ppg_rt_graph_render_foreground (PpgRtGraph *graph,
                                            gdouble     begin_time,
                                            gdouble     end_time);


static gboolean PPG_RT_GRAPH_DEBUG   = FALSE;
static guint    signals[SIGNAL_LAST] = { 0 };


/**
 * ppg_rt_graph_frame_timeout:
 * @data: (in): A #PpgRtGraph.
 *
 * Handle a timeout from the main loop to queue a draw request of the
 * next frame.
 *
 * Returns: %TRUE always.
 * Side effects: None.
 */
static gboolean
ppg_rt_graph_frame_timeout (gpointer data)
{
	GtkWidget *widget = (GtkWidget *)data;

	g_return_val_if_fail(GTK_IS_WIDGET(widget), FALSE);

	gtk_widget_queue_draw(widget);
	return TRUE;
}


/**
 * ppg_rt_graph_start:
 * @graph: (in): A #PpgRtGraph.
 *
 * Start the #PpgRtGraph. This needs to be called to start the various
 * main loop timeouts to update the frames.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_rt_graph_start (PpgRtGraph *graph)
{
	PpgRtGraphPrivate *priv;
	gdouble time_span;

	g_return_if_fail(PPG_IS_RT_GRAPH(graph));
	g_return_if_fail(!graph->priv->frame_handler);

	priv = graph->priv;

	/*
	 * Register the frame handler.
	 */
	priv->frame_handler = ppg_frame_source_add(FRAMES_PER_SECOND,
	                                           ppg_rt_graph_frame_timeout,
	                                           graph);

	/*
	 * Prepare the time spans and render the current data-set.
	 */
	time_span = priv->n_seconds + priv->n_buffered;
	priv->begin_time = ppg_get_current_time();
	priv->end_time = priv->begin_time + time_span;
	ppg_rt_graph_render_background(graph);
	ppg_rt_graph_render_foreground(graph, 0.0, 0.0);
}


/**
 * ppg_rt_graph_stop:
 * @graph: (in): A #PpgRtGraph.
 *
 * Stop the realtime graph from animating. The main loop timeout to update the
 * frames is disabled.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_rt_graph_stop (PpgRtGraph *graph)
{
	g_return_if_fail(PPG_IS_RT_GRAPH(graph));
	ppg_clear_source(&graph->priv->frame_handler);
}


/**
 * ppg_rt_graph_adjustment_changed:
 * @graph: (in): A #PpgRtGraph.
 * @adjustment: (in): A #GtkAdjustment.
 *
 * Handle the "changed" signal for the adjustment encapsulating the vertical
 * range of the graph.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_rt_graph_adjustment_changed (PpgRtGraph    *graph,
                                 GtkAdjustment *adjustment)
{
	PpgRtGraphPrivate *priv;

	g_return_if_fail(PPG_IS_RT_GRAPH(graph));
	g_return_if_fail(GTK_IS_ADJUSTMENT(adjustment));

	priv = graph->priv;

	g_object_get(adjustment,
	             "lower", &priv->lower_value,
	             "upper", &priv->upper_value,
	             NULL);
}


/**
 * ppg_rt_graph_invalidate:
 * @graph: (in): A #PpgRtGraph.
 * @begin_time: (in): The beginning time of the range.
 * @end_time: (in): The end time of the rnage.
 *
 * Invalidates a region of the graph based on the given time periods.
 * A request to render the given range is performed immediately.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_rt_graph_invalidate (PpgRtGraph  *graph,
                         gdouble      begin_time,
                         gdouble      end_time,
                         PpgRenderer *renderer)
{
	ENTRY;
	ppg_rt_graph_render_foreground(graph, begin_time, end_time);
	EXIT;
}


/**
 * ppg_rt_graph_set_renderer:
 * @graph: (in): A #PpgRtGraph.
 * @renderer: (in): A #PpgRenderer.
 *
 * Sets the renderer for the graph. After calling this function, @graph
 * is the owner of the floating reference to @renderer.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_rt_graph_set_renderer (PpgRtGraph  *graph,
                           PpgRenderer *renderer)
{
	PpgRtGraphPrivate *priv;
	GtkAdjustment *adj;

	g_return_if_fail(PPG_IS_RT_GRAPH(graph));
	g_return_if_fail(PPG_IS_RENDERER(renderer));
	g_return_if_fail(graph->priv->renderer == NULL);

	priv = graph->priv;

	priv->renderer = g_object_ref_sink(renderer);
	adj = ppg_renderer_get_adjustment(renderer);
	g_object_get(adj,
	             "lower", &priv->lower_value,
	             "upper", &priv->upper_value,
	             NULL);
	priv->adjustment_handler = 
		g_signal_connect_swapped(adj, "changed",
		                         G_CALLBACK(ppg_rt_graph_adjustment_changed),
		                         graph);
	priv->invalidate_handler =
		g_signal_connect_swapped(priv->renderer, "invalidate",
		                         G_CALLBACK(ppg_rt_graph_invalidate),
		                         graph);
}


/**
 * ppg_rt_graph_get_fg_x_for_time:
 * @graph: (in): A #PpgRtGraph.
 * @time_: (in): A time.
 *
 * Retrieves the X position for the given time using data coordinates. This
 * value may be outside the data_area rectangle as some calculations need to
 * take that into account.
 *
 * Returns: A gdouble containing the position in data coordinates.
 * Side effects: None.
 */
static gdouble
ppg_rt_graph_get_fg_x_for_time (PpgRtGraph *graph,
                                gdouble     time_)
{
	PpgRtGraphPrivate *priv;
	gdouble pixel_width;
	gdouble time_span;

	g_return_val_if_fail(PPG_IS_RT_GRAPH(graph), 0.0);

	priv = graph->priv;

	time_span = priv->n_seconds + priv->n_buffered;
	pixel_width = priv->data_area.width;
	return (time_ - priv->begin_time) / time_span * pixel_width;
}


/**
 * ppg_rt_graph_get_fg_time_for_x:
 * @graph: (in): A #PpgRtGraph.
 * @x: (in): The x value in data coordinates.
 *
 * Retrieves the time for the position @x based on the current time range of
 * the graph.
 *
 * Returns: A #gdouble containing the time.
 * Side effects: None.
 */
static gdouble
ppg_rt_graph_get_fg_time_for_x (PpgRtGraph *graph,
                                gdouble     x)
{
	PpgRtGraphPrivate *priv;
	gdouble pixel_width;
	gdouble time_span;

	g_return_val_if_fail(PPG_IS_RT_GRAPH(graph), 0.0);

	priv = graph->priv;

	time_span = priv->n_seconds + priv->n_buffered;
	pixel_width = priv->data_area.width;
	return x / pixel_width * time_span + priv->begin_time;
}


/**
 * ppg_rt_graph_draw_surface_at_offset:
 * @graph: (in): A #PpgRtGraph.
 * @surface: (in): A #cairo_surface_t.
 * @surface_x: (in): The x position to place @surface.
 * @surface_y: (in): The y position to place @surface.
 * @x: (in): The x position of the rectangle.
 * @y: (in): The y position of the rectangle.
 * @width: (in): The width of the rectangle.
 * @height: (in): The height of the rectangle.
 *
 * Draws the surface using @cr. @surface_x and @surface_y specify
 * the position to place the suraface on the context. The rectangle
 * is drawn using @x @y @width and @height.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_rt_graph_draw_surface_at_offset (cairo_t         *cr,
                                     cairo_surface_t *surface,
                                     gdouble          surface_x,
                                     gdouble          surface_y,
                                     gdouble          x,
                                     gdouble          y,
                                     gdouble          width,
                                     gdouble          height)
{
	/*
	 * Only draw what we can do on integer aligned offsets. This is why
	 * we render outside the required range a bit in the asynchronous
	 * draw. This helps avoid a "pinstripe" effect on rendered output.
	 */
	width -= ceil(x) - x;
	x = ceil(x);

	cairo_save(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_surface(cr, surface, surface_x, surface_y);
	cairo_rectangle(cr, x, y, width, height);
	cairo_fill(cr);
	cairo_restore(cr);
}


/**
 * ppg_rt_graph_draw_surface:
 * @graph: (in): A #PpgRtGraph.
 * @surface: (in): A #cairo_surface_t.
 * @x: (in): The x position of the rectangle.
 * @y: (in): The y position of the rectangle.
 * @width: (in): The width of the rectangle.
 * @height: (in): The height of the rectangle.
 *
 * Places the surface at the given rectangle and draws it with the
 * cairo context @cr.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_rt_graph_draw_surface (cairo_t *cr,
                           cairo_surface_t *surface,
                           gdouble x,
                           gdouble y,
                           gdouble width,
                           gdouble height)
{
	ppg_rt_graph_draw_surface_at_offset(cr, surface, x, y, x, y,
	                                    width, height);
}


/**
 * ppg_rt_graph_task_notify_state:
 * @graph: (in): A #PpgRtGraph.
 * @pspec: (in): A #GParamSpec.
 * @task: (in): A #PpgTask.
 *
 * Handle the "notify::state" signal from @task. If the task completed
 * successfully, go ahead and copy the render surface to the foreground
 * surface.
 *
 * Returns: None.
 * Side effects: Foreground may be updated.
 */
static void
ppg_rt_graph_task_notify_state (PpgRtGraph *graph,
                                GParamSpec *pspec,
                                PpgTask    *task)
{
	PpgRtGraphPrivate *priv;
	cairo_surface_t *surface;
	PpgTaskState state;
	cairo_t *cr = NULL;
	gdouble begin_time;
	gdouble change;
	gdouble end_time;
	gdouble height;
	gdouble offset_x;
	gdouble target_x;
	gdouble target_y;
	gdouble time_span;
	gdouble width;
	gdouble x;
	gdouble y;

	ENTRY;

	g_return_if_fail(PPG_IS_RT_GRAPH(graph));

	priv = graph->priv;

	g_object_get(task,
	             "state", &state,
	             "surface", &surface,
	             NULL);

	if (state == PPG_TASK_SUCCESS) {
		g_object_get(task,
		             "begin-time", &begin_time,
		             "end-time", &end_time,
		             "height", &height,
		             "width", &width,
		             "x", &x,
		             "y", &y,
		             NULL);

		/*
		 * Make sure this render has the right sizing information.
		 * A task could finish after a new size-allocate and is therefore
		 * now bogus.
		 */
		if (((gint)height) != priv->data_area.height) {
			GOTO(cleanup);
		}

		time_span = priv->n_seconds + priv->n_buffered;

		/*
		 * If we received a time range outside of what our visible area is,
		 * lets update the range our graph contains.
		 */
		if (end_time > priv->end_time) {
			change = end_time - priv->end_time;
			priv->offset_time = fmod(priv->offset_time + change, time_span);
			priv->end_time = end_time;
			priv->begin_time = end_time - time_span;
		}

		/*
		 * Determine the X offset for the particular time as if there is no
		 * offset in the ring buffer.
		 */
		target_x = ppg_rt_graph_get_fg_x_for_time(graph, begin_time);
		target_y = 0;

		/*
		 * Create our cairo context to draw onto the Xlib surface.
		 */
		cr = cairo_create(priv->foreground);

		/*
		 * Translate the X position into the ring buffers offset.
		 */
		if (priv->offset_time > 0.0) {
			offset_x = ppg_rt_graph_get_fg_x_for_time(
					graph,
					priv->begin_time + priv->offset_time);
			target_x = fmod(target_x + offset_x, priv->data_area.width);
			if ((target_x + width) > priv->data_area.width) {
				/*
				 * Well shit, we have to split this up into two draws. One at
				 * the end of the ring buffer, and one at the beginning.
				 */
				ppg_rt_graph_draw_surface(cr, surface,
				                          target_x, target_y,
				                          priv->data_area.width - target_x,
				                          height);
				width -= priv->data_area.width - target_x;
				target_x = priv->data_area.width - target_x;
				ppg_rt_graph_draw_surface_at_offset(cr, surface,
				                                    -target_x, target_y,
				                                    0.0, target_y,
				                                    width, height);
			} else {
				ppg_rt_graph_draw_surface(cr, surface,
				                          target_x, target_y, width, height);
			}
		} else {
			ppg_rt_graph_draw_surface(cr, surface,
			                          target_x, target_y, width, height);
		}

		cairo_destroy(cr);
		gtk_widget_queue_draw(GTK_WIDGET(graph));
	}

  cleanup:
	/*
	 * Cleanup our surface that was created for the task.
	 */
	if ((state & PPG_TASK_FINISHED_MASK) != 0) {
		cairo_surface_destroy(surface);
	}

	EXIT;
}


/**
 * ppg_rt_graph_get_n_y_lines:
 * @graph: (in): A #PpgRtGraph.
 *
 * Determine the maximum number of Y-axis lines to be drawn.
 *
 * Returns: A #guint containing the number of lines to draw.
 * Side effects: None.
 */
static guint
ppg_rt_graph_get_n_y_lines (PpgRtGraph *graph)
{
	PpgRtGraphPrivate *priv;
	gdouble height;
	gint max_lines;

	g_return_val_if_fail(PPG_IS_RT_GRAPH(graph), 0);

	priv = graph->priv;

	height = (priv->label_height + LABEL_YPAD * 2 + 5);
	if (!(max_lines = priv->max_lines)) {
		max_lines = priv->content_area.height / height;
	}
	return CLAMP(priv->content_area.height / height,
	             priv->min_lines, max_lines);
}


/**
 * ppg_rt_graph_get_n_x_lines:
 * @graph: (in): A #PpgRtGraph.
 *
 * Determine the number of X-axis lines to be drawn.
 *
 * Returns: None.
 * Side effects: None.
 */
static guint
ppg_rt_graph_get_n_x_lines (PpgRtGraph *graph)
{
	/*
	 * TODO: Determine N lines based on height/values.
	 */
	return 5;
}


/**
 * ppg_rt_graph_format_value:
 * @graph: (in): A #PpgRtGraph.
 * @value: (in): A #gdouble.
 *
 * Formats the given value into a string.
 *
 * Returns: A newly allocated string.
 * Side effects: None.
 */
static gchar*
ppg_rt_graph_format_value (PpgRtGraph *graph,
                           gdouble     value)
{
	PpgRtGraphPrivate *priv;

	g_return_val_if_fail(PPG_IS_RT_GRAPH(graph), NULL);

	priv = graph->priv;

	if ((priv->flags & PPG_RT_GRAPH_PERCENT)) {
		return g_strdup_printf("%d %%", (gint)value);
	}
	return g_strdup_printf("%0.0f", value);
}


/**
 * ppg_rt_graph_render_background:
 * @graph: (in): A #PpgRtGraph.
 *
 * Render the background of the graph.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_rt_graph_render_background (PpgRtGraph *graph)
{
	static const gdouble dashes[] = { 1.0, 2.0 };
	PpgRtGraphPrivate *priv;
	GtkStateType state;
	PangoLayout *layout;
	GtkWidget *widget = (GtkWidget *)graph;
	GtkStyle *style;
	cairo_t *cr;
	gdouble value;
	gdouble x;
	gdouble y;
	gchar *text;
	gchar label[32];
	guint n_lines;
	gint i;
	gint height;
	gint width;

	g_return_if_fail(PPG_IS_RT_GRAPH(graph));

	priv = graph->priv;

	/*
	 * Retrieve required resources.
	 */
	style = gtk_widget_get_style(widget);
	state = gtk_widget_get_state(widget);
	cr = cairo_create(priv->background);

	/*
	 * Set background color to the styles light color.
	 */
	gdk_cairo_set_source_color(cr, &style->light[state]);
	cairo_rectangle(cr,
	                priv->content_area.x + 0.5,
	                priv->content_area.y + 0.5,
	                priv->content_area.width - 1.0,
	                priv->content_area.height - 1.0);
	cairo_fill(cr);

	/*
	 * Stroke the outer line or the graph to the styles foreground
	 * color.
	 */
	gdk_cairo_set_source_color(cr, &style->fg[state]);
	cairo_set_line_width(cr, 1.0);
	cairo_set_dash(cr, dashes, G_N_ELEMENTS(dashes), 0);
	cairo_rectangle(cr,
	                priv->content_area.x + 0.5,
	                priv->content_area.y + 0.5,
	                priv->content_area.width - 1.0,
	                priv->content_area.height - 1.0);
	cairo_stroke(cr);

	layout = pango_cairo_create_layout(cr);
	pango_layout_set_font_description(layout, priv->font_desc);

	/*
	 * Stroke the inner vertical grid lines of the graph to the styles
	 * foreground color. Draw the label at the bottom.
	 */
	n_lines = ppg_rt_graph_get_n_x_lines(graph);
	for (i = 0; i <= n_lines; i++) {
		x = priv->content_area.x +
		    floor(priv->content_area.width / ((gdouble)n_lines + 1) * i) +
		    0.5;
		y = priv->content_area.y + 1.5;

		/*
		 * Don't draw the first line.
		 */
		if (i != 0) {
			cairo_move_to(cr, x, y);
			cairo_line_to(cr, x, y + priv->content_area.height - 3.0);
			cairo_stroke(cr);
		}

		/*
		 * Time labels for X axis.
		 */
		value = floor(priv->n_seconds / (n_lines + 1.0) * (n_lines + 1 - i));
		g_snprintf(label, sizeof label, "%d", (gint)value);
		pango_layout_set_text(layout, label, -1);
		y = priv->content_area.y + priv->content_area.height + LABEL_YPAD;
		cairo_move_to(cr, x, y);
		pango_cairo_show_layout(cr, layout);
	}

	/*
	 * Stroke the inner horizontal grid lines of the graph to the styles
	 * foreground color.
	 */
	n_lines = ppg_rt_graph_get_n_y_lines(graph);
	for (i = 0; i <= n_lines; i++) {
		x = priv->content_area.x + 1.5;
		y = priv->content_area.y +
		    floor(priv->content_area.height / ((gdouble)n_lines + 1) * i) +
		    0.5;

		/*
		 * Don't draw the first line.
		 */
		if (i != 0) {
			cairo_move_to(cr, x, y);
			cairo_line_to(cr, x + priv->content_area.width - 3.0, y);
			cairo_stroke(cr);
		}

		/*
		 * Time labels for Y axis.
		 */
		value = priv->upper_value -
		        ((priv->upper_value - priv->lower_value) / (n_lines + 1.0) * i);
		g_signal_emit(graph, signals[FORMAT_VALUE], 0, value, &text);
		pango_layout_set_text(layout, text, -1);
		pango_layout_get_pixel_size(layout, &width, &height);
		x = priv->content_area.x - LABEL_XPAD - width;
		y = priv->content_area.y + (priv->content_area.height / (n_lines + 1.0) * i);
		cairo_move_to(cr, x, y);
		pango_cairo_show_layout(cr, layout);
		g_free(text);
	}

	/*
	 * Cleanup resources.
	 */
	g_object_unref(layout);
	cairo_destroy(cr);
}


/**
 * ppg_rt_graph_render_foreground:
 * @graph: (in): A #PpgRtGraph.
 * @begin_time: (in): A #gdouble of the begin time or 0.0.
 * @end_time: (in): A #gdouble of the end time or 0.0.
 *
 * Requests the foreground be redrawn using the graphs renderer.
 * If @begin_time or @end_time are 0.0, then they will be replaced
 * with the current begin-time or end-time of the graph, respectively.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_rt_graph_render_foreground (PpgRtGraph *graph,
                                gdouble     begin_time,
                                gdouble     end_time)
{
	PpgRtGraphPrivate *priv;
	cairo_surface_t *surface;
	PpgTask *task;
	gdouble height;
	gdouble width;
	gdouble x;

	g_return_if_fail(PPG_IS_RT_GRAPH(graph));

	priv = graph->priv;

	/*
	 * Make sure we have a surface and area for rendering.
	 */
	if (!priv->foreground) {
		return;
	}

	/*
	 * Fill in current values if requested.
	 */
	if (begin_time == 0.0) {
		begin_time = priv->begin_time;
	}
	if (end_time == 0.0) {
		end_time = priv->end_time;
	}

	/*
	 * Try to get a bit overlapping of a time so that we don't get a
	 * "pinstripe" effect in the output. It is also important to line
	 * up on an integer boundry so clearing the region on the destination
	 * doesn't have to antialias to neighboring pixels.
	 */
	height = priv->data_area.height;
	x = ppg_rt_graph_get_fg_x_for_time(graph, begin_time);
	x = floor(x) - 1.0;
	begin_time = ppg_rt_graph_get_fg_time_for_x(graph, x);
	width = ppg_rt_graph_get_fg_x_for_time(graph, end_time) - x;
	x = 0.0;

	g_assert(height > 0);
	g_assert(width > 0);
	g_assert(x >= 0);

	/*
	 * Create a new image surface to render upon within a thread.
	 * Since this is threaded, we cannot use an Xlib surface as that
	 * would cause corruption to the X thread.
	 */
	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
	                                     ceil(width), ceil(height));

	/*
	 * Create asynchronous render task to draw required contents to
	 * the surface. When the task has completed, we will copy the image
	 * contents to the surface we use to render.
	 */
	task = ppg_renderer_draw(priv->renderer,
	                         cairo_surface_reference(surface),
	                         begin_time,
	                         end_time,
	                         x, 0, width, height);
	g_signal_connect_swapped(task, "notify::state",
	                         G_CALLBACK(ppg_rt_graph_task_notify_state),
	                         graph);
	ppg_task_schedule(task);

	/*
	 * Cleanup after allocations.
	 */
	cairo_surface_destroy(surface);
}


/**
 * ppg_rt_graph_set_flags:
 * @graph: (in): A #PpgRtGraph.
 * @flags: (in): flags for the graph.
 *
 * Sets the flags for the graph.
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_rt_graph_set_flags (PpgRtGraph      *graph,
                        PpgRtGraphFlags  flags)
{
	g_return_if_fail(PPG_IS_RT_GRAPH(graph));

	graph->priv->flags = flags;
	ppg_rt_graph_render_background(graph);
	ppg_rt_graph_render_foreground(graph, 0.0, 0.0);
	gtk_widget_queue_draw(GTK_WIDGET(graph));
}


/**
 * ppg_rt_graph_size_allocate:
 * @graph: (in): A #PpgRtGraph.
 *
 * Handle the "size-allocate" signal for the #PpgRtGraph. Adjust the
 * sizing of various surfaces to match the new size.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_rt_graph_size_allocate (GtkWidget     *widget,
                            GtkAllocation *alloc)
{
	PpgRtGraphPrivate *priv;
	PangoLayout *layout;
	PpgRtGraph *graph = (PpgRtGraph *)widget;
	GdkWindow *window;
	cairo_t *cr;
	gdouble each;

	g_return_if_fail(PPG_IS_RT_GRAPH(graph));

	priv = graph->priv;

	/*
	 * Chain up to allow Gtk to perform the resize.
	 */
	GTK_WIDGET_CLASS(ppg_rt_graph_parent_class)->size_allocate(widget, alloc);

	/*
	 * Get our window to create our suraces and such.
	 */
	window = gtk_widget_get_window(widget);

	/*
	 * Cleanup after previous cairo surfaces.
	 */
	ppg_clear_pointer(&priv->foreground, cairo_surface_destroy);
	ppg_clear_pointer(&priv->background, cairo_surface_destroy);

	/*
	 * Determine the font height so we can draw our labels.
	 */
	cr = gdk_cairo_create(window);
	layout = pango_cairo_create_layout(cr);
	pango_layout_set_font_description(layout, priv->font_desc);
	pango_layout_set_text(layout, "XXXXXXXXX", -1);
	pango_layout_get_pixel_size(layout, &priv->label_width,
	                            &priv->label_height);
	g_object_unref(layout);
	cairo_destroy(cr);

	/*
	 * Setup ring buffer defaults.
	 */
	priv->offset_time = 0.0;

	/*
	 * Determine the visible data area.
	 */
	priv->content_area.x = 1 + priv->label_width;
	priv->content_area.y = 1;
	priv->content_area.width = alloc->width - priv->label_width - 2;
	priv->content_area.height = alloc->height
	                          - (LABEL_YPAD * 2)
	                          - priv->label_height
	                          - 2;

	/*
	 * Determine the data area including buffer area.
	 */
	each = priv->content_area.width / priv->n_seconds;
	priv->data_area.x = 0;
	priv->data_area.y = 0;
	priv->data_area.width = priv->content_area.width +
	                        ceil(priv->n_buffered * each);
	priv->data_area.height = priv->content_area.height;

	/*
	 * Create new cairo surface for drawing the background.
	 */
	priv->background =
		gdk_window_create_similar_surface(window,
		                                  CAIRO_CONTENT_COLOR_ALPHA,
		                                  alloc->width, alloc->height);

	/*
	 * Create new cairo surface for drawing the foreground. This matches the
	 * size of the content area plus enough space for the buffered region.
	 */
	priv->foreground =
		gdk_window_create_similar_surface(window,
		                                  CAIRO_CONTENT_COLOR_ALPHA,
		                                  priv->data_area.width,
		                                  priv->data_area.height);

	/*
	 * Render the entire graph immediately.
	 */
	ppg_rt_graph_render_background(graph);
	ppg_rt_graph_render_foreground(graph, 0.0, 0.0);
}


/**
 * ppg_rt_graph_draw:
 * @graph: (in): A #PpgRtGraph.
 * @cr: (in): A #cairo_t.
 *
 * Draws the widget to the cairo context @cr.
 *
 * Returns: always %FALSE.
 * Side effects: None.
 */
static gboolean
ppg_rt_graph_draw (GtkWidget *widget,
                   cairo_t   *cr)
{
	PpgRtGraph *graph = (PpgRtGraph *)widget;
	PpgRtGraphPrivate *priv;
	GtkAllocation a;
	gdouble height;
	gdouble offset_x;
	gdouble scroll_x;
	gdouble time_span;
	gdouble width;
	gdouble x;
	gdouble y;

	g_return_val_if_fail(PPG_IS_RT_GRAPH(graph), FALSE);

	priv = graph->priv;
	priv->frame_count++;

	if (gtk_widget_is_drawable(widget)) {
		/*
		 * Ensure everything is order.
		 */
		g_assert(priv->background);
		g_assert(priv->foreground);
		gtk_widget_get_allocation(widget, &a);

		/*
		 * Blit the background to the context.
		 */
		cairo_set_source_surface(cr, priv->background, 0, 0);
		cairo_rectangle(cr, 0, 0, a.width, a.height);
		cairo_fill(cr);

		/*
		 * Clip future operations to the content area.
		 */
		cairo_rectangle(cr,
		                priv->content_area.x + 1.5,
		                priv->content_area.y + 1.5,
		                priv->content_area.width - 3.0,
		                priv->content_area.height - 3.0);
		cairo_clip(cr);

		/*
		 * At this point we are ready to blit our foreground (which is treated
		 * as a pixmap ring buffer) to the widgets surface. This is done in
		 * two steps. First, we draw the freshest part of the surface (always
		 * the left hand side of the source) to the right hand side of the
		 * destination. Then we draw the less-fresh data (always the right
		 * hand side of the source) to the right hand side of the destination.
		 */
		offset_x = ppg_rt_graph_get_fg_x_for_time(graph,
		                                          priv->begin_time +
		                                          priv->offset_time);
		time_span = priv->n_seconds + priv->n_buffered;
		scroll_x = priv->data_area.width / time_span *
		           (ppg_get_current_time() - priv->end_time);

		/*
		 * Draw the left side of the ring buffer.
		 */
		if (offset_x > 0.0) {
			x = priv->content_area.x + priv->data_area.width - offset_x;
			y = priv->content_area.y;
			width = offset_x;
			height = priv->content_area.height;
			x -= scroll_x;
			cairo_set_source_surface(cr, priv->foreground, x, y);
			cairo_rectangle(cr, x, y, width, height);
			cairo_fill(cr);
		}

		/*
		 * Draw the right side of ring buffer.
		 */
		x = priv->content_area.x - offset_x;
		y = priv->content_area.y;
		width = priv->data_area.width;
		height = priv->content_area.height;
		x -= scroll_x;
		x += 1.0; /* Avoid 1 pixel gap */
		cairo_set_source_surface(cr, priv->foreground, x, y);
		cairo_rectangle(cr, x, y, width, height);
		cairo_fill(cr);

		/*
		 * Draw frame and clocking information on top of the graph if
		 * debugging is enabled.
		 */
		if (G_UNLIKELY(PPG_RT_GRAPH_DEBUG)) {
			PangoLayout *layout;
			struct timespec ts;
			char text[32];

			clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
			g_snprintf(text, sizeof text, "%ld.%09ld\n%d",
			           ts.tv_sec, ts.tv_nsec, priv->frame_count);
			layout = pango_cairo_create_layout(cr);
			cairo_set_source_rgb(cr, 1, 0, 0);
			pango_layout_set_text(layout, text, -1);
			cairo_move_to(cr, priv->content_area.width / 2,
			              priv->content_area.height / 2);
			pango_cairo_show_layout(cr, layout);
			g_object_unref(layout);
		}
	}

	return FALSE;
}


#if !GTK_CHECK_VERSION(2, 91, 0)
/**
 * ppg_rt_graph_expose_event:
 * @graph: (in): A #PpgRtGraph.
 * @expose: (in): A #GdkEventExpose.
 *
 * Handle the "expose-event" for Gtk+ 2.0 based widgets.
 *
 * Returns: %TRUE if signal handling should stop.
 * Side effects: None.
 */
static gboolean
ppg_rt_graph_expose_event (GtkWidget      *widget,
                           GdkEventExpose *expose)
{
	gboolean ret = FALSE;
	cairo_t *cr;

	if (gtk_widget_is_drawable(widget)) {
		cr = gdk_cairo_create(expose->window);
		gdk_cairo_rectangle(cr, &expose->area);
		cairo_clip(cr);
		ret = ppg_rt_graph_draw(widget, cr);
		cairo_destroy(cr);
	}

	return ret;
}
#endif


/**
 * ppg_rt_graph_set_max_lines:
 * @graph: (in): A #PpgRtGraph.
 *
 * Set the max number of Y-Axis lines that should be shown.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_rt_graph_set_max_lines (PpgRtGraph *graph,
                            gint        max_lines)
{
	g_return_if_fail(PPG_IS_RT_GRAPH(graph));

	graph->priv->max_lines = max_lines;
	ppg_rt_graph_render_background(graph);
	gtk_widget_queue_draw(GTK_WIDGET(graph));
}


/**
 * ppg_rt_graph_set_max_lines:
 * @graph: (in): A #PpgRtGraph.
 * @min_lines: (in): A #gint.
 *
 * Set the minimum number of lines that should be drawn on the Y-axis.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_rt_graph_set_min_lines (PpgRtGraph *graph,
                            gint        min_lines)
{
	g_return_if_fail(PPG_IS_RT_GRAPH(graph));

	graph->priv->min_lines = min_lines;
	ppg_rt_graph_render_background(graph);
	gtk_widget_queue_draw(GTK_WIDGET(graph));
}


/**
 * ppg_rt_graph_get_preferred_width:
 * @widget: (in): A #PpgRtGraph.
 * @min_width: (in): A location for the minimum width.
 * @natural_width: (in): A location for the natural width.
 *
 * Determine the preferred width of the #PpgRtGraph.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_rt_graph_get_preferred_width (GtkWidget *widget,
                                  gint      *min_width,
                                  gint      *natural_width)
{
	/*
	 * TODO: Calculate based on font sizing and such.
	 */
	*min_width = *natural_width = 150;
}


/**
 * ppg_rt_graph_get_preferred_height:
 * @widget: (in): A #PpgRtGraph.
 * @min_height: (in): A location for the minimum height.
 * @natural_height: (in): A location for the natural height.
 *
 * Determine the preferred width of the #PpgRtGraph.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_rt_graph_get_preferred_height (GtkWidget *widget,
                                   gint      *min_height,
                                   gint      *natural_height)
{
	/*
	 * TODO: Calculate based on font sizing and such.
	 */
	*min_height = *natural_height = 50;
}


/**
 * ppg_rt_graph_size_request:
 * @widget: (in): A #PpgRtGraph.
 * @req: (out): A #GtkRequisition.
 *
 * Handle the "size-request" signal for a GtkWidget on Gtk+-2.0. Defer
 * to the 3.0 version using natural sizing.
 *
 * Returns: None.
 * Side effects: None.
 */
#if !GTK_CHECK_VERSION(2, 91, 0)
static void
ppg_rt_graph_size_request (GtkWidget      *widget,
                           GtkRequisition *req)
{
	gint dummy;

	ENTRY;
	ppg_rt_graph_get_preferred_height(widget, &req->height, &dummy);
	ppg_rt_graph_get_preferred_width(widget, &req->width, &dummy);
	EXIT;
}
#endif


/**
 * ppg_rt_graph_string_accumulator:
 * @graph: (in): A #PpgRtGraph.
 * @return_accu: (out): A #GValue.
 * @handler_return: (in): The current signal handlers return value.
 * @data: (in): user data for the accumulator.
 *
 * Signal accumulator for the "format-value" signal. Stops emission
 * once a string has been returned.
 *
 * Returns: %TRUE if signal emission should continue.
 * Side effects: None.
 */
static gboolean
ppg_rt_graph_string_accumulator (GSignalInvocationHint *ihint,
                                 GValue                *return_accu,
                                 const GValue          *handler_return,
                                 gpointer               data)
{
	if (g_value_get_string(handler_return)) {
		g_value_copy(handler_return, return_accu);
		return FALSE;
	}
	return TRUE;
}


/**
 * ppg_rt_graph_dispose:
 * @object: (in): A #GObject.
 *
 * Dispose callback for @object.  This method releases references held
 * by the #GObject instance.
 *
 * Returns: None.
 * Side effects: Plenty.
 */
static void
ppg_rt_graph_dispose (GObject *object)
{
	PpgRtGraphPrivate *priv = PPG_RT_GRAPH(object)->priv;

	ENTRY;

	ppg_rt_graph_stop(PPG_RT_GRAPH(object));
	ppg_clear_source(&priv->invalidate_handler);
	ppg_clear_source(&priv->adjustment_handler);
	ppg_clear_object(&priv->renderer);

	G_OBJECT_CLASS(ppg_rt_graph_parent_class)->dispose(object);

	EXIT;
}


/**
 * ppg_rt_graph_finalize:
 * @object: (in): A #PpgRtGraph.
 *
 * Finalizer for a #PpgRtGraph instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_rt_graph_finalize (GObject *object)
{
	PpgRtGraphPrivate *priv = PPG_RT_GRAPH(object)->priv;

	ENTRY;

	ppg_clear_pointer(&priv->foreground, cairo_surface_destroy);
	ppg_clear_pointer(&priv->background, cairo_surface_destroy);
	ppg_clear_pointer(&priv->font_desc, pango_font_description_free);

	G_OBJECT_CLASS(ppg_rt_graph_parent_class)->finalize(object);

	EXIT;
}


/**
 * ppg_rt_graph_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
static void
ppg_rt_graph_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
	PpgRtGraph *graph = PPG_RT_GRAPH(object);

	switch (prop_id) {
	case PROP_MAX_LINES:
		ppg_rt_graph_set_max_lines(graph, g_value_get_int(value));
		break;
	case PROP_MIN_LINES:
		ppg_rt_graph_set_min_lines(graph, g_value_get_int(value));
		break;
	case PROP_RENDERER:
		ppg_rt_graph_set_renderer(graph, g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}


/**
 * ppg_rt_graph_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (out): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Get a given #GObject property.
 */
static void
ppg_rt_graph_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
	PpgRtGraph *graph = PPG_RT_GRAPH(object);

	switch (prop_id) {
	case PROP_MAX_LINES:
		g_value_set_int(value, graph->priv->max_lines);
		break;
	case PROP_MIN_LINES:
		g_value_set_int(value, graph->priv->min_lines);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}


/**
 * ppg_rt_graph_class_init:
 * @klass: (in): A #PpgRtGraphClass.
 *
 * Initializes the #PpgRtGraphClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_rt_graph_class_init (PpgRtGraphClass *klass)
{
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->dispose = ppg_rt_graph_dispose;
	object_class->finalize = ppg_rt_graph_finalize;
	object_class->set_property = ppg_rt_graph_set_property;
	object_class->get_property = ppg_rt_graph_get_property;
	g_type_class_add_private(object_class, sizeof(PpgRtGraphPrivate));

	widget_class = GTK_WIDGET_CLASS(klass);
	widget_class->size_allocate = ppg_rt_graph_size_allocate;
#if GTK_CHECK_VERSION(2, 91, 0)
	widget_class->draw = ppg_rt_graph_draw;
	widget_class->get_preferred_height = ppg_rt_graph_get_preferred_height;
	widget_class->get_preferred_width = ppg_rt_graph_get_preferred_width;
#else
	widget_class->expose_event = ppg_rt_graph_expose_event;
	widget_class->size_request = ppg_rt_graph_size_request;
#endif

	klass->format_value = ppg_rt_graph_format_value;

	g_object_class_install_property(object_class,
	                                PROP_RENDERER,
	                                g_param_spec_object("renderer",
	                                                    "renderer",
	                                                    "renderer",
	                                                    PPG_TYPE_RENDERER,
	                                                    G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property(object_class,
	                                PROP_MAX_LINES,
	                                g_param_spec_int("max-lines",
	                                                 "max-lines",
	                                                 "max-lines",
	                                                 0,
	                                                 G_MAXINT,
	                                                 0,
	                                                 G_PARAM_READWRITE));

	g_object_class_install_property(object_class,
	                                PROP_MIN_LINES,
	                                g_param_spec_int("min-lines",
	                                                 "min-lines",
	                                                 "min-lines",
	                                                 0,
	                                                 G_MAXINT,
	                                                 0,
	                                                 G_PARAM_READWRITE));

	signals[FORMAT_VALUE] = g_signal_new("format-value",
	                                     PPG_TYPE_RT_GRAPH,
	                                     G_SIGNAL_RUN_LAST,
	                                     G_STRUCT_OFFSET(PpgRtGraphClass, format_value),
	                                     ppg_rt_graph_string_accumulator,
	                                     NULL,
	                                     ppg_cclosure_marshal_STRING__DOUBLE,
	                                     G_TYPE_STRING,
	                                     1,
	                                     G_TYPE_DOUBLE);

	if (g_getenv("PPG_RT_GRAPH_DEBUG")) {
		PPG_RT_GRAPH_DEBUG = 1;
	}
}


/**
 * ppg_rt_graph_init:
 * @graph: (in): A #PpgRtGraph.
 *
 * Initializes the newly created #PpgRtGraph instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_rt_graph_init (PpgRtGraph *graph)
{
	PpgRtGraphPrivate *priv;

	priv = G_TYPE_INSTANCE_GET_PRIVATE(graph, PPG_TYPE_RT_GRAPH,
	                                   PpgRtGraphPrivate);
	graph->priv = priv;

	priv->n_seconds = 60.0;
	priv->n_buffered = 1.0;

	priv->font_desc = pango_font_description_new();
	pango_font_description_set_family_static(priv->font_desc, "Monospace");
	pango_font_description_set_size(priv->font_desc, PANGO_SCALE * 8);
}

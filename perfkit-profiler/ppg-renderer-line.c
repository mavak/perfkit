/* ppg-renderer-line.c
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

#include <perfkit/perfkit.h>
#include <string.h>

#include "ppg-log.h"
#include "ppg-renderer-line.h"
#include "ppg-task-render.h"
#include "ppg-util.h"


typedef struct
{
	PpgRendererLine *renderer; /* back pointer to renderer */
	gint             id;       /* Unique line identifier */
	GQuark           key;      /* Key within model to get value */
	PkModel         *model;    /* Model containing data points */
	GdkColor         color;    /* gdk_cairo_set_source_color() */
	gboolean         fill;     /* Should we fill the line */
	gdouble          width;    /* cairo_set_line_width() */
	gdouble         *dashes;   /* cairo_set_dash() */
	gdouble          n_dashes; /* cairo_set_dash() */
	guint            handler;  /* notify:: handler id */
	gdouble          end_time; /* End time of last item */
} Line;


struct _PpgRendererLinePrivate
{
	GPtrArray     *lines;
	GtkAdjustment *range;
	gdouble        end_time;
};


static void ppg_renderer_init (PpgRendererIface *iface);


G_DEFINE_TYPE_EXTENDED(PpgRendererLine,
                       ppg_renderer_line,
                       G_TYPE_INITIALLY_UNOWNED,
                       0,
                       G_IMPLEMENT_INTERFACE(PPG_TYPE_RENDERER,
                                             ppg_renderer_init))


static inline gdouble
get_x_for_time (gdouble ratio,
                gdouble x,
                gdouble begin_time,
                gdouble time_)
{
	return x + (ratio * (time_ - begin_time));
}


static inline gdouble
get_y_for_range (gdouble ratio,
                 gdouble y,
                 gdouble begin_value,
                 gdouble value)
{
	return y - (ratio * (value - begin_value));
}


static void
ppg_renderer_line_render (PpgRendererLine *line,
                          PpgTask         *task)
{
	PpgRendererLinePrivate *priv;
	cairo_surface_t *surface;
	PkModelIter iter;
	cairo_t *cr;
	struct {
		gdouble x;
		gdouble y;
	} point;
	gdouble aggregate_time;
	gdouble begin_value;
	gdouble begin_time;
	gdouble current_x;
	gdouble current_y;
	gdouble end_time;
	gdouble first_x;
	gdouble height;
	gdouble value;
	gdouble width;
	gdouble x;
	gdouble x_ratio;
	gdouble y;
	gdouble y2;
	gdouble y_ratio;
	Line *item;
	gint i;

	ENTRY;

	g_return_if_fail(PPG_IS_RENDERER_LINE(line));
	g_return_if_fail(PPG_IS_TASK(task));

	priv = line->priv;

	g_object_get(task,
	             "begin-time", &begin_time,
	             "end-time", &end_time,
	             "height", &height,
	             "surface", &surface,
	             "x", &x,
	             "y", &y,
	             "width", &width,
	             NULL);

	if (begin_time == end_time) {
		EXIT;
	}

	g_assert(surface);
	g_assert(width > 0.0);
	g_assert(height > 0.0);

	y2 = y + height;
	x_ratio = width / (end_time - begin_time);
	y_ratio = height / 100.0;  /* TODO Calculate/get from model */
	begin_value = 0.0;         /* TODO Calculate/get from model */

	/*
	 * To reduce the number of data points we look at, we will ask the
	 * data model to aggregate values to a given range of time. We only
	 * need a max of one data point per pixel.
	 */
	aggregate_time = (end_time - begin_time) / width;

	cr = cairo_create(surface);

	for (i = 0; i < priv->lines->len; i++) {
		item = g_ptr_array_index(priv->lines, i);

		/*
		 * Apply line styling.
		 */
		cairo_set_line_width(cr, item->width);
		cairo_set_dash(cr, item->dashes, item->n_dashes, 0.0);
		gdk_cairo_set_source_color(cr, &item->color);

		if (pk_model_get_iter_for_range(item->model, &iter,
		                                begin_time, end_time,
		                                aggregate_time)) {
			first_x = get_x_for_time(x_ratio, x, begin_time, iter.time);
			do {
				value = pk_model_get_double(item->model, &iter, item->key);
				point.x = get_x_for_time(x_ratio, x, begin_time, iter.time);
				point.y = get_y_for_range(y_ratio, y2, begin_value, value);
				cairo_line_to(cr, point.x, point.y);
			} while (pk_model_iter_next(item->model, &iter));

			/*
			 * Stroke or fill the path of data points.
			 */
			if (item->fill) {
				cairo_get_current_point(cr, &current_x, &current_y);
				cairo_line_to(cr, current_x, y + height);
				cairo_line_to(cr, x, y + height);
				cairo_fill(cr);
			} else {
				cairo_stroke(cr);
			}
		}
	}

	cairo_destroy(cr);

	EXIT;
}


static PpgTask*
ppg_renderer_line_draw (PpgRenderer     *renderer,
                        cairo_surface_t *surface,
                        gdouble          begin_time,
                        gdouble          end_time,
                        gdouble          x,
                        gdouble          y,
                        gdouble          width,
                        gdouble          height)
{
	PpgRendererLinePrivate *priv;
	PpgRendererLine *line = (PpgRendererLine *)renderer;
	PpgTask *task;

	ENTRY;

	g_return_val_if_fail(PPG_IS_RENDERER_LINE(line), NULL);

	priv = line->priv;

	task = g_object_new(PPG_TYPE_TASK_RENDER,
	                    "begin-time", begin_time,
	                    "end-time", end_time,
	                    "height", height,
	                    "surface", surface,
	                    "width", width,
	                    "x", x,
	                    "y", y,
	                    NULL);
	g_signal_connect_object(task, "render",
	                        G_CALLBACK(ppg_renderer_line_render),
	                        renderer,
	                        G_CONNECT_SWAPPED);
	RETURN(task);
}


static GtkAdjustment*
ppg_renderer_line_get_adjustment (PpgRenderer *renderer)
{
	PpgRendererLine *line = (PpgRendererLine *)renderer;
	g_return_val_if_fail(PPG_IS_RENDERER_LINE(line), NULL);
	return line->priv->range;
}


static void
ppg_renderer_line_notify_end_time (PkModel    *model,
                                   GParamSpec *pspec,
                                   Line       *item)
{
	PpgRendererLinePrivate *priv;
	gdouble last_end_time;

	g_return_if_fail(PK_IS_MODEL(model));
	g_return_if_fail(item != NULL);
	g_return_if_fail(PPG_IS_RENDERER_LINE(item->renderer));

	priv = item->renderer->priv;

	last_end_time = item->end_time;
	item->end_time = pk_model_get_end_time(model);
	ppg_renderer_emit_invalidate(PPG_RENDERER(item->renderer),
	                             last_end_time, item->end_time);
}


gint
ppg_renderer_line_append (PpgRendererLine *line,
                          PkModel         *model,
                          GQuark           key)
{
	static gint sequence = 0;
	PpgRendererLinePrivate *priv;
	PpgRenderer *renderer = (PpgRenderer *)line;
	Line *new_line;

	g_return_val_if_fail(PPG_IS_RENDERER_LINE(line), 0);
	g_return_val_if_fail(PK_IS_MODEL(model), 0);
	g_return_val_if_fail(key != 0, 0);

	priv = line->priv;

	new_line = g_new0(Line, 1);
	new_line->renderer = line;
	new_line->id = ++sequence;
	new_line->key = key;
	new_line->width = 1.0;
	new_line->model = g_object_ref(model);
	new_line->handler =
		g_signal_connect(model, "notify::end-time",
		                 G_CALLBACK(ppg_renderer_line_notify_end_time),
		                 new_line);

	g_ptr_array_add(priv->lines, new_line);
	ppg_renderer_emit_invalidate(renderer, 0.0, 0.0);

	return new_line->id;
}


void
ppg_renderer_line_remove (PpgRendererLine *line,
                          gint             identifier)
{
	PpgRendererLinePrivate *priv;
	PpgRenderer *renderer = (PpgRenderer *)line;
	Line *item;
	gint i;

	g_return_if_fail(PPG_IS_RENDERER_LINE(line));
	g_return_if_fail(PPG_IS_RENDERER(renderer));
	g_return_if_fail(identifier > 0);

	priv = line->priv;

	for (i = 0; i < priv->lines->len; i++) {
		item = g_ptr_array_index(priv->lines, i);
		if (item->id == identifier) {
			g_signal_handler_disconnect(item->model, item->handler);
			item->handler = 0;
			ppg_clear_object(&item->model);
			ppg_clear_pointer(&item->dashes, g_free);
			g_ptr_array_remove_index(priv->lines, i);
			ppg_renderer_emit_invalidate(renderer, 0.0, 0.0);
			break;
		}
	}
}


void
ppg_renderer_line_set_styling (PpgRendererLine *line,
                               gint             identifier,
                               const GdkColor  *color,
                               gdouble          line_width,
                               gdouble         *dashes,
                               gint             n_dashes)
{
	PpgRendererLinePrivate *priv;
	PpgRenderer *renderer = (PpgRenderer *)line;
	Line *item;
	gint i;

	g_return_if_fail(PPG_IS_RENDERER_LINE(line));
	g_return_if_fail(color != NULL);
	g_return_if_fail(n_dashes >= 0);

	priv = line->priv;

	for (i = 0; i < priv->lines->len; i++) {
		item = g_ptr_array_index(priv->lines, i);
		if (item->id == identifier) {
			item->color = *color;
			item->width = line_width;
			ppg_clear_pointer(&item->dashes, g_free);
			item->n_dashes = 0;
			if (dashes) {
				item->dashes = g_new(gdouble, n_dashes);
				item->n_dashes = n_dashes;
				memcpy(item->dashes, dashes, sizeof(gdouble) * n_dashes);
			}
			break;
		}
	}

	ppg_renderer_emit_invalidate(renderer, 0.0, 0.0);
}


/**
 * ppg_renderer_line_dispose:
 * @object: (in): A #GObject.
 *
 * Dispose callback for @object.  This method releases references held
 * by the #GObject instance.
 *
 * Returns: None.
 * Side effects: Plenty.
 */
static void
ppg_renderer_line_dispose (GObject *object)
{
	PpgRendererLinePrivate *priv = PPG_RENDERER_LINE(object)->priv;
	PpgRendererLine *line = (PpgRendererLine *)object;

	ENTRY;

	while (priv->lines->len) {
		ppg_renderer_line_remove(line, 0);
	}

	G_OBJECT_CLASS(ppg_renderer_line_parent_class)->dispose(object);

	EXIT;
}


/**
 * ppg_renderer_line_finalize:
 * @object: (in): A #PpgRendererLine.
 *
 * Finalizer for a #PpgRendererLine instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_renderer_line_finalize (GObject *object)
{
	PpgRendererLinePrivate *priv = PPG_RENDERER_LINE(object)->priv;

	ENTRY;

	g_ptr_array_free(priv->lines, TRUE);

	G_OBJECT_CLASS(ppg_renderer_line_parent_class)->finalize(object);

	EXIT;
}


/**
 * ppg_renderer_line_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
static void
ppg_renderer_line_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}


/**
 * ppg_renderer_line_class_init:
 * @klass: (in): A #PpgRendererLineClass.
 *
 * Initializes the #PpgRendererLineClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_renderer_line_class_init (PpgRendererLineClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->dispose = ppg_renderer_line_dispose;
	object_class->finalize = ppg_renderer_line_finalize;
	object_class->set_property = ppg_renderer_line_set_property;
	g_type_class_add_private(object_class, sizeof(PpgRendererLinePrivate));
}


/**
 * ppg_renderer_line_init:
 * @line: (in): A #PpgRendererLine.
 *
 * Initializes the newly created #PpgRendererLine instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_renderer_line_init (PpgRendererLine *line)
{
	line->priv = G_TYPE_INSTANCE_GET_PRIVATE(line, PPG_TYPE_RENDERER_LINE,
	                                         PpgRendererLinePrivate);
	line->priv->lines = g_ptr_array_new();
	line->priv->range = g_object_new(GTK_TYPE_ADJUSTMENT,
	                                 "lower", 0.0,
	                                 "upper", 100.0,
	                                 NULL);
}


static void
ppg_renderer_init (PpgRendererIface *iface)
{
	iface->draw = ppg_renderer_line_draw;
	iface->get_adjustment = ppg_renderer_line_get_adjustment;
}

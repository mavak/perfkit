/* ppg-visualizer.h
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

#ifndef PPG_VISUALIZER_H
#define PPG_VISUALIZER_H

#include <goocanvas.h>
#include <gtk/gtk.h>

#include "ppg-task.h"

G_BEGIN_DECLS

#define PPG_TYPE_VISUALIZER            (ppg_visualizer_get_type())
#define PPG_VISUALIZER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_VISUALIZER, PpgVisualizer))
#define PPG_VISUALIZER_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_VISUALIZER, PpgVisualizer const))
#define PPG_VISUALIZER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_VISUALIZER, PpgVisualizerClass))
#define PPG_IS_VISUALIZER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_VISUALIZER))
#define PPG_IS_VISUALIZER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_VISUALIZER))
#define PPG_VISUALIZER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_VISUALIZER, PpgVisualizerClass))

typedef struct _PpgVisualizer        PpgVisualizer;
typedef struct _PpgVisualizerClass   PpgVisualizerClass;
typedef struct _PpgVisualizerPrivate PpgVisualizerPrivate;
typedef struct _PpgVisualizerEntry   PpgVisualizerEntry;

typedef PpgVisualizer* (*PpgVisualizerFactory) (PpgVisualizerEntry  *entry,
                                                gpointer             user_data,
                                                GError             **error);

struct _PpgVisualizer
{
	GooCanvasImage parent;

	/*< private >*/
	PpgVisualizerPrivate *priv;
};

struct _PpgVisualizerClass
{
	GooCanvasImageClass parent_class;

	PpgTask* (*draw) (PpgVisualizer   *visualizer,
	                  cairo_surface_t *surface,
	                  gdouble          begin_time,
	                  gdouble          end_time,
	                  gdouble          x,
	                  gdouble          y,
	                  gdouble          width,
	                  gdouble          height);
};

struct _PpgVisualizerEntry
{
	const gchar *name;
	const gchar *title;
	const gchar *icon_name;
	GCallback    callback;

	/*< private >*/
	gpointer     user_data;
};

gboolean      ppg_visualizer_get_is_important     (PpgVisualizer *visualizer);
gdouble       ppg_visualizer_get_natural_height   (PpgVisualizer *visualizer);
GType         ppg_visualizer_get_type             (void) G_GNUC_CONST;
void          ppg_visualizer_queue_draw           (PpgVisualizer *visualizer);
void          ppg_visualizer_queue_draw_time_span (PpgVisualizer *visualizer,
                                                   gdouble        begin_time,
                                                   gdouble        end_time);
void          ppg_visualizer_set_begin_time       (PpgVisualizer *visualizer,
                                                   gdouble        begin_time);
void          ppg_visualizer_set_end_time         (PpgVisualizer *visualizer,
                                                   gdouble        end_time);
void          ppg_visualizer_set_is_important     (PpgVisualizer *visualizer,
                                                   gboolean       important);
void          ppg_visualizer_set_time             (PpgVisualizer *visualizer,
                                                   gdouble        begin_time,
                                                   gdouble        end_time);
void          ppg_visualizer_thaw                 (PpgVisualizer *visualizer);
void          ppg_visualizer_freeze               (PpgVisualizer *visualizer);
const gchar * ppg_visualizer_get_name             (PpgVisualizer *visualizer);

G_END_DECLS

#endif /* PPG_VISUALIZER_H */

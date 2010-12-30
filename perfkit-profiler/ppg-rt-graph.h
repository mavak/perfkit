/* ppg-rt-graph.h
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

#ifndef PPG_RT_GRAPH_H
#define PPG_RT_GRAPH_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PPG_TYPE_RT_GRAPH            (ppg_rt_graph_get_type())
#define PPG_RT_GRAPH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_RT_GRAPH, PpgRtGraph))
#define PPG_RT_GRAPH_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_RT_GRAPH, PpgRtGraph const))
#define PPG_RT_GRAPH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_RT_GRAPH, PpgRtGraphClass))
#define PPG_IS_RT_GRAPH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_RT_GRAPH))
#define PPG_IS_RT_GRAPH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_RT_GRAPH))
#define PPG_RT_GRAPH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_RT_GRAPH, PpgRtGraphClass))

typedef struct _PpgRtGraph        PpgRtGraph;
typedef struct _PpgRtGraphClass   PpgRtGraphClass;
typedef struct _PpgRtGraphPrivate PpgRtGraphPrivate;
typedef enum   _PpgRtGraphFlags   PpgRtGraphFlags;

enum _PpgRtGraphFlags
{
	PPG_RT_GRAPH_NONE    = 0,
	PPG_RT_GRAPH_PERCENT = 1 << 0,
};

struct _PpgRtGraph
{
	GtkDrawingArea parent;

	/*< private >*/
	PpgRtGraphPrivate *priv;
};

struct _PpgRtGraphClass
{
	GtkDrawingAreaClass parent_class;

	gchar* (*format_value) (PpgRtGraph *graph,
	                        gdouble     value);
};

GType ppg_rt_graph_get_type  (void) G_GNUC_CONST;
void  ppg_rt_graph_start     (PpgRtGraph      *graph);
void  ppg_rt_graph_stop      (PpgRtGraph      *graph);
void  ppg_rt_graph_set_flags (PpgRtGraph      *graph,
                              PpgRtGraphFlags  flags);

G_END_DECLS

#endif /* PPG_RT_GRAPH_H */

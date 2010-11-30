/* ppg-time-span-visualizer.h
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

#ifndef PPG_TIME_SPAN_VISUALIZER_H
#define PPG_TIME_SPAN_VISUALIZER_H

#include "ppg-visualizer.h"

G_BEGIN_DECLS

#define PPG_TYPE_TIME_SPAN_VISUALIZER            (ppg_time_span_visualizer_get_type())
#define PPG_TIME_SPAN_VISUALIZER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_TIME_SPAN_VISUALIZER, PpgTimeSpanVisualizer))
#define PPG_TIME_SPAN_VISUALIZER_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_TIME_SPAN_VISUALIZER, PpgTimeSpanVisualizer const))
#define PPG_TIME_SPAN_VISUALIZER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_TIME_SPAN_VISUALIZER, PpgTimeSpanVisualizerClass))
#define PPG_IS_TIME_SPAN_VISUALIZER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_TIME_SPAN_VISUALIZER))
#define PPG_IS_TIME_SPAN_VISUALIZER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_TIME_SPAN_VISUALIZER))
#define PPG_TIME_SPAN_VISUALIZER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_TIME_SPAN_VISUALIZER, PpgTimeSpanVisualizerClass))

typedef struct _PpgTimeSpanVisualizer        PpgTimeSpanVisualizer;
typedef struct _PpgTimeSpanVisualizerClass   PpgTimeSpanVisualizerClass;
typedef struct _PpgTimeSpanVisualizerPrivate PpgTimeSpanVisualizerPrivate;

struct _PpgTimeSpanVisualizer
{
	PpgVisualizer parent;

	/*< private >*/
	PpgTimeSpanVisualizerPrivate *priv;
};

struct _PpgTimeSpanVisualizerClass
{
	PpgVisualizerClass parent_class;
};

GType ppg_time_span_visualizer_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* PPG_TIME_SPAN_VISUALIZER_H */

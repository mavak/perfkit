/* ppg-time-visualizer.h
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

#ifndef __PPG_TIME_VISUALIZER_H__
#define __PPG_TIME_VISUALIZER_H__

#include <gdk/gdk.h>

#include "ppg-model.h"
#include "ppg-visualizer.h"

G_BEGIN_DECLS

#define PPG_TYPE_TIME_VISUALIZER            (ppg_time_visualizer_get_type())
#define PPG_TIME_VISUALIZER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_TIME_VISUALIZER, PpgTimeVisualizer))
#define PPG_TIME_VISUALIZER_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_TIME_VISUALIZER, PpgTimeVisualizer const))
#define PPG_TIME_VISUALIZER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_TIME_VISUALIZER, PpgTimeVisualizerClass))
#define PPG_IS_TIME_VISUALIZER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_TIME_VISUALIZER))
#define PPG_IS_TIME_VISUALIZER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_TIME_VISUALIZER))
#define PPG_TIME_VISUALIZER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_TIME_VISUALIZER, PpgTimeVisualizerClass))

typedef struct _PpgTimeVisualizer        PpgTimeVisualizer;
typedef struct _PpgTimeVisualizerClass   PpgTimeVisualizerClass;
typedef struct _PpgTimeVisualizerPrivate PpgTimeVisualizerPrivate;

struct _PpgTimeVisualizer
{
	PpgVisualizer parent;

	/*< private >*/
	PpgTimeVisualizerPrivate *priv;
};

struct _PpgTimeVisualizerClass
{
	PpgVisualizerClass parent_class;
};

GType ppg_time_visualizer_get_type  (void) G_GNUC_CONST;
void  ppg_time_visualizer_set_model (PpgTimeVisualizer *visualizer,
                                     PpgModel          *model);

G_END_DECLS

#endif /* __PPG_TIME_VISUALIZER_H__ */

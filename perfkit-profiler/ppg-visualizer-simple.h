/* ppg-visualizer-simple.h
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

#ifndef PPG_VISUALIZER_SIMPLE_H
#define PPG_VISUALIZER_SIMPLE_H

#include "ppg-visualizer.h"

G_BEGIN_DECLS

#define PPG_TYPE_VISUALIZER_SIMPLE            (ppg_visualizer_simple_get_type())
#define PPG_VISUALIZER_SIMPLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_VISUALIZER_SIMPLE, PpgVisualizerSimple))
#define PPG_VISUALIZER_SIMPLE_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_VISUALIZER_SIMPLE, PpgVisualizerSimple const))
#define PPG_VISUALIZER_SIMPLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_VISUALIZER_SIMPLE, PpgVisualizerSimpleClass))
#define PPG_IS_VISUALIZER_SIMPLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_VISUALIZER_SIMPLE))
#define PPG_IS_VISUALIZER_SIMPLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_VISUALIZER_SIMPLE))
#define PPG_VISUALIZER_SIMPLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_VISUALIZER_SIMPLE, PpgVisualizerSimpleClass))

typedef struct _PpgVisualizerSimple        PpgVisualizerSimple;
typedef struct _PpgVisualizerSimpleClass   PpgVisualizerSimpleClass;
typedef struct _PpgVisualizerSimplePrivate PpgVisualizerSimplePrivate;

struct _PpgVisualizerSimple
{
	PpgVisualizer parent;

	/*< private >*/
	PpgVisualizerSimplePrivate *priv;
};

struct _PpgVisualizerSimpleClass
{
	PpgVisualizerClass parent_class;
};

GType ppg_visualizer_simple_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* PPG_VISUALIZER_SIMPLE_H */

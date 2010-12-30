/* ppg-renderer.h
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

#ifndef PPG_RENDERER_H
#define PPG_RENDERER_H

#include <gtk/gtk.h>

#include "ppg-task.h"

G_BEGIN_DECLS

#define PPG_TYPE_RENDERER             (ppg_renderer_get_type())
#define PPG_RENDERER(o)               (G_TYPE_CHECK_INSTANCE_CAST((o),    PPG_TYPE_RENDERER, PpgRenderer))
#define PPG_IS_RENDERER(o)            (G_TYPE_CHECK_INSTANCE_TYPE((o),    PPG_TYPE_RENDERER))
#define PPG_RENDERER_GET_INTERFACE(o) (G_TYPE_INSTANCE_GET_INTERFACE((o), PPG_TYPE_RENDERER, PpgRendererIface))

typedef struct _PpgRenderer      PpgRenderer;
typedef struct _PpgRendererIface PpgRendererIface;

struct _PpgRendererIface
{
	GTypeInterface parent;

	/* interface methods */
	PpgTask*       (*draw)           (PpgRenderer     *renderer,
	                                  cairo_surface_t *surface,
	                                  gdouble          begin_time,
	                                  gdouble          end_time,
	                                  gdouble          x,
	                                  gdouble          y,
	                                  gdouble          width,
	                                  gdouble          height);
	GtkAdjustment* (*get_adjustment) (PpgRenderer     *renderer);
};

GType          ppg_renderer_get_type        (void) G_GNUC_CONST;
PpgTask*       ppg_renderer_draw            (PpgRenderer     *renderer,
                                             cairo_surface_t *surface,
                                             gdouble          begin_time,
                                             gdouble          end_time,
                                             gdouble          x,
                                             gdouble          y,
                                             gdouble          width,
                                             gdouble          height);
void           ppg_renderer_emit_invalidate (PpgRenderer     *renderer,
                                             gdouble          begin_time,
                                             gdouble          end_time);
GtkAdjustment* ppg_renderer_get_adjustment  (PpgRenderer     *renderer);

G_END_DECLS

#endif /* PPG_RENDERER_H */

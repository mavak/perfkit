/* ppg-renderer-line.h
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

#ifndef PPG_RENDERER_LINE_H
#define PPG_RENDERER_LINE_H

#include <perfkit/perfkit.h>

#include "ppg-renderer.h"

G_BEGIN_DECLS

#define PPG_TYPE_RENDERER_LINE            (ppg_renderer_line_get_type())
#define PPG_RENDERER_LINE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_RENDERER_LINE, PpgRendererLine))
#define PPG_RENDERER_LINE_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_RENDERER_LINE, PpgRendererLine const))
#define PPG_RENDERER_LINE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_RENDERER_LINE, PpgRendererLineClass))
#define PPG_IS_RENDERER_LINE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_RENDERER_LINE))
#define PPG_IS_RENDERER_LINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_RENDERER_LINE))
#define PPG_RENDERER_LINE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_RENDERER_LINE, PpgRendererLineClass))

typedef struct _PpgRendererLine        PpgRendererLine;
typedef struct _PpgRendererLineClass   PpgRendererLineClass;
typedef struct _PpgRendererLinePrivate PpgRendererLinePrivate;

struct _PpgRendererLine
{
	GInitiallyUnowned parent;

	/*< private >*/
	PpgRendererLinePrivate *priv;
};

struct _PpgRendererLineClass
{
	GInitiallyUnownedClass parent_class;
};

GType ppg_renderer_line_get_type    (void) G_GNUC_CONST;
gint  ppg_renderer_line_append      (PpgRendererLine *line,
                                     PkModel         *model,
                                     GQuark           key);
void  ppg_renderer_line_remove      (PpgRendererLine *line,
                                     gint             identifier);
void  ppg_renderer_line_set_styling (PpgRendererLine *line,
                                     gint             identifier,
                                     const GdkColor  *color,
                                     gdouble          line_width,
                                     gdouble         *dashes,
                                     gint             n_dashes);

G_END_DECLS

#endif /* PPG_RENDERER_LINE_H */

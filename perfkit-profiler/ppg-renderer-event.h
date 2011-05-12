/* ppg-renderer-event.h
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

#ifndef PPG_RENDERER_EVENT_H
#define PPG_RENDERER_EVENT_H

#include <perfkit/perfkit.h>

#include "ppg-renderer.h"

G_BEGIN_DECLS

#define PPG_TYPE_RENDERER_EVENT            (ppg_renderer_event_get_type())
#define PPG_RENDERER_EVENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_RENDERER_EVENT, PpgRendererEvent))
#define PPG_RENDERER_EVENT_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_RENDERER_EVENT, PpgRendererEvent const))
#define PPG_RENDERER_EVENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_RENDERER_EVENT, PpgRendererEventClass))
#define PPG_IS_RENDERER_EVENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_RENDERER_EVENT))
#define PPG_IS_RENDERER_EVENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_RENDERER_EVENT))
#define PPG_RENDERER_EVENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_RENDERER_EVENT, PpgRendererEventClass))

typedef struct _PpgRendererEvent        PpgRendererEvent;
typedef struct _PpgRendererEventClass   PpgRendererEventClass;
typedef struct _PpgRendererEventPrivate PpgRendererEventPrivate;

struct _PpgRendererEvent
{
	GInitiallyUnowned parent;

	/*< private >*/
	PpgRendererEventPrivate *priv;
};

struct _PpgRendererEventClass
{
	GInitiallyUnownedClass parent_class;
};

GType ppg_renderer_event_get_type (void) G_GNUC_CONST;
gint  ppg_renderer_event_append   (PpgRendererEvent *event,
                                   PkModel          *model,
                                   GQuark            key);
void  ppg_renderer_event_remove   (PpgRendererEvent *event,
                                   gint              identifier);

G_END_DECLS

#endif /* PPG_RENDERER_EVENT_H */

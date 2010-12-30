/* ppg-task-render.h
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

#ifndef PPG_TASK_RENDER_H
#define PPG_TASK_RENDER_H

#include "ppg-task.h"

G_BEGIN_DECLS

#define PPG_TYPE_TASK_RENDER            (ppg_task_render_get_type())
#define PPG_TASK_RENDER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_TASK_RENDER, PpgTaskRender))
#define PPG_TASK_RENDER_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_TASK_RENDER, PpgTaskRender const))
#define PPG_TASK_RENDER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_TASK_RENDER, PpgTaskRenderClass))
#define PPG_IS_TASK_RENDER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_TASK_RENDER))
#define PPG_IS_TASK_RENDER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_TASK_RENDER))
#define PPG_TASK_RENDER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_TASK_RENDER, PpgTaskRenderClass))

typedef struct _PpgTaskRender        PpgTaskRender;
typedef struct _PpgTaskRenderClass   PpgTaskRenderClass;
typedef struct _PpgTaskRenderPrivate PpgTaskRenderPrivate;

struct _PpgTaskRender
{
	PpgTask parent;

	/*< private >*/
	PpgTaskRenderPrivate *priv;
};

struct _PpgTaskRenderClass
{
	PpgTaskClass parent_class;

	void (*render) (PpgTaskRender *render);
};

GType ppg_task_render_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* PPG_TASK_RENDER_H */

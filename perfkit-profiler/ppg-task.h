/* ppg-task.h
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

#ifndef PPG_TASK_H
#define PPG_TASK_H

#include <glib-object.h>

G_BEGIN_DECLS

#define PPG_TYPE_TASK            (ppg_task_get_type())
#define PPG_TYPE_TASK_STATE      (ppg_task_state_get_type())
#define PPG_TASK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_TASK, PpgTask))
#define PPG_TASK_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_TASK, PpgTask const))
#define PPG_TASK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_TASK, PpgTaskClass))
#define PPG_IS_TASK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_TASK))
#define PPG_IS_TASK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_TASK))
#define PPG_TASK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_TASK, PpgTaskClass))
#define PPG_TASK_ERROR           (ppg_task_error_quark())

typedef struct _PpgTask        PpgTask;
typedef struct _PpgTaskClass   PpgTaskClass;
typedef struct _PpgTaskPrivate PpgTaskPrivate;
typedef enum   _PpgTaskState   PpgTaskState;
typedef enum   _PpgTaskError   PpgTaskError;

enum _PpgTaskError
{
	PPG_TASK_ERROR_CANCELLED,
};

enum _PpgTaskState
{
	PPG_TASK_INITIAL = 0,
	PPG_TASK_RUNNING = 1 << 0,
	PPG_TASK_SUCCESS = 1 << 1,
	PPG_TASK_FAILED  = 1 << 2,
	PPG_TASK_FINISHED_MASK = PPG_TASK_SUCCESS | PPG_TASK_FAILED,
};

struct _PpgTask
{
	GInitiallyUnowned parent;

	/*< private >*/
	PpgTaskPrivate *priv;
};

struct _PpgTaskClass
{
	GInitiallyUnownedClass parent_class;

	void (*run)      (PpgTask *task);
	void (*schedule) (PpgTask *task);
};

void   ppg_task_cancel         (PpgTask      *task);
GQuark ppg_task_error_quark    (void) G_GNUC_CONST;
GType  ppg_task_get_type       (void) G_GNUC_CONST;
void   ppg_task_fail           (PpgTask      *task,
                                const GError *error);
void   ppg_task_finish         (PpgTask      *task);
void   ppg_task_run            (PpgTask      *task);
void   ppg_task_schedule       (PpgTask      *task);
GType  ppg_task_state_get_type (void) G_GNUC_CONST;
void   ppg_task_use_idle       (PpgTask      *task,
                                gboolean      use_idle);

G_END_DECLS

#endif /* PPG_TASK_H */

/* ppg-process-menu.h
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

#ifndef __PPG_PROCESS_MENU_H__
#define __PPG_PROCESS_MENU_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PPG_TYPE_PROCESS_MENU            (ppg_process_menu_get_type())
#define PPG_PROCESS_MENU(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_PROCESS_MENU, PpgProcessMenu))
#define PPG_PROCESS_MENU_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_PROCESS_MENU, PpgProcessMenu const))
#define PPG_PROCESS_MENU_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_PROCESS_MENU, PpgProcessMenuClass))
#define PPG_IS_PROCESS_MENU(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_PROCESS_MENU))
#define PPG_IS_PROCESS_MENU_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_PROCESS_MENU))
#define PPG_PROCESS_MENU_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_PROCESS_MENU, PpgProcessMenuClass))

typedef struct _PpgProcessMenu        PpgProcessMenu;
typedef struct _PpgProcessMenuClass   PpgProcessMenuClass;
typedef struct _PpgProcessMenuPrivate PpgProcessMenuPrivate;

struct _PpgProcessMenu
{
	GtkMenu parent;

	/*< private >*/
	PpgProcessMenuPrivate *priv;
};

struct _PpgProcessMenuClass
{
	GtkMenuClass parent_class;
};

GType ppg_process_menu_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __PPG_PROCESS_MENU_H__ */

/* ppg-hud-window.h
 *
 * Copyright (C) 2011 Christian Hergert <chris@dronelabs.com>
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

#ifndef PPG_HUD_WINDOW_H
#define PPG_HUD_WINDOW_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PPG_TYPE_HUD_WINDOW            (ppg_hud_window_get_type())
#define PPG_HUD_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_HUD_WINDOW, PpgHudWindow))
#define PPG_HUD_WINDOW_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_HUD_WINDOW, PpgHudWindow const))
#define PPG_HUD_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_HUD_WINDOW, PpgHudWindowClass))
#define PPG_IS_HUD_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_HUD_WINDOW))
#define PPG_IS_HUD_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_HUD_WINDOW))
#define PPG_HUD_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_HUD_WINDOW, PpgHudWindowClass))

typedef struct _PpgHudWindow        PpgHudWindow;
typedef struct _PpgHudWindowClass   PpgHudWindowClass;
typedef struct _PpgHudWindowPrivate PpgHudWindowPrivate;

struct _PpgHudWindow
{
	GtkWindow parent;

	/*< private >*/
	PpgHudWindowPrivate *priv;
};

struct _PpgHudWindowClass
{
	GtkWindowClass parent_class;
};

GType ppg_hud_window_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* PPG_HUD_WINDOW_H */

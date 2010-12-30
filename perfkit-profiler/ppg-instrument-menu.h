/* ppg-instrument-menu.h
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

#ifndef PPG_INSTRUMENT_MENU_H
#define PPG_INSTRUMENT_MENU_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PPG_TYPE_INSTRUMENT_MENU            (ppg_instrument_menu_get_type())
#define PPG_INSTRUMENT_MENU(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_INSTRUMENT_MENU, PpgInstrumentMenu))
#define PPG_INSTRUMENT_MENU_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_INSTRUMENT_MENU, PpgInstrumentMenu const))
#define PPG_INSTRUMENT_MENU_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_INSTRUMENT_MENU, PpgInstrumentMenuClass))
#define PPG_IS_INSTRUMENT_MENU(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_INSTRUMENT_MENU))
#define PPG_IS_INSTRUMENT_MENU_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_INSTRUMENT_MENU))
#define PPG_INSTRUMENT_MENU_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_INSTRUMENT_MENU, PpgInstrumentMenuClass))

typedef struct _PpgInstrumentMenu        PpgInstrumentMenu;
typedef struct _PpgInstrumentMenuClass   PpgInstrumentMenuClass;
typedef struct _PpgInstrumentMenuPrivate PpgInstrumentMenuPrivate;

struct _PpgInstrumentMenu
{
	GtkMenu parent;

	/*< private >*/
	PpgInstrumentMenuPrivate *priv;
};

struct _PpgInstrumentMenuClass
{
	GtkMenuClass parent_class;
};

GType ppg_instrument_menu_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* PPG_INSTRUMENT_MENU_H */

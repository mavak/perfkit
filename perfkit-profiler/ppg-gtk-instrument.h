/* ppg-gtk-instrument.h
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

#ifndef PPG_GTK_INSTRUMENT_H
#define PPG_GTK_INSTRUMENT_H

#include "ppg-instrument.h"

G_BEGIN_DECLS

#define PPG_TYPE_GTK_INSTRUMENT            (ppg_gtk_instrument_get_type())
#define PPG_GTK_INSTRUMENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_GTK_INSTRUMENT, PpgGtkInstrument))
#define PPG_GTK_INSTRUMENT_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_GTK_INSTRUMENT, PpgGtkInstrument const))
#define PPG_GTK_INSTRUMENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_GTK_INSTRUMENT, PpgGtkInstrumentClass))
#define PPG_IS_GTK_INSTRUMENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_GTK_INSTRUMENT))
#define PPG_IS_GTK_INSTRUMENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_GTK_INSTRUMENT))
#define PPG_GTK_INSTRUMENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_GTK_INSTRUMENT, PpgGtkInstrumentClass))

typedef struct _PpgGtkInstrument        PpgGtkInstrument;
typedef struct _PpgGtkInstrumentClass   PpgGtkInstrumentClass;
typedef struct _PpgGtkInstrumentPrivate PpgGtkInstrumentPrivate;

struct _PpgGtkInstrument
{
	PpgInstrument parent;

	/*< private >*/
	PpgGtkInstrumentPrivate *priv;
};

struct _PpgGtkInstrumentClass
{
	PpgInstrumentClass parent_class;
};

GType ppg_gtk_instrument_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* PPG_GTK_INSTRUMENT_H */

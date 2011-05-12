/* ppg-gdk-instrument.h
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

#ifndef PPG_GDK_INSTRUMENT_H
#define PPG_GDK_INSTRUMENT_H

#include "ppg-instrument.h"

G_BEGIN_DECLS

#define PPG_TYPE_GDK_INSTRUMENT            (ppg_gdk_instrument_get_type())
#define PPG_GDK_INSTRUMENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_GDK_INSTRUMENT, PpgGdkInstrument))
#define PPG_GDK_INSTRUMENT_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_GDK_INSTRUMENT, PpgGdkInstrument const))
#define PPG_GDK_INSTRUMENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_GDK_INSTRUMENT, PpgGdkInstrumentClass))
#define PPG_IS_GDK_INSTRUMENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_GDK_INSTRUMENT))
#define PPG_IS_GDK_INSTRUMENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_GDK_INSTRUMENT))
#define PPG_GDK_INSTRUMENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_GDK_INSTRUMENT, PpgGdkInstrumentClass))

typedef struct _PpgGdkInstrument        PpgGdkInstrument;
typedef struct _PpgGdkInstrumentClass   PpgGdkInstrumentClass;
typedef struct _PpgGdkInstrumentPrivate PpgGdkInstrumentPrivate;

struct _PpgGdkInstrument
{
	PpgInstrument parent;

	/*< private >*/
	PpgGdkInstrumentPrivate *priv;
};

struct _PpgGdkInstrumentClass
{
	PpgInstrumentClass parent_class;
};

GType ppg_gdk_instrument_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* PPG_GDK_INSTRUMENT_H */

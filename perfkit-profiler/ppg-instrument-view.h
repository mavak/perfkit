/* ppg-instrument-view.h
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

#ifndef PPG_INSTRUMENT_VIEW_H
#define PPG_INSTRUMENT_VIEW_H

#include <gtk/gtk.h>
#include <goocanvas.h>

#include "ppg-instrument.h"

G_BEGIN_DECLS

#define PPG_TYPE_INSTRUMENT_VIEW            (ppg_instrument_view_get_type())
#define PPG_INSTRUMENT_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_INSTRUMENT_VIEW, PpgInstrumentView))
#define PPG_INSTRUMENT_VIEW_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_INSTRUMENT_VIEW, PpgInstrumentView const))
#define PPG_INSTRUMENT_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_INSTRUMENT_VIEW, PpgInstrumentViewClass))
#define PPG_IS_INSTRUMENT_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_INSTRUMENT_VIEW))
#define PPG_IS_INSTRUMENT_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_INSTRUMENT_VIEW))
#define PPG_INSTRUMENT_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_INSTRUMENT_VIEW, PpgInstrumentViewClass))

typedef struct _PpgInstrumentView        PpgInstrumentView;
typedef struct _PpgInstrumentViewClass   PpgInstrumentViewClass;
typedef struct _PpgInstrumentViewPrivate PpgInstrumentViewPrivate;

struct _PpgInstrumentView
{
	GooCanvasTable parent;

	/*< private >*/
	PpgInstrumentViewPrivate *priv;
};

struct _PpgInstrumentViewClass
{
	GooCanvasTableClass parent_class;
};

GType          ppg_instrument_view_get_type       (void) G_GNUC_CONST;
void           ppg_instrument_view_set_style      (PpgInstrumentView *view,
                                                   GtkStyle          *style);
void           ppg_instrument_view_set_state      (PpgInstrumentView *view,
                                                   GtkStateType       state);
GtkStateType   ppg_instrument_view_get_state      (PpgInstrumentView *view);
void           ppg_instrument_view_set_time       (PpgInstrumentView *view,
                                                   gdouble            begin_time,
                                                   gdouble            end_time);
void           ppg_instrument_view_zoom_in        (PpgInstrumentView *view);
void           ppg_instrument_view_zoom_out       (PpgInstrumentView *view);
PpgInstrument* ppg_instrument_view_get_instrument (PpgInstrumentView *view);

G_END_DECLS

#endif /* PPG_INSTRUMENT_VIEW_H */

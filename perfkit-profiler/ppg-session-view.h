/* ppg-session-view.h
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

#ifndef PPG_SESSION_VIEW_H
#define PPG_SESSION_VIEW_H

#include <gtk/gtk.h>

#include "ppg-instrument-view.h"
#include "ppg-instrument.h"
#include "ppg-session.h"

G_BEGIN_DECLS

#define PPG_TYPE_SESSION_VIEW            (ppg_session_view_get_type())
#define PPG_SESSION_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_SESSION_VIEW, PpgSessionView))
#define PPG_SESSION_VIEW_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_SESSION_VIEW, PpgSessionView const))
#define PPG_SESSION_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_SESSION_VIEW, PpgSessionViewClass))
#define PPG_IS_SESSION_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_SESSION_VIEW))
#define PPG_IS_SESSION_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_SESSION_VIEW))
#define PPG_SESSION_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_SESSION_VIEW, PpgSessionViewClass))

typedef struct _PpgSessionView        PpgSessionView;
typedef struct _PpgSessionViewClass   PpgSessionViewClass;
typedef struct _PpgSessionViewPrivate PpgSessionViewPrivate;
typedef enum   _PpgSessionViewTool    PpgSessionViewTool;

enum _PpgSessionViewTool
{
	PPG_SESSION_VIEW_POINTER,
	PPG_SESSION_VIEW_TIME_SELECTION,
	PPG_SESSION_VIEW_ZOOM,
};

struct _PpgSessionView
{
	GtkAlignment parent;

	/*< private >*/
	PpgSessionViewPrivate *priv;
};

struct _PpgSessionViewClass
{
	GtkAlignmentClass parent_class;
};

GType    ppg_session_view_get_type           (void) G_GNUC_CONST;
gboolean ppg_session_view_get_show_data      (PpgSessionView     *view);
void     ppg_session_view_set_show_data      (PpgSessionView     *view,
                                              gboolean            show_data);
PpgInstrumentView*
         ppg_session_view_get_selected_item  (PpgSessionView     *view);
void     ppg_session_view_set_selected_item  (PpgSessionView     *view,
                                              PpgInstrumentView  *instrument_view);
void     ppg_session_view_move_forward       (PpgSessionView     *view);
void     ppg_session_view_move_backward      (PpgSessionView     *view);
void     ppg_session_view_move_up            (PpgSessionView     *view);
void     ppg_session_view_move_down          (PpgSessionView     *view);
gboolean ppg_session_view_get_selection      (PpgSessionView     *view,
                                              gdouble            *begin_time,
                                              gdouble            *end_time);
void     ppg_session_view_set_selection      (PpgSessionView     *view,
                                              gdouble             begin_time,
                                              gdouble             end_time);
void     ppg_session_view_set_tool           (PpgSessionView     *view,
                                              PpgSessionViewTool  tool);
gdouble  ppg_session_view_get_zoom           (PpgSessionView     *view);
void     ppg_session_view_zoom_in            (PpgSessionView     *view);
void     ppg_session_view_zoom_normal        (PpgSessionView     *view);
void     ppg_session_view_zoom_out           (PpgSessionView     *view);
void     ppg_session_view_add_instrument     (PpgSessionView     *view,
                                              PpgInstrument      *instrument);

G_END_DECLS

#endif /* PPG_SESSION_VIEW_H */

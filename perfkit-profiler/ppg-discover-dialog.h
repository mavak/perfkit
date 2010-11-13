/* ppg-discover-dialog.h
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

#ifndef PPG_DISCOVER_DIALOG_H
#define PPG_DISCOVER_DIALOG_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define PPG_TYPE_DISCOVER_DIALOG            (ppg_discover_dialog_get_type())
#define PPG_DISCOVER_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_DISCOVER_DIALOG, PpgDiscoverDialog))
#define PPG_DISCOVER_DIALOG_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_DISCOVER_DIALOG, PpgDiscoverDialog const))
#define PPG_DISCOVER_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_DISCOVER_DIALOG, PpgDiscoverDialogClass))
#define PPG_IS_DISCOVER_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_DISCOVER_DIALOG))
#define PPG_IS_DISCOVER_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_DISCOVER_DIALOG))
#define PPG_DISCOVER_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_DISCOVER_DIALOG, PpgDiscoverDialogClass))

typedef struct _PpgDiscoverDialog        PpgDiscoverDialog;
typedef struct _PpgDiscoverDialogClass   PpgDiscoverDialogClass;
typedef struct _PpgDiscoverDialogPrivate PpgDiscoverDialogPrivate;

struct _PpgDiscoverDialog
{
	GtkDialog parent;

	/*< private >*/
	PpgDiscoverDialogPrivate *priv;
};

struct _PpgDiscoverDialogClass
{
	GtkDialogClass parent_class;
};

GType ppg_discover_dialog_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* PPG_DISCOVER_DIALOG_H */

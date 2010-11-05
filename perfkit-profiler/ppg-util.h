/* ppg-util.h
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

#ifndef __PPG_UTIL_H__
#define __PPG_UTIL_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

const gchar* ppg_util_uname (void);
gsize ppg_util_get_total_memory (void);

void ppg_util_load_ui (GtkWidget       *widget,
                       GtkActionGroup **actions,
                       const gchar     *ui_data,
                       const gchar     *first_widget,
                       ...);

gboolean ppg_util_base_expose_event (GtkWidget       *widget,
                                     GdkEventExpose  *event);
gboolean ppg_util_bg_expose_event   (GtkWidget       *widget,
                                     GdkEventExpose  *event);
gboolean ppg_util_fg_expose_event   (GtkWidget       *widget,
                                     GdkEventExpose  *event);

GtkWidget* ppg_util_header_item_new (const gchar     *label);

G_END_DECLS

#endif /* __PPG_UTIL_H__ */

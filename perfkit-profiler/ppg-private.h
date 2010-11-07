/* ppg-private.h
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

#ifndef PPG_PRIVATE_H
#define PPG_PRIVATE_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

void     ppg_actions_init         (void);
void     ppg_actions_shutdown     (void);
void     ppg_color_init           (void);
void     ppg_color_shutdown       (void);
void     ppg_instruments_init     (void);
void     ppg_instruments_shutdown (void);
void     ppg_log_init             (gboolean       stdout_,
                                   const gchar   *filename);
void     ppg_log_shutdown         (void);
void     ppg_monitor_init         (void);
void     ppg_monitor_shutdown     (void);
void     ppg_paths_init           (void);
void     ppg_paths_shutdown       (void);
void     ppg_prefs_init           (gint          *argc,
                                   gchar       ***argv);
void     ppg_prefs_shutdown       (void);
gboolean ppg_runtime_init         (gint          *argc,
                                   gchar       ***argv);
gint     ppg_runtime_run          (void);
void     ppg_runtime_shutdown     (void);

G_END_DECLS

#endif /* PPG_PRIVATE_H */

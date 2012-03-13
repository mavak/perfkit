/* ppg-frame-source.h
 *
 * Copyright (C) 2012 Christian Hergert <chris@dronelabs.com>
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

#ifndef PPG_FRAME_SOURCE_H
#define PPG_FRAME_SOURCE_H

#include <glib.h>

G_BEGIN_DECLS

guint ppg_frame_source_add (guint       frames_per_sec,
                           GSourceFunc callback,
                           gpointer    user_data);

G_END_DECLS

#endif /* PPG_FRAME_SOURCE_H */

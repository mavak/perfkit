/* ppg-color.h
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

#ifndef __PPG_COLOR_H__
#define __PPG_COLOR_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct
{
	GdkColor color;  /* Color in GdkColor format */
	guint rgba;      /* Color in Goocanvas rgba format */
	gchar html[16];  /* Color in html '#fff' format */

	/*< private >*/
	guint mod;
} PpgColorIter;

void ppg_color_iter_init (PpgColorIter *iter);
void ppg_color_iter_next (PpgColorIter *iter);

G_END_DECLS

#endif /* __PPG_COLOR_H__ */

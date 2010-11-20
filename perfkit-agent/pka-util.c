/* pka-util.c
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

#include "pka-util.h"

const gchar *
pka_get_user_runtime_dir (void)
{
	static gsize initialized  = FALSE;
	static gchar *runtime_dir = NULL;

	if (g_once_init_enter(&initialized)) {
		gchar *name = g_strdup_printf("perfkit-%s", g_get_user_name());
		runtime_dir = g_build_filename(g_get_tmp_dir(), name, NULL);
		g_once_init_leave(&initialized, TRUE);
	}

	return runtime_dir;
}

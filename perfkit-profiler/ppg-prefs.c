/* ppg-prefs.c
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

#include <gdk/gdk.h>
#include <glib/gi18n.h>

#include "ppg-prefs.h"

#define PROJECT_SCHEMA             "org.perfkit.profiler.project"
#define PROJECT_SCHEMA_DEFAULT_DIR "default-dir"

static GSettings    *project_settings = NULL;
static GOptionEntry  entries[]        = {
	{ NULL }
};

GOptionGroup*
ppg_prefs_get_option_group (void)
{
	GOptionGroup *group;

	group = g_option_group_new("prefs", "Application preferences",
	                           "", NULL, NULL);
	g_option_group_add_entries(group, entries);
	return group;
}

gboolean
ppg_prefs_init (void)
{
	project_settings = g_settings_new(PROJECT_SCHEMA);
	return TRUE;
}

void
ppg_prefs_shutdown (void)
{
	g_object_unref(project_settings);
	project_settings = NULL;
}

GSettings*
ppg_prefs_get_project_settings (void)
{
	return project_settings;
}

#define STR_PREF(_n, _N)                                                     \
	                                                                         \
    gchar *                                                                  \
    ppg_prefs_get_project_##_n (void)                                        \
    {                                                                        \
	    return g_settings_get_string(project_settings, PROJECT_SCHEMA_##_N); \
    }                                                                        \
	                                                                         \
    void                                                                     \
    ppg_prefs_set_project_##_n (const gchar *str)                            \
    {                                                                        \
	    g_settings_set_string(project_settings, PROJECT_SCHEMA_##_N, str);   \
    }

STR_PREF(default_dir, DEFAULT_DIR)

void
ppg_prefs_get_window_size (gint *width,
                           gint *height)
{
	GdkScreen *screen = gdk_screen_get_default();

	/*
	 * TODO: We might want to consider tracking the last size of the window
	 *       and storing it. Although, there is plenty of information on
	 *       why that is wrong to be found on the interwebs.
	 */

	if (width) {
		*width = gdk_screen_get_width(screen) * 0.75;
	}
	if (height) {
		*height = gdk_screen_get_height(screen) * 0.75;
	}
}

/* ppg-window-actions.h
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

#ifndef PPG_WINDOW_ACTIONS_H
#define PPG_WINDOW_ACTIONS_H

#include <gtk/gtk.h>

#include "ppg-window.h"

G_BEGIN_DECLS

#define DEFINE_ACTION_CALLBACK(_n)                             \
    static void ppg_window_##_n##_activate (GtkAction *action, \
                                            PpgWindow *window)
#define ACTION_CALLBACK(_n) G_CALLBACK(ppg_window_##_n##_activate)

DEFINE_ACTION_CALLBACK(about);
DEFINE_ACTION_CALLBACK(add_instrument);
DEFINE_ACTION_CALLBACK(fullscreen);
DEFINE_ACTION_CALLBACK(move_forward);
DEFINE_ACTION_CALLBACK(move_backward);
DEFINE_ACTION_CALLBACK(next_instrument);
DEFINE_ACTION_CALLBACK(prev_instrument);
DEFINE_ACTION_CALLBACK(quit);
DEFINE_ACTION_CALLBACK(zoom_in);
DEFINE_ACTION_CALLBACK(zoom_instrument_in);
DEFINE_ACTION_CALLBACK(zoom_instrument_out);
DEFINE_ACTION_CALLBACK(zoom_one);
DEFINE_ACTION_CALLBACK(zoom_out);

static GtkActionEntry ppg_window_action_entries[] = {
	{ "file",
	  NULL,
	  N_("Per_fkit") },

	{ "new-session",
	  NULL,
	  N_("_New Session"),
	  NULL,
	  NULL,
	  NULL },

	{ "quit",
	  GTK_STOCK_QUIT,
	  NULL,
	  NULL,
	  NULL,
	  ACTION_CALLBACK(quit) },

	{ "close", GTK_STOCK_CLOSE,
	  N_("_Close Window"),
	  NULL,
	  NULL,
	  NULL },

	{ "edit",
	  NULL,
	  N_("_Edit") },

	{ "cut",
	  GTK_STOCK_CUT },

	{ "copy",
	  GTK_STOCK_COPY },

	{ "paste",
	  GTK_STOCK_PASTE },

	{ "preferences",
	  GTK_STOCK_PREFERENCES,
	  NULL,
	  "<control>comma",
	  N_("Configure preferences for " PRODUCT_NAME),
	  NULL },

	{ "profiler",
	  NULL,
	  N_("_Profiler") },

	{ "target",
	  NULL,
	  N_("Target") },

	{ "restart",
	  GTK_STOCK_REFRESH,
	  N_("Res_tart"),
	  NULL,
	  N_("Restart the current profiling session"),
	  NULL },

	{ "settings",
	  NULL,
	  N_("S_ettings"),
	  "<control>d",
	  N_("Adjust settings for the current profiling session"),
	  NULL },

	{ "instrument",
	  NULL,
	  N_("_Instrument") },

	{ "add-instrument",
	  GTK_STOCK_ADD,
	  N_("_Add Instrument"),
	  "<control><shift>n",
	  N_("Add an instrument to the current profiling session"),
	  ACTION_CALLBACK(add_instrument) },

	{ "configure-instrument",
	  NULL,
	  N_("_Configure"),
	  NULL,
	  N_("Configure the selected instrument"),
	  NULL },

	{ "visualizers",
	  NULL,
	  N_("_Visualizers") },

	{ "next-instrument",
	  NULL,
	  N_("Next"),
	  "j",
	  NULL,
	  ACTION_CALLBACK(next_instrument) },

	{ "previous-instrument",
	  NULL,
	  N_("Previous"),
	  "k",
	  NULL,
	  ACTION_CALLBACK(prev_instrument) },

	{ "target-spawn", NULL, N_("Spawn a new process"), "<control>t", NULL, NULL },
	{ "target-existing", NULL, N_("Select an existing process"), NULL, NULL, NULL },
	{ "target-none", NULL, N_("No target"), NULL, NULL, NULL },

	{ "tools", NULL, N_("_Tools") },
	{ "monitor", NULL, N_("Monitor") },
	{ "monitor-cpu", NULL, N_("CPU Usage"), NULL, NULL, NULL },
	{ "monitor-mem", NULL, N_("Memory Usage"), NULL, NULL, NULL },
	{ "monitor-net", NULL, N_("Network Usage"), NULL, NULL, NULL },

	{ "view",
	  NULL,
	  N_("_View") },

	{ "zoom-in",
	  GTK_STOCK_ZOOM_IN,
	  N_("Zoom In"),
	  "<control>plus",
	  NULL,
	  ACTION_CALLBACK(zoom_in) },

	{ "zoom-out",
	  GTK_STOCK_ZOOM_OUT,
	  N_("Zoom Out"),
	  "<control>minus",
	  NULL,
	  ACTION_CALLBACK(zoom_out) },

	{ "zoom-one",
	  GTK_STOCK_ZOOM_100,
	  N_("Normal Size"),
	  "<control>0",
	  NULL,
	  ACTION_CALLBACK(zoom_one) },

	{ "go-forward",
	  NULL,
	  N_("Move Forward in Time"),
	  "l",
	  NULL,
	  ACTION_CALLBACK(move_forward) },

	{ "go-back",
	  NULL,
	  N_("Move Backward in Time"),
	  "h",
	  NULL,
	  ACTION_CALLBACK(move_backward) },

	{ "zoom-in-instrument",
	  NULL,
	  N_("Zoom In Instrument"),
	  "space",
	  NULL,
	  ACTION_CALLBACK(zoom_instrument_in) },

	{ "zoom-out-instrument",
	  NULL,
	  N_("Zoom Out Instrument"),
	  "<shift>space",
	  NULL,
	  ACTION_CALLBACK(zoom_instrument_out) },

	{ "help",
	  NULL,
	  N_("_Help") },

	{ "about",
	  GTK_STOCK_ABOUT,
	  N_("About " PRODUCT_NAME),
	  NULL,
	  NULL,
	  ACTION_CALLBACK(about) },
};

static GtkToggleActionEntry ppg_window_toggle_action_entries[] = {
	{ "stop", GTK_STOCK_MEDIA_STOP, N_("_Stop"), "<control>e", N_("Stop the current profiling session"), NULL },
	{ "pause", GTK_STOCK_MEDIA_PAUSE, N_("_Pause"), "<control>z", N_("Pause the current profiling session"), NULL },
	{ "run", "media-playback-start", N_("_Run"), "<control>b", N_("Run the current profiling session"), NULL },

	{ "fullscreen",
	  GTK_STOCK_FULLSCREEN,
	  NULL,
	  "F11",
	  NULL,
	  ACTION_CALLBACK(fullscreen) },

	{ "show-data",
	  NULL,
	  N_("Show extended data") },
};

#undef ACTION_CALLBACK
#undef DEFINE_ACTION_CALLBACK

G_END_DECLS

#endif /* PPG_WINDOW_ACTIONS_H */

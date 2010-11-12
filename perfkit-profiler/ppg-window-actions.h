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

#ifndef __PPG_WINDOW_ACTIONS_H__
#define __PPG_WINDOW_ACTIONS_H__

#include <gtk/gtk.h>

#include "ppg-window.h"

G_BEGIN_DECLS

static void ppg_window_quit_activate                 (GtkAction *action,
                                                      PpgWindow *window);
static void ppg_window_close_activate                (GtkAction *action,
                                                      PpgWindow *window);
static void ppg_window_restart_activate              (GtkAction *action,
                                                      PpgWindow *window);
static void ppg_window_stop_activate                 (GtkAction *action,
                                                      PpgWindow *window);
static void ppg_window_pause_activate                (GtkAction *action,
                                                      PpgWindow *window);
static void ppg_window_run_activate                  (GtkAction *action,
                                                      PpgWindow *window);
static void ppg_window_target_spawn_activate         (GtkAction *action,
                                                      PpgWindow *window);
static void ppg_window_fullscreen_activate           (GtkAction *action,
                                                      PpgWindow *window);
static void ppg_window_about_activate                (GtkAction *action,
                                                      PpgWindow *window);
static void ppg_window_add_instrument_activate       (GtkAction *action,
                                                      PpgWindow *window);
static void ppg_window_configure_instrument_activate (GtkAction *action,
                                                      PpgWindow *window);
static void ppg_window_preferences_activate          (GtkAction *action,
                                                      PpgWindow *window);
static void ppg_window_settings_activate             (GtkAction *action,
                                                      PpgWindow *window);
static void ppg_window_zoom_in_activate              (GtkAction *action,
                                                      PpgWindow *window);
static void ppg_window_zoom_out_activate             (GtkAction *action,
                                                      PpgWindow *window);
static void ppg_window_zoom_one_activate             (GtkAction *action,
                                                      PpgWindow *window);
static void ppg_window_monitor_cpu_activate          (GtkAction *action,
                                                      PpgWindow *window);
static void ppg_window_monitor_mem_activate          (GtkAction *action,
                                                      PpgWindow *window);
static void ppg_window_monitor_net_activate          (GtkAction *action,
                                                      PpgWindow *window);
static void ppg_window_next_activate                 (GtkAction *action,
                                                      PpgWindow *window);
static void ppg_window_previous_activate             (GtkAction *action,
                                                      PpgWindow *window);
static void ppg_window_zoom_in_instrument_activate   (GtkAction *action,
                                                      PpgWindow *window);
static void ppg_window_zoom_out_instrument_activate  (GtkAction *action,
                                                      PpgWindow *window);
static void ppg_window_go_forward_activate           (GtkAction *action,
                                                      PpgWindow *window);
static void ppg_window_go_back_activate              (GtkAction *action,
                                                      PpgWindow *window);

static GtkActionEntry ppg_window_action_entries[] = {
	{ "file", NULL, N_("Per_fkit") },
	{ "quit", GTK_STOCK_QUIT, NULL, NULL, NULL, G_CALLBACK(ppg_window_quit_activate) },
	{ "close", GTK_STOCK_CLOSE, N_("_Close Window"), NULL, NULL, G_CALLBACK(ppg_window_close_activate) },

	{ "edit", NULL, N_("_Edit") },
	{ "cut", GTK_STOCK_CUT },
	{ "copy", GTK_STOCK_COPY },
	{ "paste", GTK_STOCK_PASTE },
	{ "preferences", GTK_STOCK_PREFERENCES, NULL, "<control>comma", N_("Configure preferences for " PRODUCT_NAME), G_CALLBACK(ppg_window_preferences_activate) },

	{ "profiler", NULL, N_("_Profiler") },
	{ "target", NULL, N_("Target") },
	{ "restart", GTK_STOCK_REFRESH, N_("Res_tart"), NULL, N_("Restart the current profiling session"), G_CALLBACK(ppg_window_restart_activate) },
	{ "settings", NULL, N_("S_ettings"), "<control>d", N_("Adjust settings for the current profiling session"), G_CALLBACK(ppg_window_settings_activate) },

	{ "instrument", NULL, N_("_Instrument") },
	{ "add-instrument", GTK_STOCK_ADD, N_("_Add Instrument"), "<control><shift>n", N_("Add an instrument to the current profiling session"), G_CALLBACK(ppg_window_add_instrument_activate) },
	{ "configure-instrument", NULL, N_("_Configure"), NULL, N_("Configure the selected instrument"), G_CALLBACK(ppg_window_configure_instrument_activate) },
	{ "visualizers", NULL, N_("_Visualizers") },
	{ "next-instrument", NULL, N_("Next"), "j", NULL, G_CALLBACK(ppg_window_next_activate) },
	{ "previous-instrument", NULL, N_("Previous"), "k", NULL, G_CALLBACK(ppg_window_previous_activate) },
	{ "go-forward", NULL, N_("Go Forward in Time"), "l", NULL, G_CALLBACK(ppg_window_go_forward_activate) },
	{ "go-back", NULL, N_("Go Backwards in Time"), "h", NULL, G_CALLBACK(ppg_window_go_back_activate) },
	{ "zoom-in-instrument", NULL, N_("Zoom In Instrument"), "space", NULL, G_CALLBACK(ppg_window_zoom_in_instrument_activate) },
	{ "zoom-out-instrument", NULL, N_("Zoom Out Instrument"), "<shift>space", NULL, G_CALLBACK(ppg_window_zoom_out_instrument_activate) },

	{ "target-spawn", NULL, N_("Spawn a new process"), "<control>t", NULL, G_CALLBACK(ppg_window_target_spawn_activate) },
	{ "target-existing", NULL, N_("Select an existing process"), NULL, NULL, NULL },
	{ "target-none", NULL, N_("No target"), NULL, NULL, NULL },

	{ "tools", NULL, N_("_Tools") },
	{ "monitor", NULL, N_("Monitor") },
	{ "monitor-cpu", NULL, N_("CPU Usage"), NULL, NULL, G_CALLBACK(ppg_window_monitor_cpu_activate) },
	{ "monitor-mem", NULL, N_("Memory Usage"), NULL, NULL, G_CALLBACK(ppg_window_monitor_mem_activate) },
	{ "monitor-net", NULL, N_("Network Usage"), NULL, NULL, G_CALLBACK(ppg_window_monitor_net_activate) },

	{ "view", NULL, N_("_View") },
	{ "zoom-in", GTK_STOCK_ZOOM_IN, N_("Zoom In"), "<control>equal", NULL, G_CALLBACK(ppg_window_zoom_in_activate) },
	{ "zoom-out", GTK_STOCK_ZOOM_OUT, N_("Zoom Out"), "<control>minus", NULL, G_CALLBACK(ppg_window_zoom_out_activate) },
	{ "zoom-one", GTK_STOCK_ZOOM_100, N_("Normal Size"), "<control>0", NULL, G_CALLBACK(ppg_window_zoom_one_activate) },

	{ "help", NULL, N_("_Help") },
	{ "about", GTK_STOCK_ABOUT, N_("About " PRODUCT_NAME), NULL, NULL, G_CALLBACK(ppg_window_about_activate) },
};

static GtkToggleActionEntry ppg_window_toggle_action_entries[] = {
	{ "stop", GTK_STOCK_MEDIA_STOP, N_("_Stop"), "<control>e", N_("Stop the current profiling session"), G_CALLBACK(ppg_window_stop_activate) },
	{ "pause", GTK_STOCK_MEDIA_PAUSE, N_("_Pause"), "<control>z", N_("Pause the current profiling session"), G_CALLBACK(ppg_window_pause_activate) },
	{ "run", "media-playback-start", N_("_Run"), "<control>b", N_("Run the current profiling session"), G_CALLBACK(ppg_window_run_activate) },
	{ "fullscreen", GTK_STOCK_FULLSCREEN, NULL, "F11", NULL, G_CALLBACK(ppg_window_fullscreen_activate), FALSE },
};

G_END_DECLS

#endif /* __PPG_WINDOW_ACTIONS_H__ */

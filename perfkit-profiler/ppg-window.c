/* ppg-window.c
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

#include <glib/gi18n.h>
#include <perfkit/perfkit.h>

#include "ppg-about-dialog.h"
#include "ppg-actions.h"
#include "ppg-add-instrument-dialog.h"
#include "ppg-log.h"
#include "ppg-menu-tool-item.h"
#include "ppg-prefs-dialog.h"
#include "ppg-runtime.h"
#include "ppg-session.h"
#include "ppg-session-view.h"
#include "ppg-spawn-process-dialog.h"
#include "ppg-timer-tool-item.h"
#include "ppg-util.h"
#include "ppg-window.h"
#include "ppg-window-actions.h"
#include "ppg-window-ui.h"


struct _PpgWindowPrivate
{
	PpgSession     *session;
	GtkActionGroup *actions;
	gchar          *uri;
	gboolean        state_frozen;
	GtkWidget      *menubar;
	GtkWidget      *session_view;
	GtkWidget      *target_tool_item;
	GtkWidget      *timer_tool_item;
	GtkWidget      *toolbar;
};


enum
{
	PROP_0,

	PROP_URI,
};


static guint n_windows = 0;


G_DEFINE_TYPE(PpgWindow, ppg_window, GTK_TYPE_WINDOW)


/**
 * ppg_window_check_close:
 * @window: (in): A #PpgWindow.
 *
 * Checks to see if the window is in a state that can be closed. If the session
 * is active, then the user will be prompted to confirm.
 *
 * Returns: %TRUE if the window can be closed; otherwise %FALSE.
 * Side effects: None.
 */
static gboolean
ppg_window_check_close (PpgWindow *window)
{
	PpgWindowPrivate *priv;
	GtkAction *action;
	GtkWidget *dialog;
	gboolean ret = TRUE;

	g_return_val_if_fail(PPG_IS_WINDOW(window), FALSE);

	priv = window->priv;

	/*
	 * If the stop action is sensitive, then the session is likely running
	 * and we should ask the user if we should close.
	 */

	action = gtk_action_group_get_action(priv->actions, "stop");
	if (gtk_action_get_sensitive(action)) {
		dialog = g_object_new(GTK_TYPE_MESSAGE_DIALOG,
		                      "buttons", GTK_BUTTONS_OK_CANCEL,
		                      "message-type", GTK_MESSAGE_QUESTION,
		                      "text", _("The current profiling session is "
		                                "active. Would you like to close "
		                                "this session?"),
		                      "transient-for", window,
		                      NULL);
		if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_OK) {
			ret = FALSE;
		}
		gtk_widget_destroy(dialog);
	}

	return ret;
}


static void
ppg_window_add_instrument_activate (GtkAction *action,
                                    PpgWindow *window)
{
	PpgWindowPrivate *priv;
	GtkDialog *dialog;

	g_return_if_fail(PPG_IS_WINDOW(window));

	priv = window->priv;

	dialog = g_object_new(PPG_TYPE_ADD_INSTRUMENT_DIALOG,
	                      "session", priv->session,
	                      "transient-for", window,
	                      NULL);
	gtk_dialog_run(dialog);
	gtk_widget_destroy(GTK_WIDGET(dialog));
}


static void
ppg_window_next_instrument_activate (GtkAction *action,
                                     PpgWindow *window)
{
	g_return_if_fail(PPG_IS_WINDOW(window));
	ppg_session_view_move_down(PPG_SESSION_VIEW(window->priv->session_view));
}


static void
ppg_window_prev_instrument_activate (GtkAction *action,
                                     PpgWindow *window)
{
	g_return_if_fail(PPG_IS_WINDOW(window));
	ppg_session_view_move_up(PPG_SESSION_VIEW(window->priv->session_view));
}


static void
ppg_window_zoom_in_activate (GtkAction *action,
                             PpgWindow *window)
{
	g_return_if_fail(PPG_IS_WINDOW(window));
	ppg_session_view_zoom_in(PPG_SESSION_VIEW(window->priv->session_view));
}


static void
ppg_window_zoom_out_activate (GtkAction *action,
                              PpgWindow *window)
{
	g_return_if_fail(PPG_IS_WINDOW(window));
	ppg_session_view_zoom_out(PPG_SESSION_VIEW(window->priv->session_view));
}


static void
ppg_window_zoom_one_activate (GtkAction *action,
                              PpgWindow *window)
{
	g_return_if_fail(PPG_IS_WINDOW(window));
	ppg_session_view_zoom_normal(PPG_SESSION_VIEW(window->priv->session_view));
}


static void
ppg_window_zoom_instrument_in_activate (GtkAction *action,
                                        PpgWindow *window)
{
	PpgWindowPrivate *priv;
	PpgSessionView *session_view;
	PpgInstrumentView *view;

	g_return_if_fail(PPG_IS_WINDOW(window));

	priv = window->priv;

	session_view = PPG_SESSION_VIEW(priv->session_view);
	view = ppg_session_view_get_selected_item(session_view);
	ppg_instrument_view_zoom_in(view);
}


static void
ppg_window_zoom_instrument_out_activate (GtkAction *action,
                                         PpgWindow *window)
{
	PpgWindowPrivate *priv;
	PpgSessionView *session_view;
	PpgInstrumentView *view;

	g_return_if_fail(PPG_IS_WINDOW(window));

	priv = window->priv;

	session_view = PPG_SESSION_VIEW(priv->session_view);
	view = ppg_session_view_get_selected_item(session_view);
	ppg_instrument_view_zoom_out(view);
}


static void
ppg_window_move_forward_activate (GtkAction *action,
                                  PpgWindow *window)
{
	g_return_if_fail(PPG_IS_WINDOW(window));

	ppg_session_view_move_forward(
			PPG_SESSION_VIEW(window->priv->session_view));
}


static void
ppg_window_move_backward_activate (GtkAction *action,
                                   PpgWindow *window)
{
	g_return_if_fail(PPG_IS_WINDOW(window));

	ppg_session_view_move_backward(
			PPG_SESSION_VIEW(window->priv->session_view));
}


static void
ppg_window_quit_activate (GtkAction *action,
                          PpgWindow *window)
{
	if (!ppg_window_check_close(window)) {
		return;
	}
	ppg_runtime_quit();
}


static void
ppg_window_fullscreen_activate (GtkAction *action,
                                PpgWindow *window)
{
	gboolean active;

	g_object_get(action, "active", &active, NULL);
	if (active) {
		gtk_window_fullscreen(GTK_WINDOW(window));
	} else {
		gtk_window_unfullscreen(GTK_WINDOW(window));
	}
}


static void
ppg_window_about_activate (GtkAction *action,
                           PpgWindow *window)
{
	GtkWindow *dialog;

	dialog = g_object_new(PPG_TYPE_ABOUT_DIALOG,
	                      "transient-for", window,
	                      NULL);
	gtk_window_present(GTK_WINDOW(dialog));
}


static void
ppg_window_prefs_activate (GtkAction *action,
                           PpgWindow *window)
{
	GtkWidget *prefs;

	prefs = g_object_new(PPG_TYPE_PREFS_DIALOG,
	                     "transient-for", window,
	                     NULL);
	g_signal_connect(prefs, "delete-event",
	                 G_CALLBACK(gtk_widget_hide_on_delete),
	                 NULL);
	gtk_dialog_run(GTK_DIALOG(prefs));
}


static void
ppg_window_target_spawn_activate (GtkAction *action,
                                  PpgWindow *window)
{
	GtkWidget *dialog;

	dialog = g_object_new(PPG_TYPE_SPAWN_PROCESS_DIALOG,
	                      "transient-for", window,
	                      "session", window->priv->session,
	                      NULL);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}


guint
ppg_window_count (void)
{
	return n_windows;
}


static void
ppg_window_freeze_state (PpgWindow *window)
{
	g_return_if_fail(PPG_IS_WINDOW(window));
	window->priv->state_frozen = TRUE;
}


static void
ppg_window_thaw_state (PpgWindow *window)
{
	g_return_if_fail(PPG_IS_WINDOW(window));
	window->priv->state_frozen = FALSE;
}


static void
ppg_window_session_notify_state (PpgWindow  *window,
                                 GParamSpec *pspec,
                                 PpgSession *session)
{
	PpgWindowPrivate *priv;
	PpgSessionState state;
	gboolean insensitive = FALSE;
	gboolean pause_active = FALSE;
	gboolean pause_sensitive = FALSE;
	gboolean run_active = FALSE;
	gboolean run_sensitive = FALSE;
	gboolean stop_active = FALSE;
	gboolean stop_sensitive = FALSE;

	g_return_if_fail(PPG_IS_WINDOW(window));
	g_return_if_fail(PPG_IS_SESSION(session));

	priv = window->priv;

	state = ppg_session_get_state(session);
	ppg_window_freeze_state(window);

	switch (state) {
	case PPG_SESSION_INITIAL:
		insensitive = TRUE;
		stop_active = TRUE;
		break;
	case PPG_SESSION_READY:
		stop_active = TRUE;
		run_sensitive = TRUE;
		break;
	case PPG_SESSION_STARTED:
		run_active = TRUE;
		run_sensitive = FALSE;
		pause_sensitive = TRUE;
		stop_sensitive = TRUE;
		break;
	case PPG_SESSION_STOPPED:
		stop_active = TRUE;
		run_sensitive = TRUE;
		break;
	case PPG_SESSION_MUTED:
		pause_active = TRUE;
		pause_sensitive = TRUE;
		run_active = TRUE;
		stop_sensitive = TRUE;
		break;
	case PPG_SESSION_FAILED:
		stop_active = TRUE;
		break;
	default:
		g_assert_not_reached();
		return;
	}

	ppg_window_action_set(window, "stop",
	                      "active", stop_active,
	                      "sensitive", stop_sensitive,
	                      NULL);
	ppg_window_action_set(window, "pause",
	                      "active", pause_active,
	                      "sensitive", pause_sensitive,
	                      NULL);
	ppg_window_action_set(window, "run",
	                      "active", run_active,
	                      "sensitive", run_sensitive,
	                      NULL);

	gtk_widget_set_sensitive(GTK_WIDGET(window), !insensitive);
	ppg_window_thaw_state(window);
}


static void
ppg_window_set_uri (PpgWindow   *window,
                    const gchar *uri)
{
	PpgWindowPrivate *priv;
	PkConnection *connection;

	g_return_if_fail(PPG_IS_WINDOW(window));

	priv = window->priv;

	if (!(connection = pk_connection_new_from_uri(uri))) {
		g_critical("Invalid perfkit uri: %s", uri);
		return;
	}

	priv->uri = g_strdup(uri);
	priv->session = g_object_new(PPG_TYPE_SESSION,
	                             "connection", connection,
	                             NULL);
	g_object_set(priv->session_view,
	             "session", priv->session,
	             NULL);
	g_signal_connect_swapped(priv->session, "notify::state",
	                         G_CALLBACK(ppg_window_session_notify_state),
	                         window);
	g_object_notify(G_OBJECT(priv->session), "state");
}


static GtkAction*
ppg_window_get_action (PpgWindow   *window,
                       const gchar *action)
{
	g_return_val_if_fail(PPG_IS_WINDOW(window), NULL);
	return gtk_action_group_get_action(window->priv->actions, action);
}


/**
 * ppg_window_action_set:
 * @window: (in): A #PpgWindow.
 * @name: (in): The #GtkAction<!-- -->'s name.
 * @first_property: (in): The name of the first property to set.
 *
 * Set properties on a given #GtkAction. The arguments of this function behave
 * identically to g_object_set().
 *
 * Returns: None.
 * Side effects: None.
 */
void
ppg_window_action_set (PpgWindow *window,
                       const gchar *name,
                       const gchar *first_property,
                       ...)
{
	PpgWindowPrivate *priv;
	GObject *object;
	va_list args;

	g_return_if_fail(PPG_IS_WINDOW(window));
	g_return_if_fail(name != NULL);
	g_return_if_fail(first_property != NULL);

	priv = window->priv;

	if (!(object = (GObject *)ppg_window_get_action(window, name))) {
		CRITICAL(Window, "No action named %s", name);
		return;
	}

	va_start(args, first_property);
	g_object_set_valist(object, first_property, args);
	va_end(args);
}


static gboolean
ppg_window_delete_event (GtkWidget   *widget,
                         GdkEventAny *event)
{
	PpgWindow *window = (PpgWindow *)widget;

	g_return_val_if_fail(PPG_IS_WINDOW(window), FALSE);

	/*
	 * If we can close this window, then decrement our active instances
	 * count and attempt to quit the application.
	 */
	if (ppg_window_check_close(window)) {
		n_windows--;
		ppg_runtime_try_quit();
		return FALSE;
	}
	return TRUE;
}


static void
ppg_window_realize (GtkWidget *widget)
{
	GdkGeometry geom = { 640, 300 };
	GTK_WIDGET_CLASS(ppg_window_parent_class)->realize(widget);
	gtk_window_set_geometry_hints(GTK_WINDOW(widget), widget,
	                              &geom, GDK_HINT_MIN_SIZE);
}


/**
 * ppg_window_finalize:
 * @object: (in): A #PpgWindow.
 *
 * Finalizer for a #PpgWindow instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_window_finalize (GObject *object)
{
	PpgWindowPrivate *priv = PPG_WINDOW(object)->priv;

	g_free(priv->uri);

	G_OBJECT_CLASS(ppg_window_parent_class)->finalize(object);
}

/**
 * ppg_window_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (out): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Get a given #GObject property.
 */
static void
ppg_window_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
	PpgWindow *window = PPG_WINDOW(object);

	switch (prop_id) {
	case PROP_URI:
		g_value_set_string(value, window->priv->uri);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}


/**
 * ppg_window_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
static void
ppg_window_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
	PpgWindow *window = PPG_WINDOW(object);

	switch (prop_id) {
	case PROP_URI:
		ppg_window_set_uri(window, g_value_get_string(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}


/**
 * ppg_window_class_init:
 * @klass: (in): A #PpgWindowClass.
 *
 * Initializes the #PpgWindowClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_window_class_init (PpgWindowClass *klass)
{
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_window_finalize;
	object_class->get_property = ppg_window_get_property;
	object_class->set_property = ppg_window_set_property;
	g_type_class_add_private(object_class, sizeof(PpgWindowPrivate));

	widget_class = GTK_WIDGET_CLASS(klass);
	widget_class->delete_event = ppg_window_delete_event;
	widget_class->realize = ppg_window_realize;

	g_object_class_install_property(object_class,
	                                PROP_URI,
	                                g_param_spec_string("uri",
	                                                    "Uri",
	                                                    "The uri of the session",
	                                                    NULL,
	                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	ppg_actions_register_entries(PPG_TYPE_WINDOW,
	                             ppg_window_action_entries,
	                             G_N_ELEMENTS(ppg_window_action_entries));

	ppg_actions_register_toggle_entries(PPG_TYPE_WINDOW,
	                                    ppg_window_toggle_action_entries,
	                                    G_N_ELEMENTS(ppg_window_toggle_action_entries));
}

/**
 * ppg_window_init:
 * @window: (in): A #PpgWindow.
 *
 * Initializes the newly created #PpgWindow instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_window_init (PpgWindow *window)
{
	PpgWindowPrivate *priv;
	GtkWidget *vbox;
	GtkWidget *mb_target_existing;
	GtkWidget *mb_visualizers;
	GtkWidget *target_menu;
	GtkWidget *target_existing;

	window->priv = G_TYPE_INSTANCE_GET_PRIVATE(window, PPG_TYPE_WINDOW,
	                                           PpgWindowPrivate);
	priv = window->priv;

	n_windows++;

	g_object_set(window,
	             "title", _(PRODUCT_NAME),
	             "default-width", 800,
	             "default-height", 494,
	             "window-position", GTK_WIN_POS_CENTER,
	             NULL);

	ppg_util_load_ui(GTK_WIDGET(window), &priv->actions, ppg_window_ui,
	                 "/menubar", &priv->menubar,
	                 "/menubar/instrument/visualizers", &mb_visualizers,
	                 "/menubar/profiler/target/target-existing", &mb_target_existing,
	                 "/toolbar", &priv->toolbar,
	                 "/target-popup", &target_menu,
	                 "/target-popup/target-existing", &target_existing,
	                 NULL);

	ppg_window_action_set(window, "stop",
	                      "active", TRUE,
	                      "sensitive", FALSE,
	                      NULL);
	ppg_window_action_set(window,
	                      "pause", "sensitive", FALSE,
	                      NULL);
	ppg_window_action_set(window, "restart",
	                      "sensitive", FALSE,
	                      NULL);
	ppg_window_action_set(window, "cut",
	                      "sensitive", FALSE,
	                      NULL);
	ppg_window_action_set(window, "copy",
	                      "sensitive", FALSE,
	                      NULL);
	ppg_window_action_set(window, "paste",
	                      "sensitive", FALSE,
	                      NULL);
	ppg_window_action_set(window, "configure-instrument",
	                      "sensitive", FALSE,
	                      NULL);
	ppg_window_action_set(window, "next-instrument",
	                      "sensitive", FALSE,
	                      NULL);
	ppg_window_action_set(window, "previous-instrument",
	                      "sensitive", FALSE,
	                      NULL);
	ppg_window_action_set(window, "zoom-in-instrument",
	                      "sensitive", FALSE,
	                      NULL);
	ppg_window_action_set(window, "zoom-out-instrument",
	                      "sensitive", FALSE,
	                      NULL);

	vbox = g_object_new(GTK_TYPE_VBOX,
	                    "visible", TRUE,
	                    NULL);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	gtk_container_add_with_properties(GTK_CONTAINER(vbox), priv->menubar,
	                                  "expand", FALSE,
	                                  NULL);

	gtk_container_add_with_properties(GTK_CONTAINER(vbox), priv->toolbar,
	                                  "expand", FALSE,
	                                  NULL);

	priv->timer_tool_item = g_object_new(PPG_TYPE_TIMER_TOOL_ITEM,
	                                     "visible", TRUE,
	                                     "width-request", 250,
	                                     NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(priv->toolbar),
	                                  priv->timer_tool_item,
	                                  "expand", TRUE,
	                                  NULL);

	priv->target_tool_item = g_object_new(PPG_TYPE_MENU_TOOL_ITEM,
	                                      "label", _("Select target ..."),
	                                      "menu", target_menu,
	                                      "visible", TRUE,
	                                      "width-request", 175,
	                                      NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(priv->toolbar),
	                                  priv->target_tool_item,
	                                  "expand", FALSE,
	                                  "homogeneous", FALSE,
	                                  NULL);

	priv->session_view = g_object_new(PPG_TYPE_SESSION_VIEW,
	                                  "visible", TRUE,
	                                  NULL);
	gtk_container_add(GTK_CONTAINER(vbox), priv->session_view);
	g_object_bind_property(ppg_window_get_action(window, "show-data"),
	                       "active", priv->session_view, "show_data",
	                       G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
}

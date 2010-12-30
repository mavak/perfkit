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

#include "ppg-actions.h"
#include "ppg-session.h"
#include "ppg-session-view.h"
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

	GtkWidget      *menubar;
	GtkWidget      *session_view;
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


guint
ppg_window_count (void)
{
	return n_windows;
}


static void
ppg_window_set_uri (PpgWindow   *window,
                    const gchar *uri)
{
	PpgWindowPrivate *priv;
	PkConnection *connection;
	PpgSession *session;

	g_return_if_fail(PPG_IS_WINDOW(window));

	priv = window->priv;

	if (!(connection = pk_connection_new_from_uri(uri))) {
		g_critical("Invalid perfkit uri: %s", uri);
		return;
	}

	priv->uri = g_strdup(uri);
	session = g_object_new(PPG_TYPE_SESSION, "connection", connection, NULL);
	g_object_set(priv->session_view, "session", session, NULL);
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

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_window_finalize;
	object_class->get_property = ppg_window_get_property;
	object_class->set_property = ppg_window_set_property;
	g_type_class_add_private(object_class, sizeof(PpgWindowPrivate));

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

	window->priv = G_TYPE_INSTANCE_GET_PRIVATE(window, PPG_TYPE_WINDOW,
	                                           PpgWindowPrivate);
	priv = window->priv;

	ppg_util_load_ui(GTK_WIDGET(window), &priv->actions, ppg_window_ui,
	                 "/menubar", &priv->menubar,
	                 "/toolbar", &priv->toolbar,
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

	priv->session_view = g_object_new(PPG_TYPE_SESSION_VIEW,
	                                  "visible", TRUE,
	                                  NULL);
	gtk_container_add(GTK_CONTAINER(vbox), priv->session_view);
}

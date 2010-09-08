/* ppg-stop-action.c
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n.h>

#include "ppg-stop-action.h"
#include "ppg-session.h"
#include "ppg-window.h"
#include "ppg-util.h"

struct _PpgStopActionPrivate
{
	GtkWidget  *window;
	PpgSession *session;
	gboolean    block_toggled;
};

enum
{
	PROP_0,
	PROP_WIDGET,
};

G_DEFINE_TYPE(PpgStopAction, ppg_stop_action, GTK_TYPE_TOGGLE_ACTION)

static void
ppg_stop_action_toggled (GtkToggleAction *toggle)
{
	PpgStopActionPrivate *priv;
	gboolean active;

	g_return_if_fail(PPG_IS_STOP_ACTION(toggle));

	priv = PPG_STOP_ACTION(toggle)->priv;
	g_object_get(toggle, "active", &active, NULL);
	if (!priv->block_toggled) {
		if (priv->session) {
			if (active) {
				ppg_session_stop(priv->session, NULL);
			}
		}
	}
}

static void
ppg_stop_action_state_changed (PpgStopAction *action,
                               guint          state,
                               PpgSession    *session)
{
	PpgStopActionPrivate *priv;
	gboolean active;
	gboolean sensitive;

	g_return_if_fail(PPG_IS_STOP_ACTION(action));
	g_return_if_fail(PPG_IS_SESSION(session));

	priv = action->priv;

	switch (state) {
	case PPG_SESSION_STARTED:
	case PPG_SESSION_PAUSED:
		active = FALSE;
		sensitive = TRUE;
		break;
	case PPG_SESSION_STOPPED:
		active = TRUE;
		sensitive = FALSE;
		break;
	default:
		g_assert_not_reached();
		break;
	}

	priv->block_toggled = TRUE;
	g_object_set(action,
	             "active", active,
	             "sensitive", sensitive,
	             NULL);
	priv->block_toggled = FALSE;
}

static void
ppg_stop_action_set_widget (PpgStopAction *action,
                            GtkWidget     *widget)
{
	PpgStopActionPrivate *priv;

	g_return_if_fail(PPG_IS_STOP_ACTION(action));
	g_return_if_fail(GTK_IS_WIDGET(widget));
	g_return_if_fail(action->priv->window == NULL);

	priv = action->priv;
	priv->window = widget;
	g_object_get(priv->window,
	             "session", &priv->session,
	             NULL);
	g_signal_connect_swapped(priv->session, "state-changed",
	                         G_CALLBACK(ppg_stop_action_state_changed),
	                         action);
}

static void
ppg_stop_action_finalize (GObject *object)
{
	PpgStopActionPrivate *priv = PPG_STOP_ACTION(object)->priv;

	if (priv->session) {
		g_object_unref(priv->session);
	}

	G_OBJECT_CLASS(ppg_stop_action_parent_class)->finalize(object);
}

static void
ppg_stop_action_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
	PpgStopActionPrivate *priv;

	priv = PPG_STOP_ACTION(object)->priv;
	switch (prop_id) {
	case PROP_WIDGET:
		g_value_set_object(value, priv->window);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
ppg_stop_action_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
	switch (prop_id) {
	case PROP_WIDGET:
		ppg_stop_action_set_widget(PPG_STOP_ACTION(object),
		                           g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static GObject*
ppg_stop_action_constructor (GType                  type,
                             guint                  n_props,
                             GObjectConstructParam *props)
{
	GObject *object;

	object = PARENT_CTOR(ppg_stop_action)(type, n_props, props);
	g_object_set(object,
	             "active", TRUE,
	             "icon-name", GTK_STOCK_MEDIA_STOP,
	             "label", _("Stop"),
	             "name", "StopAction",
	             "sensitive", FALSE,
	             "tooltip", _("Stop this profiling session"),
	             NULL);
	return object;
}

static void
ppg_stop_action_class_init (PpgStopActionClass *klass)
{
	GObjectClass *object_class;
	GtkToggleActionClass *toggle_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->constructor = ppg_stop_action_constructor;
	object_class->finalize = ppg_stop_action_finalize;
	object_class->get_property = ppg_stop_action_get_property;
	object_class->set_property = ppg_stop_action_set_property;
	g_type_class_add_private(object_class, sizeof(PpgStopActionPrivate));

	toggle_class = GTK_TOGGLE_ACTION_CLASS(klass);
	toggle_class->toggled = ppg_stop_action_toggled;

	g_object_class_install_property(object_class,
	                                PROP_WIDGET,
	                                g_param_spec_object("widget",
	                                                    "widget",
	                                                    "widget",
	                                                    GTK_TYPE_WIDGET,
	                                                    G_PARAM_READWRITE));
}

static void
ppg_stop_action_init (PpgStopAction *action)
{
	INIT_PRIV(action, STOP_ACTION, StopAction);
}
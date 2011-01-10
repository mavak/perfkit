/* ppg-hud-window.c
 *
 * Copyright (C) 2011 Christian Hergert <chris@dronelabs.com>
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

#include "ppg-hud-window.h"
#include "ppg-util.h"


struct _PpgHudWindowPrivate
{
	GtkAction *actions;
};


enum
{
	PROP_0,
	PROP_ACTION_GROUP,
};


G_DEFINE_TYPE(PpgHudWindow, ppg_hud_window, GTK_TYPE_WINDOW)


static void
ppg_hud_window_set_action_group (PpgHudWindow   *window,
                                 GtkActionGroup *actions)
{
	PpgHudWindowPrivate *priv;
	GtkAction *action;
	GtkWidget *b;
	GtkWidget *hbox;
	GtkWidget *l;

	g_return_if_fail(PPG_IS_HUD_WINDOW(window));
	g_return_if_fail(GTK_IS_ACTION_GROUP(actions));

	priv = window->priv;

	priv->actions = g_object_ref(actions);

	hbox = g_object_new(GTK_TYPE_HBOX,
	                    "spacing", 3,
	                    "visible", TRUE,
	                    NULL);
	gtk_container_add(GTK_CONTAINER(window), hbox);

	action = gtk_action_group_get_action(actions, "stop");
	b = gtk_action_create_tool_item(action);
	gtk_container_add_with_properties(GTK_CONTAINER(hbox), b,
	                                  "expand", FALSE,
	                                  NULL);

	action = gtk_action_group_get_action(actions, "pause");
	b = gtk_action_create_tool_item(action);
	gtk_container_add_with_properties(GTK_CONTAINER(hbox), b,
	                                  "expand", FALSE,
	                                  NULL);

	action = gtk_action_group_get_action(actions, "run");
	b = gtk_action_create_tool_item(action);
	gtk_container_add_with_properties(GTK_CONTAINER(hbox), b,
	                                  "expand", FALSE,
	                                  NULL);

	action = gtk_action_group_get_action(actions, "restart");
	b = gtk_action_create_tool_item(action);
	gtk_container_add_with_properties(GTK_CONTAINER(hbox), b,
	                                  "expand", FALSE,
	                                  NULL);

	l = g_object_new(GTK_TYPE_LABEL,
	                 "label", "00:00:00.00",
	                 "visible", TRUE,
	                 NULL);
	gtk_container_add(GTK_CONTAINER(hbox), l);
}


/**
 * ppg_hud_window_finalize:
 * @object: (in): A #PpgHudWindow.
 *
 * Finalizer for a #PpgHudWindow instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_hud_window_finalize (GObject *object)
{
	PpgHudWindowPrivate *priv = PPG_HUD_WINDOW(object)->priv;

	ppg_clear_object(&priv->actions);

	G_OBJECT_CLASS(ppg_hud_window_parent_class)->finalize(object);
}

/**
 * ppg_hud_window_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
static void
ppg_hud_window_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
	PpgHudWindow *window = PPG_HUD_WINDOW(object);

	switch (prop_id) {
	case PROP_ACTION_GROUP:
		ppg_hud_window_set_action_group(window, g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * ppg_hud_window_class_init:
 * @klass: (in): A #PpgHudWindowClass.
 *
 * Initializes the #PpgHudWindowClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_hud_window_class_init (PpgHudWindowClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_hud_window_finalize;
	object_class->set_property = ppg_hud_window_set_property;
	g_type_class_add_private(object_class, sizeof(PpgHudWindowPrivate));

	g_object_class_install_property(object_class,
	                                PROP_ACTION_GROUP,
	                                g_param_spec_object("action-group",
	                                                    "action-group",
	                                                    "action-group",
	                                                    GTK_TYPE_ACTION_GROUP,
	                                                    G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

/**
 * ppg_hud_window_init:
 * @window: (in): A #PpgHudWindow.
 *
 * Initializes the newly created #PpgHudWindow instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_hud_window_init (PpgHudWindow *window)
{
	window->priv = G_TYPE_INSTANCE_GET_PRIVATE(window, PPG_TYPE_HUD_WINDOW,
	                                           PpgHudWindowPrivate);

	g_object_set(window,
	             "border-width", 12,
	             "decorated", FALSE,
	             "resizable", FALSE,
	             NULL);
}

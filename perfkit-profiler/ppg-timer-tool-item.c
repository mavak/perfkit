/* ppg-timer-tool-item.c
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

#include "ppg-log.h"
#include "ppg-session.h"
#include "ppg-timer-tool-item.h"

#define MAX_TIMER_CHARS 11

G_DEFINE_TYPE(PpgTimerToolItem, ppg_timer_tool_item, GTK_TYPE_TOOL_ITEM)

struct _PpgTimerToolItemPrivate
{
	PpgSession *session;
	GtkWidget  *button;
	GtkWidget  *label;
};

enum
{
	PROP_0,
	PROP_SESSION,
};


static void
ppg_timer_tool_item_format (gchar *formatted,
                            gsize n,
                            gdouble position)
{
	gint h;
	gint m;
	gint s;
	gint hu;

	h = position / 3600.0;
	position -= h * 3600.0;

	m = position / 60.0;
	position -= m * 60.0;

	s = position;
	position -= s;

	hu = position * 100;

	g_snprintf(formatted, n, "%02d:%02d:%02d.%02d", h, m, s, hu);
	formatted[n - 1] = '\0';
}


static void
ppg_timer_tool_item_notify_elapsed (PpgTimerToolItem *item,
                                    GParamSpec       *pspec,
                                    PpgSession       *session)
{
	PpgTimerToolItemPrivate *priv;
	gchar formatted[MAX_TIMER_CHARS + 1];
	gdouble elapsed;

	g_return_if_fail(PPG_IS_TIMER_TOOL_ITEM(item));
	g_return_if_fail(PPG_IS_SESSION(session));

	priv = item->priv;

	elapsed = ppg_session_get_elapsed(session);
	ppg_timer_tool_item_format(formatted, sizeof formatted, elapsed);
	gtk_label_set_label(GTK_LABEL(priv->label), formatted);
}


static void
ppg_timer_tool_item_notify_state (PpgTimerToolItem *item,
                                  GParamSpec       *pspec,
                                  PpgSession       *session)
{
	PpgTimerToolItemPrivate *priv;
	GdkColor color;

	g_return_if_fail(PPG_IS_TIMER_TOOL_ITEM(item));

	priv = item->priv;

	switch (ppg_session_get_state(session)) {
	case PPG_SESSION_INITIAL:
	case PPG_SESSION_READY:
	case PPG_SESSION_STOPPED:
		gtk_widget_modify_base(priv->button, GTK_STATE_NORMAL, NULL);
		break;
	case PPG_SESSION_FAILED:
	case PPG_SESSION_STARTED:
		gdk_color_parse("#cc66666", &color);
		gtk_widget_modify_base(priv->button, GTK_STATE_NORMAL, &color);
		break;
	case PPG_SESSION_MUTED:
		gdk_color_parse("#ee9a77", &color);
		gtk_widget_modify_base(priv->button, GTK_STATE_NORMAL, &color);
		break;
	default:
		g_assert_not_reached();
		return;
	}
}

static void
ppg_timer_tool_item_set_session (PpgTimerToolItem *item,
                                 PpgSession       *session)
{
	PpgTimerToolItemPrivate *priv;

	g_return_if_fail(PPG_IS_TIMER_TOOL_ITEM(item));

	priv = item->priv;
	priv->session = session;
	g_signal_connect_swapped(session, "notify::elapsed",
	                         G_CALLBACK(ppg_timer_tool_item_notify_elapsed),
	                         item);
	g_signal_connect_swapped(session, "notify::state",
	                         G_CALLBACK(ppg_timer_tool_item_notify_state),
	                         item);
}

/**
 * ppg_timer_tool_item_finalize:
 * @object: (in): A #PpgTimerToolItem.
 *
 * Finalizer for a #PpgTimerToolItem instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_timer_tool_item_finalize (GObject *object)
{
	G_OBJECT_CLASS(ppg_timer_tool_item_parent_class)->finalize(object);
}

/**
 * ppg_timer_tool_item_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
static void
ppg_timer_tool_item_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
	PpgTimerToolItem *item = PPG_TIMER_TOOL_ITEM(object);

	switch (prop_id) {
	case PROP_SESSION:
		ppg_timer_tool_item_set_session(item, g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * ppg_timer_tool_item_class_init:
 * @klass: (in): A #PpgTimerToolItemClass.
 *
 * Initializes the #PpgTimerToolItemClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_timer_tool_item_class_init (PpgTimerToolItemClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_timer_tool_item_finalize;
	object_class->set_property = ppg_timer_tool_item_set_property;
	g_type_class_add_private(object_class, sizeof(PpgTimerToolItemPrivate));

	g_object_class_install_property(object_class,
	                                PROP_SESSION,
	                                g_param_spec_object("session",
	                                                    "session",
	                                                    "session",
	                                                    PPG_TYPE_SESSION,
	                                                    G_PARAM_WRITABLE));
}

/**
 * ppg_timer_tool_item_init:
 * @item: (in): A #PpgTimerToolItem.
 *
 * Initializes the newly created #PpgTimerToolItem instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_timer_tool_item_init (PpgTimerToolItem *item)
{
	PpgTimerToolItemPrivate *priv;
	GtkStyleContext *style_context;
	PangoAttrList *attrs;
	GtkWidget *align;
	GtkWidget *hbox;

	priv = G_TYPE_INSTANCE_GET_PRIVATE(item, PPG_TYPE_TIMER_TOOL_ITEM,
	                                   PpgTimerToolItemPrivate);
	item->priv = priv;

	align = g_object_new(GTK_TYPE_ALIGNMENT,
	                     "xalign", 0.5f,
	                     "xscale", 0.0f,
	                     "yalign", 0.5f,
	                     "yscale", 0.0f,
	                     "visible", TRUE,
	                     NULL);
	gtk_container_add(GTK_CONTAINER(item), align);

	hbox = g_object_new(GTK_TYPE_HBOX,
	                    "visible", TRUE,
	                    NULL);
	gtk_container_add(GTK_CONTAINER(align), hbox);

	priv->button = g_object_new(GTK_TYPE_BUTTON,
	                            "height-request", 32,
	                            "sensitive", FALSE,
	                            "visible", TRUE,
	                            NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(hbox), priv->button,
	                                  "expand", FALSE,
	                                  NULL);

	style_context = gtk_widget_get_style_context(priv->button);
	gtk_style_context_set_state(style_context, GTK_STATE_FLAG_NORMAL);

	attrs = pango_attr_list_new();
	pango_attr_list_insert(attrs, pango_attr_size_new(12 * PANGO_SCALE));
	pango_attr_list_insert(attrs, pango_attr_family_new("Monospace"));
	pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
	priv->label = g_object_new(GTK_TYPE_LABEL,
	                           "attributes", attrs,
	                           "label", "00:00:00.00",
	                           "selectable", TRUE,
	                           "single-line-mode", TRUE,
	                           "use-markup", FALSE,
	                           "visible", TRUE,
	                           "xalign", 0.5f,
	                           "yalign", 0.5f,
	                           NULL);
	gtk_container_add(GTK_CONTAINER(priv->button), priv->label);
	pango_attr_list_unref(attrs);
}

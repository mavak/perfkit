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
	GtkWidget  *drawing;
	GtkWidget  *offscreen;
	GtkWidget  *button;
	GtkWidget  *label;
};

enum
{
	PROP_0,
	PROP_SESSION,
};


static void
ppg_timer_tool_item_draw (GtkWidget        *widget,
                          cairo_t          *cr,
                          PpgTimerToolItem *item)
{
	PpgTimerToolItemPrivate *priv;
#if !GTK_CHECK_VERSION(2, 91, 0)
	GtkAllocation a;
	GdkPixmap *pixmap;
#endif

	g_return_if_fail(PPG_IS_TIMER_TOOL_ITEM(item));

	priv = item->priv;

#if GTK_CHECK_VERSION(2, 91, 0)
	gtk_widget_draw(priv->offscreen, cr);
#else
	gtk_widget_get_allocation(widget, &a);
	pixmap = gtk_offscreen_window_get_pixmap(GTK_OFFSCREEN_WINDOW(priv->offscreen));
	gdk_cairo_set_source_pixmap(cr, pixmap, 0, 0);
	cairo_rectangle(cr, 0, 0, a.width, a.height);
	cairo_fill(cr);
#endif
}


#if !GTK_CHECK_VERSION(2, 91, 0)
static void
ppg_timer_tool_item_expose_event (GtkWidget        *widget,
                                  GdkEventExpose   *expose,
                                  PpgTimerToolItem *item)
{
	cairo_t *cr;

	cr = gdk_cairo_create(expose->window);
	gdk_cairo_rectangle(cr, &expose->area);
	cairo_clip(cr);
	ppg_timer_tool_item_draw(widget, cr, item);
	cairo_destroy(cr);
}
#endif


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
ppg_timer_tool_item_notify_elapsed (PpgSession       *session,
                                    GParamSpec       *pspec,
                                    PpgTimerToolItem *item)
{
	PpgTimerToolItemPrivate *priv;
	gchar formatted[MAX_TIMER_CHARS + 1];
	gdouble elapsed;

	g_return_if_fail(PPG_IS_TIMER_TOOL_ITEM(item));

	priv = item->priv;

	elapsed = ppg_session_get_elapsed(session);
	ppg_timer_tool_item_format(formatted, sizeof formatted, elapsed);
	gtk_label_set_label(GTK_LABEL(priv->label), formatted);
	gtk_widget_queue_draw(GTK_WIDGET(priv->drawing));
}


static void
ppg_timer_tool_item_notify_state (PpgSession       *session,
                                  GParamSpec       *pspec,
                                  PpgTimerToolItem *item)
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

	gtk_widget_queue_draw(priv->drawing);
}


static void
ppg_timer_tool_item_size_allocate (GtkWidget     *widget,
                                   GtkAllocation *alloc)
{
	PpgTimerToolItemPrivate *priv;
	GtkAllocation a;

	GTK_WIDGET_CLASS(ppg_timer_tool_item_parent_class)->
		size_allocate(widget, alloc);

	priv = PPG_TIMER_TOOL_ITEM(widget)->priv;

	gtk_widget_get_allocation(priv->drawing, &a);
	gtk_widget_size_allocate(priv->offscreen, &a);
	gtk_widget_queue_draw(priv->offscreen);
}


static void
ppg_timer_tool_item_set_session (PpgTimerToolItem *item,
                                 PpgSession       *session)
{
	PpgTimerToolItemPrivate *priv;

	g_return_if_fail(PPG_IS_TIMER_TOOL_ITEM(item));

	priv = item->priv;
	priv->session = session;

	g_signal_connect(session,
	                 "notify::elapsed",
	                 G_CALLBACK(ppg_timer_tool_item_notify_elapsed),
	                 item);
	g_signal_connect_swapped(session, "notify::state",
	                         G_CALLBACK(ppg_timer_tool_item_notify_state),
	                         item);
}

static gboolean
ppg_timer_tool_item_offscreen_damage (PpgTimerToolItem *item,
                                      GdkEventExpose   *expose,
                                      GtkWidget        *offscreen)
{
	g_return_val_if_fail(PPG_IS_TIMER_TOOL_ITEM(item), FALSE);
	gtk_widget_queue_draw_area(item->priv->drawing,
	                           expose->area.x, expose->area.y,
	                           expose->area.width, expose->area.height);
	return FALSE;
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
	GtkWidgetClass *widget_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_timer_tool_item_finalize;
	object_class->set_property = ppg_timer_tool_item_set_property;
	g_type_class_add_private(object_class, sizeof(PpgTimerToolItemPrivate));

	widget_class = GTK_WIDGET_CLASS(klass);
	widget_class->size_allocate = ppg_timer_tool_item_size_allocate;

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
	GtkWidget *align;
	GtkWidget *hbox;
	PangoAttrList *attrs;

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

	priv->drawing = g_object_new(GTK_TYPE_DRAWING_AREA,
	                             "height-request", 32,
	                             "visible", TRUE,
	                             "width-request", 180,
	                             NULL);
	gtk_container_add(GTK_CONTAINER(hbox), priv->drawing);

#if GTK_CHECK_VERSION(2, 91, 0)
	g_signal_connect(priv->drawing, "draw",
	                 G_CALLBACK(ppg_timer_tool_item_draw),
	                 item);
#else
	g_signal_connect(priv->drawing, "expose-event",
	                 G_CALLBACK(ppg_timer_tool_item_expose_event),
	                 item);
#endif

	priv->offscreen = g_object_new(GTK_TYPE_OFFSCREEN_WINDOW,
	                               "height-request", 32,
	                               "visible", TRUE,
	                               "width-request", 180,
	                               NULL);
	g_signal_connect_swapped(priv->offscreen, "damage-event",
	                         G_CALLBACK(ppg_timer_tool_item_offscreen_damage),
	                         item);

	priv->button = g_object_new(GTK_TYPE_BUTTON,
	                            "height-request", 32,
	                            "visible", TRUE,
	                            NULL);
	gtk_container_add(GTK_CONTAINER(priv->offscreen), priv->button);

	attrs = pango_attr_list_new();
	pango_attr_list_insert(attrs, pango_attr_size_new(12 * PANGO_SCALE));
	pango_attr_list_insert(attrs, pango_attr_family_new("Monospace"));
	pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
	priv->label = g_object_new(GTK_TYPE_LABEL,
	                           "attributes", attrs,
	                           "label", "00:00:00.00",
	                           "selectable", TRUE,
	                           "use-markup", FALSE,
	                           "visible", TRUE,
	                           "xalign", 0.5f,
	                           "yalign", 0.5f,
	                           NULL);
	gtk_container_add(GTK_CONTAINER(priv->button), priv->label);
	pango_attr_list_unref(attrs);
}

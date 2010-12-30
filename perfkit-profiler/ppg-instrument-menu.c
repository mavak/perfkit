/* ppg-instrument-menu.c
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

#include "ppg-instrument.h"
#include "ppg-instrument-menu.h"
#include "ppg-instrument-view.h"
#include "ppg-util.h"


G_DEFINE_TYPE(PpgInstrumentMenu, ppg_instrument_menu, GTK_TYPE_MENU)


struct _PpgInstrumentMenuPrivate
{
	PpgInstrumentView *instrument_view;
	GtkWidget         *compact_menu;
	GtkWidget         *mute_menu;
	GtkWidget         *visualizers_menu;
};


enum
{
	PROP_0,
	PROP_INSTRUMENT_VIEW,
};


static GQuark
ppg_instrument_menu_visualizer_quark (void)
{
	return g_quark_from_static_string("visualizer-name");
}


static gboolean
visualizer_is_active (GList       *list,
                      const gchar *name)
{
	GList *iter;

	for (iter = list; iter; iter = iter->next) {
		if (!g_strcmp0(ppg_visualizer_get_name(iter->data), name)) {
			return TRUE;
		}
	}
	return FALSE;
}


static void
ppg_instrument_menu_visualizer_activate (PpgInstrumentMenu *menu,
                                         GtkWidget         *menu_item)
{
	PpgInstrumentMenuPrivate *priv;
	PpgInstrument *instrument;
	const gchar *name;
	gboolean active;

	g_return_if_fail(PPG_IS_INSTRUMENT_MENU(menu));

	priv = menu->priv;

	name = g_object_get_qdata(G_OBJECT(menu_item),
	                          ppg_instrument_menu_visualizer_quark());
	instrument = ppg_instrument_view_get_instrument(priv->instrument_view);

	g_object_get(menu_item, "active", &active, NULL);
	if (active) {
		ppg_instrument_add_visualizer(instrument, name);
	} else {
		/*
		 * TODO: Remove a visualizer.
		 */
	}
}


/**
 * ppg_instrument_menu_set_instrument_view:
 * @menu: (in): A #PpgInstrumentMenu.
 * @instrument_view: (in): A #PpgInstrumentView.
 *
 * Set the instrument view for the menu.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_instrument_menu_set_instrument_view (PpgInstrumentMenu *menu,
                                         PpgInstrumentView *instrument_view)
{
	PpgInstrumentMenuPrivate *priv;
	PpgVisualizerEntry *entries;
	PpgInstrument *instrument;
	GtkWidget *menu_item;
	GtkWidget *submenu;
	gboolean active;
	GList *list;
	gsize n_entries;
	gint i;

	g_return_if_fail(PPG_IS_INSTRUMENT_MENU(menu));

	priv = menu->priv;

	priv->instrument_view = g_object_ref(instrument_view);
	instrument = ppg_instrument_view_get_instrument(instrument_view);

	/*
	 * Bind various check menu items to properties.
	 */
	g_object_bind_property(priv->instrument_view, "frozen",
	                       priv->mute_menu, "active",
	                       G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
	g_object_bind_property(priv->instrument_view, "compact",
	                       priv->compact_menu, "active",
	                       G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

	/*
	 * Build visualizers menu.
	 */
	submenu = g_object_new(GTK_TYPE_MENU, "visible", TRUE, NULL);
	g_object_set(priv->visualizers_menu, "submenu", submenu, NULL);
	entries = ppg_instrument_get_entries(instrument, &n_entries);
	list = ppg_instrument_get_visualizers(instrument);
	for (i = 0; i < n_entries; i++) {
		active = visualizer_is_active(list, entries[i].name);
		menu_item = g_object_new(GTK_TYPE_CHECK_MENU_ITEM,
		                         "label", entries[i].title,
		                         "visible", TRUE,
		                         "active", active,
		                         NULL);
		g_object_set_qdata_full(G_OBJECT(menu_item),
		                        ppg_instrument_menu_visualizer_quark(),
		                        g_strdup(entries[i].name), g_free);
		gtk_menu_shell_append(GTK_MENU_SHELL(submenu), menu_item);
		g_signal_connect_swapped(menu_item, "activate",
		                         G_CALLBACK(ppg_instrument_menu_visualizer_activate),
		                         menu);
	}
	g_list_free(list);
}


/**
 * ppg_instrument_menu_zoom_in:
 * @menu: (in): A #PpgInstrumentMenu.
 *
 * Zoom in the instrument view.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_instrument_menu_zoom_in (PpgInstrumentMenu *menu,
                             GtkWidget         *menu_item)
{
	ppg_instrument_view_zoom_in(menu->priv->instrument_view);
}


/**
 * ppg_instrument_menu_zoom_out:
 * @menu: (in): A #PpgInstrumentMenu.
 *
 * Zoom out the instrument view.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_instrument_menu_zoom_out (PpgInstrumentMenu *menu,
                              GtkWidget         *menu_item)
{
	ppg_instrument_view_zoom_out(menu->priv->instrument_view);
}


/**
 * ppg_instrument_menu_dispose:
 * @object: (in): A #GObject.
 *
 * Dispose callback for @object.  This method releases references held
 * by the #GObject instance.
 *
 * Returns: None.
 * Side effects: Plenty.
 */
static void
ppg_instrument_menu_dispose (GObject *object)
{
	PpgInstrumentMenuPrivate *priv = PPG_INSTRUMENT_MENU(object)->priv;

	ppg_clear_object(&priv->instrument_view);

	G_OBJECT_CLASS(ppg_instrument_menu_parent_class)->dispose(object);
}


/**
 * ppg_instrument_menu_finalize:
 * @object: (in): A #PpgInstrumentMenu.
 *
 * Finalizer for a #PpgInstrumentMenu instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_instrument_menu_finalize (GObject *object)
{
	G_OBJECT_CLASS(ppg_instrument_menu_parent_class)->finalize(object);
}


/**
 * ppg_instrument_menu_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
static void
ppg_instrument_menu_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
	PpgInstrumentMenu *menu = PPG_INSTRUMENT_MENU(object);

	switch (prop_id) {
	case PROP_INSTRUMENT_VIEW:
		ppg_instrument_menu_set_instrument_view(menu,
		                                        g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}


/**
 * ppg_instrument_menu_class_init:
 * @klass: (in): A #PpgInstrumentMenuClass.
 *
 * Initializes the #PpgInstrumentMenuClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_instrument_menu_class_init (PpgInstrumentMenuClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->dispose = ppg_instrument_menu_dispose;
	object_class->finalize = ppg_instrument_menu_finalize;
	object_class->set_property = ppg_instrument_menu_set_property;
	g_type_class_add_private(object_class, sizeof(PpgInstrumentMenuPrivate));

	g_object_class_install_property(object_class,
	                                PROP_INSTRUMENT_VIEW,
	                                g_param_spec_object("instrument-view",
	                                                    "View",
	                                                    "The instrument view",
	                                                    PPG_TYPE_INSTRUMENT_VIEW,
	                                                    G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}


/**
 * ppg_instrument_menu_init:
 * @menu: (in): A #PpgInstrumentMenu.
 *
 * Initializes the newly created #PpgInstrumentMenu instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_instrument_menu_init (PpgInstrumentMenu *menu)
{
	PpgInstrumentMenuPrivate *priv;
	GtkWidget *menu_item;

	menu->priv = G_TYPE_INSTANCE_GET_PRIVATE(menu,
	                                         PPG_TYPE_INSTRUMENT_MENU,
	                                         PpgInstrumentMenuPrivate);
	priv = menu->priv;

	priv->mute_menu = g_object_new(GTK_TYPE_CHECK_MENU_ITEM,
	                               "label", _("Mute"),
	                               "visible", TRUE,
	                               NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), priv->mute_menu);

	priv->compact_menu = g_object_new(GTK_TYPE_CHECK_MENU_ITEM,
	                                 "label", _("Compact mode"),
	                                 "visible", TRUE,
	                                 NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), priv->compact_menu);

	menu_item = g_object_new(GTK_TYPE_MENU_ITEM,
	                         "label", _("Configure"),
	                         "visible", TRUE,
	                         NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = g_object_new(GTK_TYPE_SEPARATOR_MENU_ITEM,
	                         "visible", TRUE,
	                         NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	menu_item = g_object_new(GTK_TYPE_MENU_ITEM,
	                         "label", _("Zoom In"),
	                         "visible", TRUE,
	                         NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect_swapped(menu_item, "activate",
	                         G_CALLBACK(ppg_instrument_menu_zoom_in),
	                         menu);

	menu_item = g_object_new(GTK_TYPE_MENU_ITEM,
	                         "label", _("Zoom Out"),
	                         "visible", TRUE,
	                         NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
	g_signal_connect_swapped(menu_item, "activate",
	                         G_CALLBACK(ppg_instrument_menu_zoom_out),
	                         menu);

	menu_item = g_object_new(GTK_TYPE_SEPARATOR_MENU_ITEM,
	                         "visible", TRUE,
	                         NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

	priv->visualizers_menu = g_object_new(GTK_TYPE_MENU_ITEM,
	                                      "label", _("Visualizers"),
	                                      "visible", TRUE,
	                                      NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), priv->visualizers_menu);
}

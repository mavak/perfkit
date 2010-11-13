/* ppg-discover-dialog.c
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

#include <avahi-gobject/ga-client.h>
#include <avahi-gobject/ga-service-browser.h>
#include <avahi-gobject/ga-service-resolver.h>
#include <glib/gi18n.h>

#include "ppg-discover-dialog.h"
#include "ppg-log.h"

G_DEFINE_TYPE(PpgDiscoverDialog, ppg_discover_dialog, GTK_TYPE_DIALOG)

#define SERVICE_TYPE "_workstation._tcp" /* FIXME */

struct _PpgDiscoverDialogPrivate
{
	GtkListStore *model;
	GtkWidget    *ok_button;
	GtkWidget    *spinner;
	GtkWidget    *uri_entry;
	gchar        *uri;

	GaClient          *client;
	GaServiceBrowser  *browser;
	GaServiceResolver *resolver;
};

enum
{
	PROP_0,
	PROP_URI,
};

static void
ppg_discover_dialog_set_uri (PpgDiscoverDialog *dialog,
                             const gchar       *uri)
{
	PpgDiscoverDialogPrivate *priv;

	g_return_if_fail(PPG_IS_DISCOVER_DIALOG(dialog));
	g_return_if_fail(uri != NULL);

	priv = dialog->priv;
}

static void
ppg_discover_dialog_found_cb (GaServiceResolver   *resolver,
                              AvahiIfIndex         iface,
                              GaProtocol           proto,
                              const gchar         *name,
                              const gchar         *type,
                              const gchar         *domain,
                              const gchar         *host_name,
                              AvahiAddress        *address,
                              gint                 port,
                              AvahiStringList     *txt,
                              GaLookupResultFlags  flags,
                              PpgDiscoverDialog   *dialog)
{
	PpgDiscoverDialogPrivate *priv;
	gchar addr[AVAHI_ADDRESS_STR_MAX] = { 0 };
	GtkTreeIter iter;

	g_return_if_fail(PPG_IS_DISCOVER_DIALOG(dialog));

	priv = dialog->priv;

	avahi_address_snprint(addr, sizeof addr, address);
	gtk_list_store_append(priv->model, &iter);
	gtk_list_store_set(priv->model, &iter,
	                   0, host_name,
	                   1, addr,
	                   -1);
	g_object_unref(resolver);
}

static void
ppg_discover_dialog_failure_cb (GaServiceResolver *resolver,
                                const GError      *error,
                                PpgDiscoverDialog *dialog)
{
	WARNING(Avahi, "%s", error->message);
	g_object_unref(resolver);
}

static void
ppg_discover_dialog_new_service_cb (GaServiceBrowser    *browser,
                                    AvahiIfIndex         iface,
                                    GaProtocol           proto,
                                    const gchar         *name,
                                    const gchar         *type,
                                    const gchar         *domain,
                                    GaLookupResultFlags  flags,
                                    PpgDiscoverDialog   *dialog)
{
	PpgDiscoverDialogPrivate *priv;
	GaServiceResolver *resolver;
	GError *error = NULL;

	g_return_if_fail(PPG_IS_DISCOVER_DIALOG(dialog));

	priv = dialog->priv;

	resolver = ga_service_resolver_new(iface,
	                                   proto,
	                                   name,
	                                   type,
	                                   domain,
	                                   GA_PROTOCOL_UNSPEC,
	                                   GA_LOOKUP_NO_FLAGS);
	g_signal_connect(resolver, "found",
	                 G_CALLBACK(ppg_discover_dialog_found_cb),
	                 dialog);
	g_signal_connect(resolver, "failure",
	                 G_CALLBACK(ppg_discover_dialog_failure_cb),
	                 dialog);

	if (!ga_service_resolver_attach(resolver, priv->client, &error)) {
		WARNING(Avahi, "Failed to resolve Avahi hostname: %s", error->message);
		g_error_free(error);
	}
}

static void
ppg_discover_dialog_discover (PpgDiscoverDialog *dialog)
{
	PpgDiscoverDialogPrivate *priv;
	GError *error = NULL;

	ENTRY;

	g_return_if_fail(PPG_IS_DISCOVER_DIALOG(dialog));
	g_return_if_fail(!dialog->priv->client);

	priv = dialog->priv;

	priv->client = g_object_new(GA_TYPE_CLIENT, NULL);
	if (!ga_client_start(priv->client, &error)) {
		CRITICAL(Avahi, "%s", error->message);
		g_error_free(error);
		EXIT;
	}

	priv->browser = ga_service_browser_new((gchar *)SERVICE_TYPE);
	g_signal_connect(priv->browser, "new-service",
	                 G_CALLBACK(ppg_discover_dialog_new_service_cb), dialog);
	if (!ga_service_browser_attach(priv->browser, priv->client, &error)) {
		CRITICAL(Avahi, "%s", error->message);
		g_error_free(error);
		EXIT;
	}

	EXIT;
}

static void
ppg_discover_dialog_selection_changed (GtkTreeSelection  *selection,
                                       PpgDiscoverDialog *dialog)
{
	PpgDiscoverDialogPrivate *priv;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *addr;
	gint count;

	g_return_if_fail(PPG_IS_DISCOVER_DIALOG(dialog));

	priv = dialog->priv;

	count = gtk_tree_selection_count_selected_rows(selection);
	gtk_widget_set_sensitive(priv->ok_button, (count > 0));

	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		gtk_tree_model_get(model, &iter,
		                   1, &addr,
		                   -1);
		g_free(priv->uri);
		priv->uri = g_strdup_printf("perfkit://%s:9929", addr);
		g_free(addr);
	} else {
		g_free(priv->uri);
		priv->uri = NULL;
	}

	gtk_entry_set_text(GTK_ENTRY(priv->uri_entry), priv->uri);
}

/**
 * ppg_discover_dialog_finalize:
 * @object: (in): A #PpgDiscoverDialog.
 *
 * Finalizer for a #PpgDiscoverDialog instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_discover_dialog_finalize (GObject *object)
{
	PpgDiscoverDialogPrivate *priv = PPG_DISCOVER_DIALOG(object)->priv;

	g_free(priv->uri);
	g_object_unref(priv->model);

	G_OBJECT_CLASS(ppg_discover_dialog_parent_class)->finalize(object);
}

/**
 * ppg_discover_dialog_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (out): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Get a given #GObject property.
 */
static void
ppg_discover_dialog_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
	PpgDiscoverDialog *dialog = PPG_DISCOVER_DIALOG(object);

	switch (prop_id) {
	case PROP_URI:
		g_value_set_string(value, dialog->priv->uri);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * ppg_discover_dialog_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
static void
ppg_discover_dialog_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
	PpgDiscoverDialog *dialog = PPG_DISCOVER_DIALOG(object);

	switch (prop_id) {
	case PROP_URI:
		ppg_discover_dialog_set_uri(dialog, g_value_get_string(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * ppg_discover_dialog_class_init:
 * @klass: (in): A #PpgDiscoverDialogClass.
 *
 * Initializes the #PpgDiscoverDialogClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_discover_dialog_class_init (PpgDiscoverDialogClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_discover_dialog_finalize;
	object_class->get_property = ppg_discover_dialog_get_property;
	object_class->set_property = ppg_discover_dialog_set_property;
	g_type_class_add_private(object_class, sizeof(PpgDiscoverDialogPrivate));

	g_object_class_install_property(object_class,
	                                PROP_URI,
	                                g_param_spec_string("uri",
	                                                    "uri",
	                                                    "The host uri",
	                                                    NULL,
	                                                    G_PARAM_READWRITE));
}

/**
 * ppg_discover_dialog_init:
 * @dialog: (in): A #PpgDiscoverDialog.
 *
 * Initializes the newly created #PpgDiscoverDialog instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_discover_dialog_init (PpgDiscoverDialog *dialog)
{
	PpgDiscoverDialogPrivate *priv;
	GtkTreeSelection *selection;
	GtkTreeViewColumn *column;
	GtkCellRenderer *cell;
	GtkWidget *action;
	GtkWidget *align;
	GtkWidget *box;
	GtkWidget *content;
	GtkWidget *l;
	GtkWidget *notebook;
	GtkWidget *scroller;
	GtkWidget *treeview;

	priv = G_TYPE_INSTANCE_GET_PRIVATE(dialog, PPG_TYPE_DISCOVER_DIALOG,
	                                   PpgDiscoverDialogPrivate);
	dialog->priv = priv;

	g_object_set(dialog,
	             "border-width", 6,
	             "default-height", 300,
	             "default-width", 350,
	             "title", _("Connect to Agent"),
	             NULL);

	action = gtk_dialog_get_action_area(GTK_DIALOG(dialog));
	content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	g_object_set(content,
	             "border-width", 6,
	             "spacing", 12,
	             NULL);

	priv->uri_entry = g_object_new(GTK_TYPE_ENTRY,
	                               "visible", TRUE,
	                               "editable", FALSE,
	                               NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(content), priv->uri_entry,
	                                  "expand", FALSE,
	                                  NULL);

	notebook = g_object_new(GTK_TYPE_NOTEBOOK,
	                        "visible", TRUE,
	                        NULL);
	gtk_container_add(GTK_CONTAINER(content), notebook);

	l = g_object_new(GTK_TYPE_LABEL,
	                 "label", _("_Discover"),
	                 "use-underline", TRUE,
	                 "visible", TRUE,
	                 NULL);
	box = g_object_new(GTK_TYPE_VBOX,
	                   "visible", TRUE,
	                   NULL);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), box, l);

	scroller = g_object_new(GTK_TYPE_SCROLLED_WINDOW,
	                        "hscrollbar-policy", GTK_POLICY_AUTOMATIC,
	                        "shadow-type", GTK_SHADOW_IN,
	                        "visible", TRUE,
	                        "vscrollbar-policy", GTK_POLICY_AUTOMATIC,
	                        NULL);
	gtk_container_add(GTK_CONTAINER(box), scroller);

	priv->model = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);

	treeview = g_object_new(GTK_TYPE_TREE_VIEW,
	                        "model", priv->model,
	                        "visible", TRUE,
	                        NULL);
	gtk_container_add(GTK_CONTAINER(scroller), treeview);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	g_signal_connect(selection, "changed",
	                 G_CALLBACK(ppg_discover_dialog_selection_changed),
	                 dialog);

	column = g_object_new(GTK_TYPE_TREE_VIEW_COLUMN,
	                      "expand", TRUE,
	                      "title", _("Hostname"),
	                      NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

	cell = g_object_new(GTK_TYPE_CELL_RENDERER_TEXT,
	                    "ellipsize", PANGO_ELLIPSIZE_MIDDLE,
	                    NULL);
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(column), cell, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(column), cell, "text", 0);

	column = g_object_new(GTK_TYPE_TREE_VIEW_COLUMN,
	                      "expand", TRUE,
	                      "title", _("Address"),
	                      NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

	cell = g_object_new(GTK_TYPE_CELL_RENDERER_TEXT,
	                    "ellipsize", PANGO_ELLIPSIZE_MIDDLE,
	                    NULL);
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(column), cell, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(column), cell, "text", 1);

	align = g_object_new(GTK_TYPE_ALIGNMENT,
	                     "xalign", 1.0f,
	                     "xscale", 0.0f,
	                     "yalign", 0.5f,
	                     "yscale", 0.0f,
	                     "visible", TRUE,
	                     NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(action), align,
	                                  "expand", FALSE,
	                                  NULL);

	priv->spinner = g_object_new(GTK_TYPE_SPINNER,
	                             "height-request", 16,
	                             "visible", FALSE,
	                             "width-request", 16,
	                             NULL);
	gtk_container_add(GTK_CONTAINER(align), priv->spinner);

	gtk_dialog_add_button(GTK_DIALOG(dialog),
	                      GTK_STOCK_CANCEL,
	                      GTK_RESPONSE_CANCEL);
	priv->ok_button = gtk_dialog_add_button(GTK_DIALOG(dialog),
	                                        GTK_STOCK_CONNECT,
	                                        GTK_RESPONSE_OK);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
	gtk_widget_set_sensitive(priv->ok_button, FALSE);

	ppg_discover_dialog_discover(dialog);

	gtk_window_set_focus(GTK_WINDOW(dialog), treeview);
}

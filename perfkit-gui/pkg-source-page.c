/* pkg-source-page.c
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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n.h>

#include "pkg-log.h"
#include "pkg-page.h"
#include "pkg-path.h"
#include "pkg-source-page.h"

/**
 * SECTION:pkg-source-page
 * @title: PkgSourcePage
 * @short_description:
 *
 * Section overview.
 */

static void pkg_page_init (PkgPageIface *iface);

G_DEFINE_TYPE_EXTENDED(PkgSourcePage,
                       pkg_source_page,
                       GTK_TYPE_ALIGNMENT,
                       0,
                       G_IMPLEMENT_INTERFACE(PKG_TYPE_PAGE, pkg_page_init));

struct _PkgSourcePagePrivate
{
	PkConnection *connection;
	gint          id;        /* Source id. */
	gint          sub_id;    /* Subscription id for info. */
	GtkBuilder   *builder;
	GtkListStore *model;
	GtkWidget    *container;
	GtkWidget    *title;
	GtkWidget    *treeview;
};

enum
{
	PROP_0,
	PROP_CONNECTION,
	PROP_ID,
};

/**
 * pkg_source_page_new:
 *
 * Creates a new instance of #PkgSourcePage.
 *
 * Returns: the newly created instance of #PkgSourcePage.
 * Side effects: None.
 */
GtkWidget*
pkg_source_page_new (PkConnection *connection, /* IN */
                     gint          source)     /* IN */
{
	PkgSourcePage *page;

	ENTRY;
	page = g_object_new(PKG_TYPE_SOURCE_PAGE,
	                    "connection", connection,
	                    "id", source,
	                    NULL);
	RETURN(GTK_WIDGET(page));
}

static void
pkg_source_page_manifest_cb (PkManifest *manifest,  /* IN */
                             gpointer    user_data) /* IN */
{
	PkgSourcePagePrivate *priv;
	PkgSourcePage *page = user_data;
	GtkTreeIter iter;
	const gchar *name;
	const gchar *type;
	gint n_rows;
	gint i;

	g_return_if_fail(PKG_IS_SOURCE_PAGE(page));

	ENTRY;
	priv = page->priv;
	DEBUG(SourcePage, "Received a new manifest");
	n_rows = pk_manifest_get_n_rows(manifest);
	for (i = 1; i <= n_rows; i++) {
		name = pk_manifest_get_row_name(manifest, i);
		type = g_type_name(pk_manifest_get_row_type(manifest, i));
		gtk_list_store_append(priv->model, &iter);
		gtk_list_store_set(priv->model, &iter,
		                   0, name,
		                   1, type,
		                   -1);
	}
	EXIT;
}

static void
pkg_source_page_sample_cb (PkManifest *manifest,  /* IN */
                           PkSample   *sample,    /* IN */
                           gpointer    user_data) /* IN */
{
	PkgSourcePage *page = user_data;

	g_return_if_fail(PKG_IS_SOURCE_PAGE(page));

	ENTRY;
	DEBUG(SourcePage, "Received a new sample");
	EXIT;
}

static void
pkg_source_page_set_handlers_cb (GObject      *source,    /* IN */
                                 GAsyncResult *result,    /* IN */
                                 gpointer      user_data) /* IN */
{
	PkConnection *connection = PK_CONNECTION(source);
	PkgSourcePagePrivate *priv;
	PkgSourcePage *page = user_data;

	g_return_if_fail(PKG_IS_SOURCE_PAGE(page));
	g_return_if_fail(PK_IS_CONNECTION(source));

	ENTRY;
	priv = page->priv;
	DEBUG(SourcesPage, "Set handlers complete: %d", priv->id);
	pk_connection_subscription_add_source(connection, priv->sub_id, priv->id, NULL);
	pk_connection_subscription_unmute(connection, priv->sub_id, NULL);
	EXIT;
}

static void
pkg_source_page_subscription_added (GObject      *source,    /* IN */
                                    GAsyncResult *result,    /* IN */
                                    gpointer      user_data) /* IN */
{
	PkgSourcePagePrivate *priv;

	g_return_if_fail(PKG_IS_SOURCE_PAGE(user_data));

	ENTRY;
	priv = PKG_SOURCE_PAGE(user_data)->priv;
	if (pk_connection_manager_add_subscription_finish(PK_CONNECTION(source),
	                                                  result, &priv->sub_id,
	                                                  NULL)) {
		g_debug("Subscribed with sub_id: %d", priv->sub_id);
		pk_connection_subscription_set_handlers_async(
				priv->connection, priv->sub_id,
				pkg_source_page_manifest_cb,
				user_data, NULL,
				pkg_source_page_sample_cb,
				user_data, NULL,
				NULL,
				pkg_source_page_set_handlers_cb,
				user_data);
	}
	g_object_unref(user_data);
	EXIT;
}

void
pkg_source_page_load (PkgPage *page) /* IN */
{
	PkgSourcePagePrivate *priv;
	gchar *markup;

	g_return_if_fail(PKG_IS_SOURCE_PAGE(page));

	ENTRY;
	priv = PKG_SOURCE_PAGE(page)->priv;
	markup = g_markup_printf_escaped(
			_("<span weight=\"bold\">Source %d</span>"),
			priv->id);
	gtk_label_set_markup(GTK_LABEL(priv->title), markup);
	g_free(markup);
	if (priv->sub_id == -1) {
		pk_connection_manager_add_subscription_async(
				priv->connection, 0, 0, NULL,
				pkg_source_page_subscription_added,
				g_object_ref(page));
	}
	EXIT;
}

void
pkg_source_page_unload (PkgPage *page) /* IN */
{
	PkgSourcePagePrivate *priv;
	gboolean removed;

	ENTRY;
	priv = PKG_SOURCE_PAGE(page)->priv;
	if (priv->sub_id != -1) {
		INFO(Subscription, "Removing subscription %d", priv->sub_id);
		pk_connection_manager_remove_subscription(priv->connection,
		                                          priv->sub_id, &removed,
		                                          NULL);
	}
	EXIT;
}

static void
pkg_source_page_finalize (GObject *object)
{
	ENTRY;
	G_OBJECT_CLASS(pkg_source_page_parent_class)->finalize(object);
	EXIT;
}

static void
pkg_source_page_dispose (GObject *object)
{
	PkgSourcePagePrivate *priv;

	ENTRY;
	priv = PKG_SOURCE_PAGE(object)->priv;
	g_object_unref(priv->connection);
	G_OBJECT_CLASS(pkg_source_page_parent_class)->dispose(object);
	EXIT;
}

static void
pkg_source_page_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
	PkgSourcePagePrivate *priv;

	ENTRY;
	priv = PKG_SOURCE_PAGE(object)->priv;
	switch (prop_id) {
	CASE(PROP_CONNECTION);
		priv->connection = g_object_ref(g_value_get_object(value));
		BREAK;
	CASE(PROP_ID);
		priv->id = g_value_get_int(value);
		BREAK;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
	EXIT;
}

static void
pkg_source_page_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
	PkgSourcePagePrivate *priv;

	ENTRY;
	priv = PKG_SOURCE_PAGE(object)->priv;
	switch (prop_id) {
	CASE(PROP_CONNECTION);
		g_value_set_object(value, priv->connection);
		BREAK;
	CASE(PROP_ID);
		g_value_set_int(value, priv->id);
		BREAK;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
	EXIT;
}

static void
pkg_source_page_class_init (PkgSourcePageClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = pkg_source_page_finalize;
	object_class->dispose = pkg_source_page_dispose;
	object_class->set_property = pkg_source_page_set_property;
	object_class->get_property = pkg_source_page_get_property;
	g_type_class_add_private(object_class, sizeof(PkgSourcePagePrivate));

	g_object_class_install_property(object_class,
	                                PROP_CONNECTION,
	                                g_param_spec_object("connection",
	                                                    "Connection",
	                                                    "Connection",
	                                                    PK_TYPE_CONNECTION,
	                                                    G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property(object_class,
	                                PROP_ID,
	                                g_param_spec_int("id",
	                                                 "Id",
	                                                 "Id",
	                                                  0,
	                                                  G_MAXINT,
	                                                  0,
	                                                  G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
pkg_source_page_init (PkgSourcePage *page)
{
	PkgSourcePagePrivate *priv;
	GtkTreeViewColumn *column;
	GtkCellRenderer *cell;
	gchar *path;

	ENTRY;
	page->priv = G_TYPE_INSTANCE_GET_PRIVATE(page,
	                                         PKG_TYPE_SOURCE_PAGE,
	                                         PkgSourcePagePrivate);
	priv = page->priv;
	priv->sub_id = -1;

	#define EXTRACT_WIDGET(_n, _f) \
		priv->_f = GTK_WIDGET(gtk_builder_get_object(priv->builder, _n))

	path = pkg_path_for_data("ui", "pkg-source-page.ui", NULL);
	priv->builder = gtk_builder_new();
	gtk_builder_add_from_file(priv->builder, path, NULL);
	g_free(path);

	EXTRACT_WIDGET("source-page", container);
	EXTRACT_WIDGET("title", title);
	EXTRACT_WIDGET("treeview", treeview);

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Name"));
	cell = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, cell, TRUE);
	gtk_tree_view_column_add_attribute(column, cell, "text", 0);
	gtk_tree_view_append_column(GTK_TREE_VIEW(priv->treeview), column);

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Type"));
	cell = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, cell, TRUE);
	gtk_tree_view_column_add_attribute(column, cell, "text", 1);
	gtk_tree_view_append_column(GTK_TREE_VIEW(priv->treeview), column);

	priv->model = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(priv->treeview),
	                        GTK_TREE_MODEL(priv->model));

	gtk_widget_unparent(priv->container);
	gtk_container_add(GTK_CONTAINER(page), priv->container);

	EXIT;
}

static void
pkg_page_init (PkgPageIface *iface) /* IN */
{
	ENTRY;
	iface->load = pkg_source_page_load;
	iface->unload = pkg_source_page_unload;
	EXIT;
}

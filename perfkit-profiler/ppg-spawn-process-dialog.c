/* ppg-spawn-process-dialog.c
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
#include <gdk/gdkkeysyms.h>

#include "ppg-session.h"
#include "ppg-spawn-process-dialog.h"


/*
 * The following value is stored as a gboolean in the gpointer.
 * A value of 0 means we should copy the environment and a value
 * of 1 means we should not (disabled). This makes the default
 * to copy, which is what we want.
 */
#define COPY_QUARK (g_quark_from_static_string("copy-env-quark"))


struct _PpgSpawnProcessDialogPrivate
{
	PpgSession   *session;
	GtkWidget    *target_entry;
	GtkWidget    *args_entry;
	GtkWidget    *dir_button;
	GtkWidget    *env_treeview;
	GtkWidget    *ok_button;
	GtkWidget    *copy_env_button;
	GtkListStore *env_model;
};


enum
{
	PROP_0,
	PROP_SESSION,
};


G_DEFINE_TYPE(PpgSpawnProcessDialog, ppg_spawn_process_dialog, GTK_TYPE_DIALOG)


static gchar **
ppg_spawn_process_dialog_build_env (PpgSpawnProcessDialog *dialog)
{
	PpgSpawnProcessDialogPrivate *priv;
	GHashTable *env;
	GHashTableIter env_iter;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GPtrArray *ar;
	gboolean is_dummy;
	gchar **ret;
	gchar *key;
	gchar *value;
	gchar *item;
	gchar **names;
	gint i;

	g_return_val_if_fail(PPG_IS_SPAWN_PROCESS_DIALOG(dialog), NULL);

	priv = dialog->priv;

	model = GTK_TREE_MODEL(priv->env_model);
	env = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

	/*
	 * Copy the current environment if needed.
	 */
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->copy_env_button))) {
		names = g_listenv();
		for (i = 0; names[i]; i++) {
			g_hash_table_insert(env,
			                    g_strdup(names[i]),
			                    g_strdup(g_getenv(names[i])));
		}
		g_strfreev(names);
	}

	/*
	 * Add overrides in the treeview.
	 */
	if (gtk_tree_model_get_iter_first(model, &iter)) {
		do {
			gtk_tree_model_get(model, &iter, 2, &is_dummy, -1);
			if (is_dummy) {
				break;
			}
			gtk_tree_model_get(model, &iter,
			                   0, &key,
			                   1, &value,
			                   -1);
			g_hash_table_insert(env, key, value);
		} while (gtk_tree_model_iter_next(model, &iter));
	}

	/*
	 * Convert to a GStrv of KEY=VALUE strings.
	 */
	ar = g_ptr_array_new();
	g_hash_table_iter_init(&env_iter, env);
	while (g_hash_table_iter_next(&env_iter, (gpointer *)&key,
	                              (gpointer *)&value)) {
		item = g_strdup_printf("%s=%s", key, value);
		g_ptr_array_add(ar, item);
	}
	g_ptr_array_add(ar, NULL);

	/*
	 * Free the array after stealing its data.
	 */
	ret = (gchar **)ar->pdata;
	g_ptr_array_free(ar, FALSE);

	g_hash_table_unref(env);

	return ret;
}

static void
ppg_spawn_process_dialog_try_append_dummy (PpgSpawnProcessDialog *dialog)
{
	PpgSpawnProcessDialogPrivate *priv;
	GtkTreeIter iter;
	GtkTreeModel *model;
	gboolean is_dummy;

	g_return_if_fail(PPG_IS_SPAWN_PROCESS_DIALOG(dialog));

	priv = dialog->priv;
	model = GTK_TREE_MODEL(priv->env_model);

	if (gtk_tree_model_get_iter_first(model, &iter)) {
		do {
			gtk_tree_model_get(model, &iter, 2, &is_dummy, -1);
			if (is_dummy) {
				return;
			}
		} while (gtk_tree_model_iter_next(model, &iter));
	}

	gtk_list_store_append(priv->env_model, &iter);
	gtk_list_store_set(priv->env_model, &iter,
	                   0, _("Click to add variable"),
	                   2, TRUE,
                       -1);
}

static void
ppg_spawn_process_dialog_key_edited (GtkCellRendererText *cell,
                                     gchar *path,
                                     gchar *new_text,
                                     PpgSpawnProcessDialog *dialog)
{
	PpgSpawnProcessDialogPrivate *priv;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *tree_path;
	gchar *old_text;
	GtkTreeViewColumn *column;

	g_return_if_fail(PPG_IS_SPAWN_PROCESS_DIALOG(dialog));

	priv = dialog->priv;
	model = GTK_TREE_MODEL(priv->env_model);
	tree_path = gtk_tree_path_new_from_string(path);

	if (gtk_tree_model_get_iter(model, &iter, tree_path)) {
		gtk_tree_model_get(model, &iter,
		                   0, &old_text,
		                   -1);

		if (!g_strcmp0(old_text, new_text) || (!old_text && !new_text[0])) {
			goto cleanup;
		}

		g_strdelimit(new_text, " -", '_');

		gtk_list_store_set(priv->env_model, &iter,
		                   0, new_text,
		                   2, FALSE,
		                   -1);

		/* move to next column */
		column = gtk_tree_view_get_column(GTK_TREE_VIEW(priv->env_treeview), 1);
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(priv->env_treeview), tree_path,
		                         column, TRUE);

	  cleanup:
		g_free(old_text);
	}

	gtk_tree_path_free(tree_path);
	ppg_spawn_process_dialog_try_append_dummy(dialog);
}

static void
ppg_spawn_process_dialog_value_edited (GtkCellRendererText *cell,
                                       gchar *path,
                                       gchar *new_text,
                                       PpgSpawnProcessDialog *dialog)
{
	PpgSpawnProcessDialogPrivate *priv;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *tree_path;
	gchar *old_text;

	g_return_if_fail(PPG_IS_SPAWN_PROCESS_DIALOG(dialog));

	priv = dialog->priv;
	model = GTK_TREE_MODEL(priv->env_model);
	tree_path = gtk_tree_path_new_from_string(path);

	if (gtk_tree_model_get_iter(model, &iter, tree_path)) {
		gtk_tree_model_get(model, &iter,
		                   1, &old_text,
		                   -1);

		if (!g_strcmp0(old_text, new_text) || (!old_text && !new_text[0])) {
			g_free(old_text);
			return;
		}
		g_free(old_text);

		gtk_list_store_set(priv->env_model, &iter,
		                   1, new_text,
		                   2, FALSE,
		                   -1);
	}

	gtk_tree_path_free(tree_path);

	ppg_spawn_process_dialog_try_append_dummy(dialog);
}

static void
ppg_spawn_process_dialog_args_changed (GtkWidget             *entry,
                                       PpgSpawnProcessDialog *dialog)
{
	PpgSpawnProcessDialogPrivate *priv;
	const gchar *text;
	gint argc = 0;
	gchar **argv = NULL;
	GError *error = NULL;
	gboolean empty;

	g_return_if_fail(PPG_IS_SPAWN_PROCESS_DIALOG(dialog));

	priv = dialog->priv;
	text = gtk_entry_get_text(GTK_ENTRY(entry));
	empty = !text || !text[0];

	if (!empty && !g_shell_parse_argv(text, &argc, &argv, &error)) {
		gtk_widget_set_tooltip_text(entry, error->message);
		gtk_entry_set_icon_from_icon_name(GTK_ENTRY(entry),
		                                  GTK_ENTRY_ICON_SECONDARY,
		                                  GTK_STOCK_DIALOG_ERROR);
		gtk_widget_set_sensitive(priv->ok_button, FALSE);
		g_error_free(error);
	} else {
		gtk_widget_set_tooltip_text(entry, NULL);
		gtk_entry_set_icon_from_icon_name(GTK_ENTRY(entry),
		                                  GTK_ENTRY_ICON_SECONDARY,
		                                  NULL);
		gtk_widget_set_sensitive(priv->ok_button, TRUE);
	}

	g_strfreev(argv);
}

static void
ppg_spawn_process_dialog_add_env (PpgSpawnProcessDialog *dialog,
                                  const gchar *key,
                                  const gchar *value)
{
	PpgSpawnProcessDialogPrivate *priv;
	GtkTreeIter iter;

	g_return_if_fail(PPG_IS_SPAWN_PROCESS_DIALOG(dialog));
	g_return_if_fail(key != NULL);
	g_return_if_fail(value != NULL);

	priv = dialog->priv;

	gtk_list_store_prepend(priv->env_model, &iter);
	gtk_list_store_set(priv->env_model, &iter,
	                   0, key,
	                   1, value,
	                   2, FALSE,
	                   -1);
}

static void
ppg_spawn_process_dialog_load_env (PpgSpawnProcessDialog *dialog,
                                   gchar **env)
{
	PpgSpawnProcessDialogPrivate *priv;
	gboolean do_copy;
	gchar **kv;
	gint len;
	gint i;

	g_return_if_fail(PPG_IS_SPAWN_PROCESS_DIALOG(dialog));

	priv = dialog->priv;

	do_copy = !GPOINTER_TO_INT(g_object_get_qdata(G_OBJECT(priv->session),
	                           COPY_QUARK));

	for (i = 0; env[i]; i++) {
		kv = g_strsplit(env[i], "=", 2);
		len = g_strv_length(kv);

		if (!len) {
			g_strfreev(kv);
			continue;
		}

		/*
		 * We can ignore this item if the item if we are copying from
		 * the current env and it matches.
		 */
		if (do_copy) {
			if (g_getenv(kv[0])) {
				/*
				 * The strcmp is safe because kv[1] is at least NULL.
				 */
				if (!g_strcmp0(kv[1], g_getenv(kv[0]))) {
					g_strfreev(kv);
					continue;
				}
			}
		}

		switch (len) {
		case 1:
			ppg_spawn_process_dialog_add_env(dialog, kv[0], "");
			break;
		case 2:
			ppg_spawn_process_dialog_add_env(dialog, kv[0], kv[1]);
			break;
		default:
			g_assert_not_reached();
		}
		g_strfreev(kv);
	}
}

static void
ppg_spawn_process_dialog_set_session (PpgSpawnProcessDialog *dialog,
                                      PpgSession            *session)
{
	PpgSpawnProcessDialogPrivate *priv;
	gchar **args;
	gchar *args_str;
	gchar **env;
	gchar *target;
	gchar *working_dir;
	gboolean active;

	g_return_if_fail(PPG_IS_SPAWN_PROCESS_DIALOG(dialog));
	g_return_if_fail(PPG_IS_SESSION(session));

	priv = dialog->priv;
	priv->session = session;

	g_object_get(priv->session,
	             "args", &args,
	             "env", &env,
	             "target", &target,
	             "working-dir", &working_dir,
	             NULL);

	if (target) {
		gtk_entry_set_text(GTK_ENTRY(priv->target_entry), target);
		g_free(target);
	}

	if (args) {
		args_str = g_strjoinv(" ", args);
		gtk_entry_set_text(GTK_ENTRY(priv->args_entry), args_str);
		g_free(args_str);
		g_strfreev(args);
	}

	if (env) {
		ppg_spawn_process_dialog_load_env(dialog, env);
		g_strfreev(env);
	}

	if (working_dir) {
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(priv->dir_button),
		                              working_dir);
		g_free(working_dir);
	}

	active = !GPOINTER_TO_INT(g_object_get_qdata(G_OBJECT(priv->session),
	                          COPY_QUARK));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->copy_env_button),
	                             active);
}

static void
ppg_spawn_process_dialog_response (PpgSpawnProcessDialog *dialog,
                                   gint                   response_id)
{
	PpgSpawnProcessDialogPrivate *priv;
	const gchar *args;
	const gchar *target;
	const gchar *working_dir;
	gchar **argv = NULL;
	gchar **env = NULL;
	gint argc = 0;
	gboolean empty;
	gboolean do_copy;

	g_return_if_fail(PPG_IS_SPAWN_PROCESS_DIALOG(dialog));

	priv = dialog->priv;

	if (response_id != GTK_RESPONSE_OK) {
		return;
	}

	target = gtk_entry_get_text(GTK_ENTRY(priv->target_entry));
	args = gtk_entry_get_text(GTK_ENTRY(priv->args_entry));
	empty = !args || !args[0];
	working_dir = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(priv->dir_button));

	if (!empty && !g_shell_parse_argv(args, &argc, &argv, NULL)) {
		g_signal_stop_emission_by_name(dialog, "response");
		return;
	}

	env = ppg_spawn_process_dialog_build_env(dialog);

	g_object_set(priv->session,
	             "args", argv,
	             "env", env,
	             "target", target,
	             "working-dir", working_dir,
	             NULL);

	do_copy = !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->copy_env_button));
	g_object_set_qdata(G_OBJECT(priv->session), COPY_QUARK,
	                   GINT_TO_POINTER(do_copy));

	g_strfreev(env);
	g_strfreev(argv);
}

static gboolean
tree_view_key_press (GtkTreeView           *tree_view,
                     GdkEventKey           *key,
                     PpgSpawnProcessDialog *dialog)
{
	PpgSpawnProcessDialogPrivate *priv;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean is_dummy;

	g_return_val_if_fail(PPG_IS_SPAWN_PROCESS_DIALOG(dialog), FALSE);

	priv = dialog->priv;

	switch (key->keyval) {
	case GDK_KEY_Delete:
		selection = gtk_tree_view_get_selection(tree_view);
		if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
			gtk_tree_model_get(model, &iter, 2, &is_dummy, -1);
			if (!is_dummy) {
				gtk_list_store_remove(priv->env_model, &iter);
			}
		}
		break;
	default:
		break;
	}

	return FALSE;
}

static void
browse_clicked (GtkWidget             *widget,
                PpgSpawnProcessDialog *dialog)
{
	PpgSpawnProcessDialogPrivate *priv;
	GtkWidget *chooser;
	const gchar *filename;
	gchar *dir;
	gint ret;

	g_return_if_fail(PPG_IS_SPAWN_PROCESS_DIALOG(dialog));

	priv = dialog->priv;

	chooser = gtk_file_chooser_dialog_new(_("Open Target"), GTK_WINDOW(dialog),
	                                      GTK_FILE_CHOOSER_ACTION_OPEN,
	                                      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	                                      GTK_STOCK_OPEN, GTK_RESPONSE_OK,
	                                      NULL);

	ret = gtk_dialog_run(GTK_DIALOG(chooser));
	if (ret == GTK_RESPONSE_OK) {
		/*
		 * Set the entry to the filename path.
		 */
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
		gtk_entry_set_text(GTK_ENTRY(priv->target_entry), filename);

		/*
		 * Set the working-dir to the directory containing the file.
		 */
		dir = g_path_get_dirname(filename);
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(priv->dir_button), dir);
		g_free(dir);
	}

	gtk_widget_destroy(chooser);
}

/**
 * ppg_spawn_process_dialog_finalize:
 * @object: (in): A #PpgSpawnProcessDialog.
 *
 * Finalizer for a #PpgSpawnProcessDialog instance.  Frees any resources held by
 * the instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_spawn_process_dialog_finalize (GObject *object)
{
	G_OBJECT_CLASS(ppg_spawn_process_dialog_parent_class)->finalize(object);
}

/**
 * ppg_spawn_process_dialog_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (out): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Get a given #GObject property.
 */
static void
ppg_spawn_process_dialog_get_property (GObject    *object,
                                       guint       prop_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
	PpgSpawnProcessDialog *dialog = PPG_SPAWN_PROCESS_DIALOG(object);

	switch (prop_id) {
	case PROP_SESSION:
		g_value_set_object(value, dialog->priv->session);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * ppg_spawn_process_dialog_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
static void
ppg_spawn_process_dialog_set_property (GObject      *object,
                                       guint         prop_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
	PpgSpawnProcessDialog *dialog = PPG_SPAWN_PROCESS_DIALOG(object);

	switch (prop_id) {
	case PROP_SESSION:
		ppg_spawn_process_dialog_set_session(dialog,
		                                     g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

/**
 * ppg_spawn_process_dialog_class_init:
 * @klass: (in): A #PpgSpawnProcessDialogClass.
 *
 * Initializes the #PpgSpawnProcessDialogClass and prepares the vtable.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_spawn_process_dialog_class_init (PpgSpawnProcessDialogClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_spawn_process_dialog_finalize;
	object_class->get_property = ppg_spawn_process_dialog_get_property;
	object_class->set_property = ppg_spawn_process_dialog_set_property;
	g_type_class_add_private(object_class, sizeof(PpgSpawnProcessDialogPrivate));

	g_object_class_install_property(object_class,
	                                PROP_SESSION,
	                                g_param_spec_object("session",
	                                                    "session",
	                                                    "session",
	                                                    PPG_TYPE_SESSION,
	                                                    G_PARAM_READWRITE));
}

/**
 * ppg_spawn_process_dialog_init:
 * @dialog: (in): A #PpgSpawnProcessDialog.
 *
 * Initializes the newly created #PpgSpawnProcessDialog instance.
 *
 * Returns: None.
 * Side effects: None.
 */
static void
ppg_spawn_process_dialog_init (PpgSpawnProcessDialog *dialog)
{
	PpgSpawnProcessDialogPrivate *priv;
	GtkTreeViewColumn *column;
	GtkCellRenderer *cell;
	GtkWidget *content_area;
	GtkWidget *vbox;
	GtkWidget *vbox2;
	GtkWidget *b;
	GtkWidget *l;
	GtkWidget *table;
	GtkWidget *align;
	GtkWidget *scroller;
	GtkWidget *hbox;

	priv = G_TYPE_INSTANCE_GET_PRIVATE(dialog,
	                                   PPG_TYPE_SPAWN_PROCESS_DIALOG,
	                                   PpgSpawnProcessDialogPrivate);
	dialog->priv = priv;

	g_object_set(dialog,
	             "border-width", 6,
	             "default-width", 350,
	             "default-height", 400,
#if !GTK_CHECK_VERSION(2, 91, 0)
	             "has-separator", FALSE,
#endif
	             "title", _("Spawn a new process"),
	             NULL);

	content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	vbox = g_object_new(GTK_TYPE_VBOX,
	                    "border-width", 6,
	                    "spacing", 12,
	                    "visible", TRUE,
	                    NULL);
	gtk_container_add(GTK_CONTAINER(content_area), vbox);

	l = g_object_new(GTK_TYPE_LABEL,
	                 "label", _("Choose a target executable to spawn and "
	                 	        "configure the environment."),
	                 "justify", GTK_JUSTIFY_CENTER,
	                 "visible", TRUE,
	                 "wrap", TRUE,
	                 NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(vbox), l,
	                                  "expand", FALSE,
	                                  NULL);

	table = g_object_new(GTK_TYPE_TABLE,
	                     "n-columns", 3,
	                     "n-rows", 6,
	                     "row-spacing", 3,
	                     "visible", TRUE,
	                     NULL);
	gtk_container_add(GTK_CONTAINER(vbox), table);

	l = g_object_new(GTK_TYPE_LABEL,
	                 "use-markup", TRUE,
	                 "label", _("<b>Process</b>"),
	                 "visible", TRUE,
	                 "xalign", 0.0f,
	                 "ypad", 3,
	                 NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(table), l,
	                                  "bottom-attach", 1,
	                                  "left-attach", 0,
	                                  "right-attach", 2,
	                                  "top-attach", 0,
	                                  "y-options", GTK_FILL,
	                                  NULL);

	hbox = g_object_new(GTK_TYPE_HBOX,
	                    "spacing", 3,
	                    "visible", TRUE,
	                    NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(table), hbox,
	                                  "bottom-attach", 2,
	                                  "left-attach", 1,
	                                  "right-attach", 2,
	                                  "top-attach", 1,
	                                  "y-options", GTK_FILL,
	                                  NULL);

	priv->target_entry = g_object_new(GTK_TYPE_ENTRY,
	                                  "activates-default", TRUE,
	                                  "visible", TRUE,
	                                  NULL);
	gtk_container_add(GTK_CONTAINER(hbox), priv->target_entry);

	/*
	 * FIXME: This will probably need to be hidden on remote profiling.
	 */
	b = g_object_new(GTK_TYPE_BUTTON,
	                 "label", _("..."), /* Is "_Browse" better? */
	                 "use-underline", TRUE,
	                 "visible", TRUE,
	                 NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(hbox), b,
	                                  "expand", FALSE,
	                                  NULL);
	g_signal_connect(b, "clicked", G_CALLBACK(browse_clicked), dialog);

	l = g_object_new(GTK_TYPE_LABEL,
	                 "label", _("_Executable:"),
	                 "mnemonic-widget", priv->target_entry,
	                 "use-underline", TRUE,
	                 "visible", TRUE,
	                 "xalign", 1.0f,
	                 "xpad", 12,
	                 NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(table), l,
	                                  "bottom-attach", 2,
	                                  "left-attach", 0,
	                                  "right-attach", 1,
	                                  "top-attach", 1,
	                                  "y-options", GTK_FILL,
	                                  "x-options", GTK_FILL,
	                                  NULL);

	priv->args_entry = g_object_new(GTK_TYPE_ENTRY,
	                                "activates-default", TRUE,
	                                "visible", TRUE,
	                                NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(table), priv->args_entry,
	                                  "bottom-attach", 3,
	                                  "left-attach", 1,
	                                  "right-attach", 2,
	                                  "top-attach", 2,
	                                  "y-options", GTK_FILL,
	                                  NULL);
	g_signal_connect(priv->args_entry, "changed",
	                 G_CALLBACK(ppg_spawn_process_dialog_args_changed),
	                 dialog);

	l = g_object_new(GTK_TYPE_LABEL,
	                 "label", _("_Arguments:"),
	                 "mnemonic-widget", priv->args_entry,
	                 "use-underline", TRUE,
	                 "visible", TRUE,
	                 "xalign", 1.0f,
	                 "xpad", 12,
	                 NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(table), l,
	                                  "bottom-attach", 3,
	                                  "left-attach", 0,
	                                  "right-attach", 1,
	                                  "top-attach", 2,
	                                  "y-options", GTK_FILL,
	                                  "x-options", GTK_FILL,
	                                  NULL);

	/*
	 * FIXME: Remote profiling should use a GtkEntry here.
	 */
	priv->dir_button = g_object_new(GTK_TYPE_FILE_CHOOSER_BUTTON,
	                                "action", GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
	                                "visible", TRUE,
	                                NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(table), priv->dir_button,
	                                  "bottom-attach", 4,
	                                  "left-attach", 1,
	                                  "right-attach", 2,
	                                  "top-attach", 3,
	                                  "y-options", GTK_FILL,
	                                  NULL);

	l = g_object_new(GTK_TYPE_LABEL,
	                 "label", _("Working _Directory:"),
	                 "mnemonic-widget", priv->dir_button,
	                 "use-underline", TRUE,
	                 "visible", TRUE,
	                 "xalign", 1.0f,
	                 "xpad", 12,
	                 NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(table), l,
	                                  "bottom-attach", 4,
	                                  "left-attach", 0,
	                                  "right-attach", 1,
	                                  "top-attach", 3,
	                                  "y-options", GTK_FILL,
	                                  "x-options", GTK_FILL,
	                                  NULL);

	l = g_object_new(GTK_TYPE_LABEL,
	                 "use-markup", TRUE,
	                 "label", _("<b>Environment</b>"),
	                 "visible", TRUE,
	                 "xalign", 0.0f,
	                 "ypad", 3,
	                 NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(table), l,
	                                  "bottom-attach", 5,
	                                  "left-attach", 0,
	                                  "right-attach", 2,
	                                  "top-attach", 4,
	                                  "y-options", GTK_FILL,
	                                  NULL);

	align = g_object_new(GTK_TYPE_ALIGNMENT,
	                     "visible", TRUE,
	                     "left-padding", 12,
	                     NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(table), align,
	                                  "bottom-attach", 6,
	                                  "left-attach", 0,
	                                  "right-attach", 2,
	                                  "top-attach", 5,
	                                  NULL);

	vbox2 = g_object_new(GTK_TYPE_VBOX,
	                     "spacing", 6,
	                     "visible", TRUE,
	                     NULL);
	gtk_container_add(GTK_CONTAINER(align), vbox2);

	scroller = g_object_new(GTK_TYPE_SCROLLED_WINDOW,
	                        "hscrollbar-policy", GTK_POLICY_NEVER,
	                        "vscrollbar-policy", GTK_POLICY_AUTOMATIC,
	                        "shadow-type", GTK_SHADOW_IN,
	                        "visible", TRUE,
	                        NULL);
	gtk_container_add(GTK_CONTAINER(vbox2), scroller);

	priv->env_model = gtk_list_store_new(3,
	                                     G_TYPE_STRING,
	                                     G_TYPE_STRING,
	                                     G_TYPE_BOOLEAN);
	ppg_spawn_process_dialog_try_append_dummy(dialog);

	priv->env_treeview = g_object_new(GTK_TYPE_TREE_VIEW,
	                                  "model", priv->env_model,
	                                  "visible", TRUE,
	                                  NULL);
	gtk_container_add(GTK_CONTAINER(scroller), priv->env_treeview);
	g_signal_connect(priv->env_treeview, "key-press-event",
	                 G_CALLBACK(tree_view_key_press),
	                 dialog);

	column = g_object_new(GTK_TYPE_TREE_VIEW_COLUMN,
	                      "expand", TRUE,
	                      "title", _("Key"),
	                      NULL);
	cell = g_object_new(GTK_TYPE_CELL_RENDERER_TEXT,
	                    "editable", TRUE,
	                    "ellipsize", PANGO_ELLIPSIZE_MIDDLE,
	                    NULL);
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(column), cell, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(column), cell, "text", 0);
	gtk_tree_view_append_column(GTK_TREE_VIEW(priv->env_treeview), column);
	g_signal_connect(cell, "edited",
	                 G_CALLBACK(ppg_spawn_process_dialog_key_edited),
	                 dialog);

	column = g_object_new(GTK_TYPE_TREE_VIEW_COLUMN,
	                      "expand", TRUE,
	                      "title", _("Value"),
	                      NULL);
	cell = g_object_new(GTK_TYPE_CELL_RENDERER_TEXT,
	                    "editable", TRUE,
	                    "ellipsize", PANGO_ELLIPSIZE_MIDDLE,
	                    NULL);
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(column), cell, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(column), cell, "text", 1);
	gtk_tree_view_append_column(GTK_TREE_VIEW(priv->env_treeview), column);
	g_signal_connect(cell, "edited",
	                 G_CALLBACK(ppg_spawn_process_dialog_value_edited),
	                 dialog);

	priv->copy_env_button = g_object_new(GTK_TYPE_CHECK_BUTTON,
	                                     "active", TRUE,
	                                     "label", _("_Copy current environment"),
	                                     "use-underline", TRUE,
	                                     "visible", TRUE,
	                                     NULL);
	gtk_container_add_with_properties(GTK_CONTAINER(vbox2),
	                                  priv->copy_env_button,
	                                  "expand", FALSE,
	                                  NULL);

	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CANCEL,
	                      GTK_RESPONSE_CANCEL);
	priv->ok_button = gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_OK,
	                                        GTK_RESPONSE_OK);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

	g_signal_connect(dialog, "response",
	                 G_CALLBACK(ppg_spawn_process_dialog_response),
	                 NULL);
}

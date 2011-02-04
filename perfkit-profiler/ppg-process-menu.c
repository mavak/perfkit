/* ppg-process-menu.c
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

#include <stdlib.h>
#include <string.h>

#include "ppg-process-menu.h"
#include "ppg-session.h"

G_DEFINE_TYPE(PpgProcessMenu, ppg_process_menu, GTK_TYPE_MENU)

struct _PpgProcessMenuPrivate
{
	PpgSession *session;
};

enum
{
	PROP_0,
	PROP_SESSION,
};


static GQuark pid_quark;


static void
ppg_process_menu_set_session (PpgProcessMenu *menu,
                              PpgSession *session)
{
	g_return_if_fail(PPG_IS_PROCESS_MENU(menu));
	menu->priv->session = session;
}

static void
ppg_process_menu_item_activate (GtkMenuItem *item,
                                PpgProcessMenu *menu)
{
	PpgProcessMenuPrivate *priv;
	GPid pid;

	g_return_if_fail(PPG_IS_PROCESS_MENU(menu));

	priv = menu->priv;

	pid = GPOINTER_TO_INT(g_object_get_qdata(G_OBJECT(item), pid_quark));
	g_object_set(priv->session, "pid", pid, NULL);
}

static void
ppg_process_menu_add_item (PpgProcessMenu *menu,
                           GPid pid)
{
	GtkMenuItem *item;
	gchar *path = NULL;
	gchar *label = NULL;
	gchar *buffer = NULL;
	gchar cmd[32];

	g_return_if_fail(PPG_IS_PROCESS_MENU(menu));

	path = g_strdup_printf("/proc/%d/cmdline", (gint)pid);
	if (!g_file_test(path, G_FILE_TEST_IS_REGULAR)) {
		goto cleanup;
	}

	if (!g_file_get_contents(path, &buffer, NULL, NULL)) {
		goto cleanup;
	}

	if (!buffer || !buffer[0]) {
		/*
		 * Probably a kernel thread.
		 */
		goto cleanup;
	}

	strncpy(cmd, buffer, sizeof cmd - 1);
	cmd[sizeof cmd - 1] = '\0';

	label = g_strdup_printf("%d - %s", (gint)pid, cmd);
	item = g_object_new(GTK_TYPE_MENU_ITEM,
	                    "label", label,
	                    "visible", TRUE,
	                    "tooltip-text", buffer,
	                    NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(item));
	g_object_set_qdata(G_OBJECT(item), pid_quark, GINT_TO_POINTER(pid));
	g_signal_connect(item, "activate",
	                 G_CALLBACK(ppg_process_menu_item_activate),
	                 menu);

  cleanup:
	g_free(label);
	g_free(buffer);
	g_free(path);
}

static void
local_process_foreach (void (*callback) (PpgProcessMenu*, GPid),
                       PpgProcessMenu *menu)
{
	const gchar *name;
	char *endptr = NULL;
	GDir *dir;
	GPid pid;

	if (!(dir = g_dir_open("/proc", 0, NULL))) {
		return;
	}

	while ((name = g_dir_read_name(dir))) {
		if ((pid = strtol(name, &endptr, 10))) {
			if (endptr && !endptr[0]) {
				callback(menu, pid);
			}
		}
	}

	g_dir_close(dir);
}

static void
ppg_process_menu_show (GtkWidget *widget)
{
	GtkWidgetClass *klass;
	GList *list;
	GList *iter;

	g_return_if_fail(PPG_IS_PROCESS_MENU(widget));

	/*
	 * Remove all existing children.
	 */
	list = gtk_container_get_children(GTK_CONTAINER(widget));
	for (iter = list; iter; iter = iter->next) {
		gtk_container_remove(GTK_CONTAINER(widget), GTK_WIDGET(iter->data));
		gtk_widget_destroy(GTK_WIDGET(iter->data));
	}
	g_list_free(list);

	/*
	 * Add a menu item for each process.
	 */
	local_process_foreach(ppg_process_menu_add_item, PPG_PROCESS_MENU(widget));

	/*
	 * Show the widget.
	 */
	klass = GTK_WIDGET_CLASS(ppg_process_menu_parent_class);
	klass->show(widget);
}

static void
ppg_process_menu_finalize (GObject *object)
{
	G_OBJECT_CLASS(ppg_process_menu_parent_class)->finalize(object);
}

static void
ppg_process_menu_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
	PpgProcessMenu *menu = PPG_PROCESS_MENU(object);

	switch (prop_id) {
	case PROP_SESSION:
		g_value_set_object(value, menu->priv->session);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
ppg_process_menu_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
	PpgProcessMenu *menu = PPG_PROCESS_MENU(object);

	switch (prop_id) {
	case PROP_SESSION:
		ppg_process_menu_set_session(menu, g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
ppg_process_menu_class_init (PpgProcessMenuClass *klass)
{
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = ppg_process_menu_finalize;
	object_class->get_property = ppg_process_menu_get_property;
	object_class->set_property = ppg_process_menu_set_property;
	g_type_class_add_private(object_class, sizeof(PpgProcessMenuPrivate));

	widget_class = GTK_WIDGET_CLASS(klass);
	widget_class->show = ppg_process_menu_show;

	g_object_class_install_property(object_class,
	                                PROP_SESSION,
	                                g_param_spec_object("session",
	                                                    "session",
	                                                    "session",
	                                                    PPG_TYPE_SESSION,
	                                                    G_PARAM_READWRITE));

	pid_quark = g_quark_from_static_string("ppg-process-menu-pid");
}

static void
ppg_process_menu_init (PpgProcessMenu *menu)
{
	menu->priv = G_TYPE_INSTANCE_GET_PRIVATE(menu, PPG_TYPE_PROCESS_MENU,
	                                         PpgProcessMenuPrivate);
}

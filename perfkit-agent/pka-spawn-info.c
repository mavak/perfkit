/* pka-spawn-info.c
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

#include <string.h>

#include "pka-log.h"
#include "pka-spawn-info.h"

PkaSpawnInfo*
pka_spawn_info_copy (PkaSpawnInfo *spawn_info) /* IN */
{
	PkaSpawnInfo *copy;

	g_return_val_if_fail(spawn_info != NULL, NULL);

	ENTRY;
	copy = g_slice_new0(PkaSpawnInfo);
	copy->pid = spawn_info->pid;
	copy->target = g_strdup(spawn_info->target);
	copy->args = g_strdupv(spawn_info->args);
	copy->env = g_strdupv(spawn_info->env);
	copy->working_dir = g_strdup(spawn_info->working_dir);
	copy->standard_input = spawn_info->standard_input;
	copy->standard_output = spawn_info->standard_output;
	copy->standard_error = spawn_info->standard_error;
	RETURN(copy);
}

void
pka_spawn_info_free (PkaSpawnInfo *spawn_info) /* IN */
{
	g_return_if_fail(spawn_info != NULL);

	ENTRY;
	g_free(spawn_info->target);
	g_strfreev(spawn_info->args);
	g_strfreev(spawn_info->env);
	g_free(spawn_info->working_dir);
	g_slice_free(PkaSpawnInfo, spawn_info);
	EXIT;
}

void
pka_spawn_info_set_env (PkaSpawnInfo *spawn_info,
                        const gchar  *key,
                        const gchar  *value)
{
	gchar *env;
	guint32 len;
	gint i;

	g_return_if_fail(spawn_info);
	g_return_if_fail(spawn_info->env);
	g_return_if_fail(key);

	env = g_strdup_printf("%s=%s", key, value ? value : "");
	len = strlen(key) + 1;

	for (i = 0; spawn_info->env[i]; i++) {
		if (g_str_equal(spawn_info->env[i], env)) {
			g_free(spawn_info->env[i]);
			spawn_info->env[i] = env;
			return;
		}
	}

	len = g_strv_length(spawn_info->env);
	spawn_info->env = g_realloc_n(spawn_info->env, len + 2, sizeof(gchar*));
	spawn_info->env[len] = env;
	spawn_info->env[len + 1] = NULL;
	g_assert(g_strv_length(spawn_info->env) == len + 1);
}

GType
pka_spawn_info_get_type (void)
{
	static gsize initialized = FALSE;
	static GType type_id = G_TYPE_INVALID;

	if (g_once_init_enter(&initialized)) {
		type_id = g_boxed_type_register_static("PkaSpawnInfo",
		                                       (GBoxedCopyFunc)pka_spawn_info_copy,
		                                       (GBoxedFreeFunc)pka_spawn_info_free);
		g_once_init_leave(&initialized, TRUE);
	}
	return type_id;
}

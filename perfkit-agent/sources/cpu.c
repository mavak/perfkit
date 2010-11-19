/* cpu.c
 *
 * Copyright (C) 2010 Andrew Stiegmann <andrew.stiegmann@gmail.com>
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

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <perfkit-agent/perfkit-agent.h>
#include <stdio.h>
#include <string.h>

#include "src-utils.h"

typedef struct
{
	PkaManifest *manifest;
} CpuData;

/**
 * parse_cpu_stat_line:
 * @str: (in): The line to parse
 * @val: (inout): The pointer to the int array
 *
 * Parses a line from /proc/stat sinto an int array.
 *
 * Returns: None.
 * Side effects: @val is updated.
 **/
static gboolean
parse_cpu_stat_line (gchar *str,
                     gint  *val)
{
	if (!strncmp(str, "cpu ", 4)) {
		return FALSE;
	}

	return 10 == sscanf(str, "cpu%d %d %d %d %d %d %d %d %d %d",
	                    &val[0], &val[1], &val[2], &val[3], &val[4],
	                    &val[5], &val[6], &val[7], &val[8], &val[9]);
}


/**
 * cpu_sample:
 * @source: (in): A #PkaSourceSimple.
 * @user_data: (in): A #CpuData.
 *
 * Handles a callback to generate another sample.
 *
 * Return value: None.
 * Side effects: @user_data is updated with new values.
 */
static inline void
cpu_sample (PkaSourceSimple *source,
            gpointer         user_data)
{
	CpuData *cpud = user_data;
	gchar *curr;
	gchar fileBuf[1024];
	gint cpuData[10];

	/*
	 * Create and deliver our manifest if it has not yet been done.
	 */
	if (G_UNLIKELY(!cpud->manifest)) {
		cpud->manifest = pka_manifest_sized_new(17);
		pka_manifest_append(cpud->manifest, "CPU Number", G_TYPE_INT);
		pka_manifest_append(cpud->manifest, "User", G_TYPE_INT);
		pka_manifest_append(cpud->manifest, "Nice", G_TYPE_INT);
		pka_manifest_append(cpud->manifest, "System", G_TYPE_INT);
		pka_manifest_append(cpud->manifest, "Idle", G_TYPE_INT);
		pka_manifest_append(cpud->manifest, "I/O Wait", G_TYPE_INT);
		pka_manifest_append(cpud->manifest, "IRQ", G_TYPE_INT);
		pka_manifest_append(cpud->manifest, "Soft IRQ", G_TYPE_INT);
		pka_manifest_append(cpud->manifest, "VM Stolen", G_TYPE_INT);
		pka_manifest_append(cpud->manifest, "VM Guest", G_TYPE_INT);
		pka_source_deliver_manifest(PKA_SOURCE(source), cpud->manifest);
	}

	/*
	 * Read in /proc/stat first.
	 */
	src_utils_read_file("/proc/stat", fileBuf, 1024);
	curr = fileBuf;

	while (curr != NULL) {
		gchar *next = src_utils_str_tok('\n', curr);
		PkaSample *s;
		gint i;

		if (parse_cpu_stat_line(curr, cpuData)) {
			s = pka_sample_new();
			for (i = 0; i < 10; i++)
				pka_sample_append_int(s, i+1, cpuData[i]);
				pka_source_deliver_sample(PKA_SOURCE(source), s);
				pka_sample_unref(s);
			}
			curr = next;
		}
	}

/**
 * cpu_free:
 * @data: (in): A #CpuData.
 *
 * Frees resources associated with @data and @data.
 *
 * Returns: None.
 * Side effects: Everything.
 */
static void
cpu_free (gpointer data)
{
	CpuData *cpud = data;

	g_return_if_fail(cpud);

	ENTRY;
	if (cpud->manifest) {
		pka_manifest_unref(cpud->manifest);
	}
	g_slice_free(CpuData, data);
	EXIT;
}

/**
 * cpu_new:
 * @error: (error): A location for a #GError, or %NULL.
 *
 * Creates a new instance of the cpu source.
 *
 * Returns: The newly created source instance.
 * Side effects: None.
 */
GObject*
cpu_new (GError **error)
{
	PkaSource *source;
	CpuData *cpu;

	ENTRY;
	cpu = g_slice_new0(CpuData);
	source = pka_source_simple_new_full(cpu_sample, /* Sample */
	                                    NULL,       /* Spawn */
	                                    cpu,        /* Data */
	                                    cpu_free);  /* Free */
	RETURN(G_OBJECT(source));
}

const PkaPluginInfo pka_plugin_info = {
	.id          = N_("Cpu"),
	.name        = N_("CPU usage"),
	.description = N_("This source provides general CPU usage information "
	                  "about the running machine"),
	.version     = N_("0.1.1"),
	.copyright   = N_("Andrew Stiegmann"),
	.factory     = cpu_new,
	.plugin_type = PKA_PLUGIN_SOURCE,
};

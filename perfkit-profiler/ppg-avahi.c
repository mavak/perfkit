/* ppg-avahi.c
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

#include <avahi-gobject/ga-entry-group.h>
#include <glib-object.h>

#include "ppg-log.h"

static GaClient     *client      = NULL;
static GaEntryGroup *entry_group = NULL;

void
ppg_avahi_init (void)
{
	GError *error = NULL;

	ENTRY;

	g_return_if_fail(!client);
	g_return_if_fail(!entry_group);

	client = g_object_new(GA_TYPE_CLIENT, NULL);
	if (!ga_client_start(client, &error)) {
		WARNING(Avahi, "%s", error->message);
		g_clear_error(&error);
		goto error;
	}

	entry_group = g_object_new(GA_TYPE_ENTRY_GROUP, NULL);
	if (!ga_entry_group_attach(entry_group, client, &error)) {
		WARNING(Avahi, "%s", error->message);
		g_clear_error(&error);
		goto error;
	}

	if (!ga_entry_group_add_service(entry_group, "Perfkit Remote Control",
	                                "_perfkit_remote._tcp", 9919, &error,
	                                NULL)) {
		WARNING(Avahi, "%s", error->message);
		g_clear_error(&error);
		goto error;
	}

	if (!ga_entry_group_commit(entry_group, &error)) {
		WARNING(Avahi, "%s", error->message);
		g_clear_error(&error);
		goto error;
	}

	DEBUG(Avahi, "Avahi subsystem initialized.");
	EXIT;

  error:
	if (entry_group) {
		g_object_unref(entry_group);
		entry_group = NULL;
	}
	if (client) {
		g_object_unref(client);
		client = NULL;
	}
	EXIT;
}

void
ppg_avahi_shutdown (void)
{
}

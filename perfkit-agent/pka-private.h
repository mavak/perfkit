/* pka-private.h
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

#ifndef __PKA_PRIVATE_H__
#define __PKA_PRIVATE_H__

#include "pka-channel.h"
#include "pka-manifest.h"
#include "pka-plugin.h"
#include "pka-sample.h"
#include "pka-source.h"
#include "pka-spawn-info.h"
#include "pka-subscription.h"

G_BEGIN_DECLS

void     pka_config_init                   (const gchar     *filename);
void     pka_config_shutdown               (void);
void     pka_log_init                      (gboolean         stdout_,
                                            const gchar     *filename);
void     pka_log_shutdown                  (void);
void     pka_manager_init                  (void);
void     pka_manager_quit                  (void);
void     pka_manager_run                   (void);
void     pka_manager_shutdown              (void);
void     pka_manifest_set_source_id        (PkaManifest     *manifest,
                                            gint             source_id);
void     pka_sample_set_source_id          (PkaSample       *sample,
                                            gint             source_id);
void     pka_source_add_subscription       (PkaSource       *source,
                                            PkaSubscription *subscription);
void     pka_source_notify_muted           (PkaSource       *source);
void     pka_source_notify_reset           (PkaSource       *source);
void     pka_source_notify_started         (PkaSource       *source,
                                            PkaSpawnInfo    *spawn_info);
void     pka_source_notify_stopped         (PkaSource       *source);
void     pka_source_notify_unmuted         (PkaSource       *source);
void     pka_source_remove_subscription    (PkaSource       *source,
                                            PkaSubscription *subscription);
gboolean pka_source_set_channel            (PkaSource       *source,
                                            PkaChannel      *channel);
void     pka_source_set_plugin             (PkaSource       *source,
                                            PkaPlugin       *plugin);

G_END_DECLS

#endif /* __PKA_PRIVATE_H__ */

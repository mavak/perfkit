/* pka-channel.h
 *
 * Copyright (C) 2009 Christian Hergert
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

#if !defined (__PERFKIT_AGENT_INSIDE__) && !defined (PERFKIT_COMPILATION)
#error "Only <perfkit-agent/perfkit-agent.h> can be included directly."
#endif

#ifndef __PKA_CHANNEL_H__
#define __PKA_CHANNEL_H__

#include <glib-object.h>

#include "pka-manifest.h"
#include "pka-sample.h"
#include "pka-source-info.h"
#include "pka-spawn-info.h"

G_BEGIN_DECLS

#define PKA_TYPE_CHANNEL            (pka_channel_get_type())
#define PKA_CHANNEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKA_TYPE_CHANNEL, PkaChannel))
#define PKA_CHANNEL_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PKA_TYPE_CHANNEL, PkaChannel const))
#define PKA_CHANNEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PKA_TYPE_CHANNEL, PkaChannelClass))
#define PKA_IS_CHANNEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PKA_TYPE_CHANNEL))
#define PKA_IS_CHANNEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PKA_TYPE_CHANNEL))
#define PKA_CHANNEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PKA_TYPE_CHANNEL, PkaChannelClass))
#define PKA_CHANNEL_ERROR           (pka_channel_error_quark())

/**
 * PkaChannelError:
 * @PKA_CHANNEL_ERROR_STATE: The operation is invalid for the current state
 *   of the channel.
 *
 * #PkaChannel error enumeration.
 */
typedef enum
{
	PKA_CHANNEL_ERROR_UNKNOWN,
	PKA_CHANNEL_ERROR_STATE,
} PkaChannelError;

/**
 * PkaChannelState:
 * @PKA_CHANNEL_READY: 
 * @PKA_CHANNEL_RUNNING:
 * @PKA_CHANNEL_PAUSED:
 * @PKA_CHANNEL_STOPPED:
 * @PKA_CHANNEL_FAILED: 
 *
 * The #PkaChannel state enumeration.
 */
typedef enum
{
	PKA_CHANNEL_READY    = 1,
	PKA_CHANNEL_RUNNING  = 2,
	PKA_CHANNEL_PAUSED   = 3,
	PKA_CHANNEL_STOPPED  = 4,
	PKA_CHANNEL_FAILED   = 5,
} PkaChannelState;

typedef struct _PkaChannel        PkaChannel;
typedef struct _PkaChannelClass   PkaChannelClass;
typedef struct _PkaChannelPrivate PkaChannelPrivate;

struct _PkaChannel
{
	GObject parent;

	/*< private >*/
	PkaChannelPrivate *priv;
};

struct _PkaChannelClass
{
	GObjectClass parent_class;
};

GType           pka_channel_get_type        (void) G_GNUC_CONST;
GQuark          pka_channel_error_quark     (void) G_GNUC_CONST;
PkaChannel*     pka_channel_new             (const PkaSpawnInfo  *spawn_info);
guint           pka_channel_get_id          (PkaChannel          *channel);
PkaChannelState pka_channel_get_state       (PkaChannel          *channel);
const gchar*    pka_channel_get_target      (PkaChannel          *channel);
const gchar*    pka_channel_get_working_dir (PkaChannel          *channel);
gchar**         pka_channel_get_args        (PkaChannel          *channel);
gchar**         pka_channel_get_env         (PkaChannel          *channel);
GPid            pka_channel_get_pid         (PkaChannel          *channel);
gboolean        pka_channel_start           (PkaChannel          *channel,
                                             GError             **error);
gboolean        pka_channel_stop            (PkaChannel          *channel,
                                             gboolean             killpid,
                                             GError             **error);
gboolean        pka_channel_pause           (PkaChannel          *channel,
                                             GError             **error);
gboolean        pka_channel_unpause         (PkaChannel          *channel,
                                             GError             **error);
PkaSource*      pka_channel_add_source      (PkaChannel          *channel,
                                             PkaSourceInfo       *source_info);
gboolean        pka_channel_remove_source   (PkaChannel          *channel,
                                             gint                 source_id,
                                             GError             **error);

G_END_DECLS

#endif /* __PKA_CHANNEL_H__ */
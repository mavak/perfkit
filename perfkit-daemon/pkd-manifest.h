/* pkd-manifest.h
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

#ifndef __PKD_MANIFEST_H__
#define __PKD_MANIFEST_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define PKD_TYPE_MANIFEST (pkd_manifest_get_type())

typedef struct _PkdManifest PkdManifest;

/**
 * PkdResolution:
 * @PKD_RESOLUTION_PRECISE: Samples include relative timestamp in 100-ns ticks.
 * @PKD_RESOLUTION_USEC: Samples include relative timestamp in microseconds.
 * @PKD_RESOLUTION_MSEC: Samples include relative timestamps in millseconds.
 * @PKD_RESOLUTION_SECOND: Samples include relative timestamps in seconds.
 * @PKD_RESOLUTION_MINUTE: Samples include relative timestamps in minutes.
 * @PKD_RESOLUTION_HOUR: Samples include relative timestamps in hours.
 *
 * The #PkdResolution enumeration.  Samples include their difference in time
 * since the describing #PkManifest.  The resolution allows control over how
 * that relative timestamp is encoded.
 *
 * Normally, a sample 1 second difference would be 100000000 100-ns ticks.
 * However, if @PKD_RESOLUTION_SECOND is used, it would be 1.  This is helpful
 * in that "varint" encoding allows for very tight compression of small
 * integers, thus saving precious bytes in the wire format.
 */
typedef enum
{
	PKD_RESOLUTION_PRECISE,
	PKD_RESOLUTION_USEC,
	PKD_RESOLUTION_MSEC,
	PKD_RESOLUTION_SECOND,
	PKD_RESOLUTION_MINUTE,
	PKD_RESOLUTION_HOUR,
} PkdResolution;

GType            pkd_manifest_get_type       (void) G_GNUC_CONST;
PkdManifest*     pkd_manifest_new            (void);
PkdManifest*     pkd_manifest_sized_new      (gint           size);
PkdManifest*     pkd_manifest_ref            (PkdManifest   *manifest);
void             pkd_manifest_unref          (PkdManifest   *manifest);
gint             pkd_manifest_get_source_id  (PkdManifest   *manifest);
PkdResolution    pkd_manifest_get_resolution (PkdManifest   *manifest);
void             pkd_manifest_set_resolution (PkdManifest   *manifest,
                                              PkdResolution  resolution);
void             pkd_manifest_get_timeval    (PkdManifest   *manifest,
                                              GTimeVal      *tv);
void             pkd_manifest_set_timeval    (PkdManifest   *manifest,
                                              GTimeVal      *tv);
guint            pkd_manifest_append         (PkdManifest   *manifest,
                                              const gchar   *name,
                                              GType          type);
guint            pkd_manifest_get_n_rows     (PkdManifest   *manifest);
GType            pkd_manifest_get_row_type   (PkdManifest   *manifest,
                                              gint           row);
const gchar*     pkd_manifest_get_row_name   (PkdManifest   *manifest,
                                              gint           row);

G_END_DECLS

#endif /* __PKD_MANIFEST_H__ */

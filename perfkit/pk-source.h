/* pk-source.h
 *
 * Copyright (C) 2010 Christian Hergert
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PK_SOURCE_H__
#define __PK_SOURCE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define PK_TYPE_SOURCE            (pk_source_get_type())
#define PK_SOURCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PK_TYPE_SOURCE, PkSource))
#define PK_SOURCE_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PK_TYPE_SOURCE, PkSource const))
#define PK_SOURCE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PK_TYPE_SOURCE, PkSourceClass))
#define PK_IS_SOURCE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PK_TYPE_SOURCE))
#define PK_IS_SOURCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PK_TYPE_SOURCE))
#define PK_SOURCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PK_TYPE_SOURCE, PkSourceClass))

typedef struct _PkSource        PkSource;
typedef struct _PkSourceClass   PkSourceClass;
typedef struct _PkSourcePrivate PkSourcePrivate;

struct _PkSource
{
	GObject parent;

	/*< private >*/
	PkSourcePrivate *priv;
};

struct _PkSourceClass
{
	GObjectClass parent_class;
};

GType    pk_source_get_type         (void) G_GNUC_CONST;
gboolean pk_source_set_prop_by_name (PkSource     *source,
                                     const gchar  *name,
                                     const GValue *value);
gboolean pk_source_get_prop_by_name (PkSource     *source,
                                     const gchar  *name,
                                     GValue       *value);

G_END_DECLS

#endif /* __PK_SOURCE_H__ */

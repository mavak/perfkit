/* pk-model-memory.h
 *
 * Copyright (C) 2010 Christian Hergert <chris@dronelabs.com>
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

#ifndef PK_MODEL_MEMORY_H
#define PK_MODEL_MEMORY_H

#include "pk-model.h"

G_BEGIN_DECLS

#define PK_TYPE_MODEL_MEMORY            (pk_model_memory_get_type())
#define PK_MODEL_MEMORY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PK_TYPE_MODEL_MEMORY, PkModelMemory))
#define PK_MODEL_MEMORY_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PK_TYPE_MODEL_MEMORY, PkModelMemory const))
#define PK_MODEL_MEMORY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PK_TYPE_MODEL_MEMORY, PkModelMemoryClass))
#define PK_IS_MODEL_MEMORY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PK_TYPE_MODEL_MEMORY))
#define PK_IS_MODEL_MEMORY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PK_TYPE_MODEL_MEMORY))
#define PK_MODEL_MEMORY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PK_TYPE_MODEL_MEMORY, PkModelMemoryClass))

typedef struct _PkModelMemory        PkModelMemory;
typedef struct _PkModelMemoryClass   PkModelMemoryClass;
typedef struct _PkModelMemoryPrivate PkModelMemoryPrivate;

struct _PkModelMemory
{
	GObject parent;

	/*< private >*/
	PkModelMemoryPrivate *priv;
};

struct _PkModelMemoryClass
{
	GObjectClass parent_class;
};

GType pk_model_memory_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* PK_MODEL_MEMORY_H */

/* ppg-ruler.h
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

#ifndef PPG_RULER_H
#define PPG_RULER_H

#include "ppg-header.h"

G_BEGIN_DECLS

#define PPG_TYPE_RULER            (ppg_ruler_get_type())
#define PPG_RULER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_RULER, PpgRuler))
#define PPG_RULER_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_RULER, PpgRuler const))
#define PPG_RULER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_RULER, PpgRulerClass))
#define PPG_IS_RULER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_RULER))
#define PPG_IS_RULER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_RULER))
#define PPG_RULER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_RULER, PpgRulerClass))

typedef struct _PpgRuler        PpgRuler;
typedef struct _PpgRulerClass   PpgRulerClass;
typedef struct _PpgRulerPrivate PpgRulerPrivate;

struct _PpgRuler
{
	PpgHeader parent;

	/*< private >*/
	PpgRulerPrivate *priv;
};

struct _PpgRulerClass
{
	PpgHeaderClass parent_class;
};

GType ppg_ruler_get_type       (void) G_GNUC_CONST;
void  ppg_ruler_get_range      (PpgRuler *ruler,
                                gdouble  *lower,
                                gdouble  *upper,
                                gdouble  *position);
void  ppg_ruler_set_range      (PpgRuler *ruler,
                                gdouble   lower,
                                gdouble   upper,
                                gdouble   position);
gdouble ppg_ruler_get_position (PpgRuler *ruler);
void    ppg_ruler_set_position (PpgRuler *ruler,
                                gdouble   position);

G_END_DECLS

#endif /* PPG_RULER_H */

/* ppg-clock-source.h
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

#ifndef PPG_CLOCK_SOURCE_H
#define PPG_CLOCK_SOURCE_H

#include <perfkit/perfkit.h>

G_BEGIN_DECLS

#define PPG_TYPE_CLOCK_SOURCE            (ppg_clock_source_get_type())
#define PPG_CLOCK_SOURCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_CLOCK_SOURCE, PpgClockSource))
#define PPG_CLOCK_SOURCE_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), PPG_TYPE_CLOCK_SOURCE, PpgClockSource const))
#define PPG_CLOCK_SOURCE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  PPG_TYPE_CLOCK_SOURCE, PpgClockSourceClass))
#define PPG_IS_CLOCK_SOURCE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PPG_TYPE_CLOCK_SOURCE))
#define PPG_IS_CLOCK_SOURCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  PPG_TYPE_CLOCK_SOURCE))
#define PPG_CLOCK_SOURCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  PPG_TYPE_CLOCK_SOURCE, PpgClockSourceClass))

typedef struct _PpgClockSource        PpgClockSource;
typedef struct _PpgClockSourceClass   PpgClockSourceClass;
typedef struct _PpgClockSourcePrivate PpgClockSourcePrivate;

struct _PpgClockSource
{
	GInitiallyUnowned parent;

	/*< private >*/
	PpgClockSourcePrivate *priv;
};

struct _PpgClockSourceClass
{
	GInitiallyUnownedClass parent_class;
};

void    ppg_clock_source_freeze         (PpgClockSource *source);
GType   ppg_clock_source_get_type       (void) G_GNUC_CONST;
void    ppg_clock_source_set_interval   (PpgClockSource *source,
                                         guint            msec);
void    ppg_clock_source_start          (PpgClockSource *source,
                                         const GTimeVal *tv);
void    ppg_clock_source_unfreeze       (PpgClockSource *source);
gdouble ppg_clock_source_get_elapsed    (PpgClockSource *source);

G_END_DECLS

#endif /* PPG_CLOCK_SOURCE_H */

/* ppg-animation.c
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

#include <gtk/gtk.h>

#include "ppg-animation.h"

G_DEFINE_TYPE(PpgAnimation, ppg_animation, G_TYPE_INITIALLY_UNOWNED)

#define TIMEVAL_TO_MSEC(t) (((t).tv_sec * 1000UL) + ((t).tv_usec / 1000UL))
#define LAST_FUNDAMENTAL 64
#define TWEEN(type)                                         \
    static void                                             \
    tween_##type (const GValue *begin,                      \
                  const GValue *end,                        \
                  GValue *value,                            \
                  gdouble offset)                           \
    {                                                       \
    	g##type x = g_value_get_##type(begin);              \
    	g##type y = g_value_get_##type(end);                \
    	g_value_set_##type(value, x + ((y - x) * offset));  \
    }

typedef gdouble (*AlphaFunc) (gdouble offset);
typedef void    (*TweenFunc) (const GValue *begin,
                              const GValue *end,
                              GValue       *value,
                              gdouble       offset);

typedef struct
{
	gboolean    is_child;
	GParamSpec *pspec;
	GValue      begin;
	GValue      end;
} Tween;

struct _PpgAnimationPrivate
{
	gpointer  target;        /* Target object to animate */
	guint64   begin_msec;    /* Time in which animation started */
	guint     duration_msec; /* Duration of animation */
	guint     mode;          /* Tween mode */
	guint     tween_handler; /* GSource performing tweens */
	GArray   *tweens;        /* Array of tweens to perform */
};

enum
{
	PROP_0,
	PROP_DURATION,
	PROP_MODE,
	PROP_TARGET,
};

static AlphaFunc alpha_funcs[PPG_ANIMATION_LAST];
static TweenFunc tween_funcs[LAST_FUNDAMENTAL];

TWEEN(int);
TWEEN(uint);
TWEEN(long);
TWEEN(ulong);
TWEEN(float);
TWEEN(double);

static gdouble
alpha_linear (gdouble offset)
{
	return offset;
}

static gdouble
alpha_ease_in_quad (gdouble offset)
{
	return offset * offset;
}

static gdouble
alpha_ease_out_quad (gdouble offset)
{
	return -1.0 * offset * (offset - 2.0);
}

static void
ppg_animation_load_begin_values (PpgAnimation *animation)
{
	PpgAnimationPrivate *priv;
	GtkContainer *container;
	Tween *tween;
	gint i;

	g_return_if_fail(PPG_IS_ANIMATION(animation));

	priv = animation->priv;

	for (i = 0; i < priv->tweens->len; i++) {
		tween = &g_array_index(priv->tweens, Tween, i);
		g_value_reset(&tween->begin);
		if (tween->is_child) {
			container = GTK_CONTAINER(gtk_widget_get_parent(priv->target));
			gtk_container_child_get_property(container, priv->target,
			                                 tween->pspec->name,
			                                 &tween->begin);
		} else {
			g_object_get_property(priv->target, tween->pspec->name,
			                      &tween->begin);
		}
	}
}

static void
ppg_animation_unload_begin_values (PpgAnimation *animation)
{
	PpgAnimationPrivate *priv;
	Tween *tween;
	gint i;

	g_return_if_fail(PPG_IS_ANIMATION(animation));

	priv = animation->priv;

	for (i = 0; i < priv->tweens->len; i++) {
		tween = &g_array_index(priv->tweens, Tween, i);
		g_value_reset(&tween->begin);
	}
}

static gdouble
ppg_animation_get_offset(PpgAnimation *animation)
{
	PpgAnimationPrivate *priv;
	GTimeVal now;
	guint64 msec;
	gdouble offset;

	g_return_val_if_fail(PPG_IS_ANIMATION(animation), 0.0);

	priv = animation->priv;

	g_get_current_time(&now);
	msec = TIMEVAL_TO_MSEC(now);
	offset = (gdouble)(msec - priv->begin_msec)
	       / (gdouble)priv->duration_msec;
	return CLAMP(offset, 0.0, 1.0);
}

static void
ppg_animation_update_property (PpgAnimation *animation,
                               gpointer      target,
                               Tween        *tween,
                               const GValue *value)
{
	g_object_set_property(target, tween->pspec->name, value);
}

static void
ppg_animation_update_child_property (PpgAnimation *animation,
                                     gpointer      target,
                                     Tween        *tween,
                                     const GValue *value)
{
	GtkWidget *parent;

	parent = gtk_widget_get_parent(GTK_WIDGET(target)),
	gtk_container_child_set_property(GTK_CONTAINER(parent), target,
	                                 tween->pspec->name, value);
}

static void
ppg_animation_get_value_at_offset (PpgAnimation *animation,
                                   gdouble       offset,
                                   Tween        *tween,
                                   GValue       *value)
{
	PpgAnimationPrivate *priv;

	g_return_if_fail(PPG_IS_ANIMATION(animation));
	g_return_if_fail(offset >= 0.0);
	g_return_if_fail(offset <= 1.0);
	g_return_if_fail(tween != NULL);
	g_return_if_fail(value != NULL);
	g_return_if_fail(value->g_type == tween->pspec->value_type);

	priv = animation->priv;

	if (value->g_type < LAST_FUNDAMENTAL) {
		/*
		 * XXX: If you hit the following assertion, you need to add a function
		 *      to create the new value at the given offset.
		 */
		g_assert(tween_funcs[value->g_type]);
		tween_funcs[value->g_type](&tween->begin, &tween->end, value, offset);
	} else {
		/*
		 * TODO: Support complex transitions.
		 */
		if (offset >= 1.0) {
			g_value_copy(&tween->end, value);
		}
	}
}

static gboolean
ppg_animation_tick (PpgAnimation *animation)
{
	PpgAnimationPrivate *priv;
	gdouble offset;
	gdouble alpha;
	GValue value = { 0 };
	Tween *tween;
	gint i;

	g_return_val_if_fail(PPG_IS_ANIMATION(animation), FALSE);

	priv = animation->priv;
	offset = ppg_animation_get_offset(animation);
	alpha = alpha_funcs[priv->mode](offset);

	for (i = 0; i < priv->tweens->len; i++) {
		tween = &g_array_index(priv->tweens, Tween, i);

		g_value_init(&value, tween->pspec->value_type);
		ppg_animation_get_value_at_offset(animation, alpha, tween, &value);

		if (!tween->is_child) {
			ppg_animation_update_property(animation, priv->target,
			                              tween, &value);
		} else {
			ppg_animation_update_child_property(animation, priv->target,
			                                    tween, &value);
		}

		g_value_unset(&value);
	}

	return (offset < 1.0);
}

static gboolean
ppg_animation_timeout (gpointer data)
{
	PpgAnimation *animation = (PpgAnimation *)data;
	gboolean ret;

	if (!(ret = ppg_animation_tick(animation))) {
		ppg_animation_stop(animation);
	}

	return ret;
}

void
ppg_animation_start (PpgAnimation *animation)
{
	PpgAnimationPrivate *priv;
	GTimeVal now;

	g_return_if_fail(PPG_IS_ANIMATION(animation));
	g_return_if_fail(!animation->priv->tween_handler);

	priv = animation->priv;

	g_get_current_time(&now);
	g_object_ref_sink(animation);
	ppg_animation_load_begin_values(animation);

	priv->begin_msec = TIMEVAL_TO_MSEC(now);
	priv->tween_handler = g_timeout_add(33, ppg_animation_timeout, animation);
	ppg_animation_tick(animation);
}

void
ppg_animation_stop (PpgAnimation *animation)
{
	PpgAnimationPrivate *priv;

	g_return_if_fail(PPG_IS_ANIMATION(animation));
	g_return_if_fail(animation->priv->tween_handler);

	priv = animation->priv;

	g_source_remove(priv->tween_handler);
	priv->tween_handler = 0;
	ppg_animation_unload_begin_values(animation);
	g_object_unref(animation);
}

void
ppg_animation_add_property (PpgAnimation *animation,
                            GParamSpec   *pspec,
                            const GValue *value)
{
	PpgAnimationPrivate *priv;
	Tween tween = { 0 };
	GType type;

	g_return_if_fail(PPG_IS_ANIMATION(animation));
	g_return_if_fail(pspec != NULL);
	g_return_if_fail(value != NULL);
	g_return_if_fail(value->g_type);
	g_return_if_fail(animation->priv->target);
	g_return_if_fail(!animation->priv->tween_handler);

	priv = animation->priv;
	type = G_TYPE_FROM_INSTANCE(priv->target);

	tween.is_child = !g_type_is_a(type, pspec->owner_type);
	if (tween.is_child) {
		if (!GTK_IS_WIDGET(priv->target)) {
			g_critical("Cannot locate property %s in class %s",
			           pspec->name, g_type_name(type));
			return;
		}
	}

	tween.pspec = g_param_spec_ref(pspec);
	g_value_init(&tween.begin, pspec->value_type);
	g_value_init(&tween.end, pspec->value_type);
	g_value_copy(value, &tween.end);
	g_array_append_val(priv->tweens, tween);
}

static void
ppg_animation_dispose (GObject *object)
{
	PpgAnimationPrivate *priv = PPG_ANIMATION(object)->priv;
	gpointer instance;

	if ((instance = priv->target)) {
		priv->target = NULL;
		g_object_unref(instance);
	}

	G_OBJECT_CLASS(ppg_animation_parent_class)->dispose(object);
}

static void
ppg_animation_finalize (GObject *object)
{
	PpgAnimationPrivate *priv = PPG_ANIMATION(object)->priv;
	Tween *tween;
	gint i;

	for (i = 0; i < priv->tweens->len; i++) {
		tween = &g_array_index(priv->tweens, Tween, i);
		g_value_unset(&tween->begin);
		g_value_unset(&tween->end);
		g_param_spec_unref(tween->pspec);
	}

	g_array_unref(priv->tweens);

	G_OBJECT_CLASS(ppg_animation_parent_class)->finalize(object);
}

static void
ppg_animation_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
	PpgAnimation *animation = PPG_ANIMATION(object);

	switch (prop_id) {
	case PROP_DURATION:
		animation->priv->duration_msec = g_value_get_uint(value);
		break;
	case PROP_MODE:
		animation->priv->mode = g_value_get_uint(value);
		break;
	case PROP_TARGET:
		animation->priv->target = g_value_dup_object(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}

static void
ppg_animation_class_init (PpgAnimationClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS(klass);
	object_class->dispose = ppg_animation_dispose;
	object_class->finalize = ppg_animation_finalize;
	object_class->set_property = ppg_animation_set_property;
	g_type_class_add_private(object_class, sizeof(PpgAnimationPrivate));

	g_object_class_install_property(object_class,
	                                PROP_DURATION,
	                                g_param_spec_uint("duration",
	                                                  "Duration",
	                                                  "The duration of the animation",
	                                                  0,
	                                                  G_MAXUINT,
	                                                  250,
	                                                  G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

	/*
	 * TODO: Convert PpgAnimation to a GEnum.
	 */
	g_object_class_install_property(object_class,
	                                PROP_MODE,
	                                g_param_spec_uint("mode",
	                                                  "Mode",
	                                                  "The animation mode",
	                                                  0,
	                                                  PPG_ANIMATION_LAST,
	                                                  0,
	                                                  G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property(object_class,
	                                PROP_TARGET,
	                                g_param_spec_object("target",
	                                                    "Target",
	                                                    "The target of the animation",
	                                                    G_TYPE_OBJECT,
	                                                    G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

#define SET_ALPHA(_T, _t) \
	alpha_funcs[PPG_ANIMATION_##_T] = alpha_##_t

	SET_ALPHA(LINEAR, linear);
	SET_ALPHA(EASE_IN_QUAD, ease_in_quad);
	SET_ALPHA(EASE_OUT_QUAD, ease_out_quad);

#define SET_TWEEN(_T, _t) \
	G_STMT_START { \
		guint idx = G_TYPE_##_T; \
		tween_funcs[idx] = tween_##_t; \
	} G_STMT_END

	SET_TWEEN(INT, int);
	SET_TWEEN(UINT, uint);
	SET_TWEEN(LONG, long);
	SET_TWEEN(ULONG, ulong);
	SET_TWEEN(FLOAT, float);
	SET_TWEEN(DOUBLE, double);
}

static void
ppg_animation_init (PpgAnimation *animation)
{
	PpgAnimationPrivate *priv;

	priv = G_TYPE_INSTANCE_GET_PRIVATE(animation, PPG_TYPE_ANIMATION,
	                                   PpgAnimationPrivate);
	animation->priv = priv;

	priv->duration_msec = 250;
	priv->mode = PPG_ANIMATION_LINEAR;
	priv->tweens = g_array_new(FALSE, FALSE, sizeof(Tween));
}

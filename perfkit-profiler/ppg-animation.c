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
#include "ppg-frame-source.h"


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


G_DEFINE_TYPE(PpgAnimation, ppg_animation, G_TYPE_INITIALLY_UNOWNED)


typedef gdouble (*AlphaFunc) (gdouble       offset);
typedef void    (*TweenFunc) (const GValue *begin,
                              const GValue *end,
                              GValue       *value,
                              gdouble       offset);

typedef struct
{
	gboolean    is_child; /* Does GParamSpec belong to parent widget */
	GParamSpec *pspec;    /* GParamSpec of target property */
	GValue      begin;    /* Begin value in animation */
	GValue      end;      /* End value in animation */
} Tween;


struct _PpgAnimationPrivate
{
	gpointer  target;        /* Target object to animate */
	guint64   begin_msec;    /* Time in which animation started */
	guint     duration_msec; /* Duration of animation */
	guint     mode;          /* Tween mode */
	guint     tween_handler; /* GSource performing tweens */
	GArray   *tweens;        /* Array of tweens to perform */
	guint     frame_rate;    /* The frame-rate to use */
	guint     frame_count;   /* Counter for debugging frames rendered */
};


enum
{
	PROP_0,
	PROP_DURATION,
	PROP_FRAME_RATE,
	PROP_MODE,
	PROP_TARGET,
};


enum
{
	TICK,
	LAST_SIGNAL
};


/*
 * Globals.
 */
static AlphaFunc alpha_funcs[PPG_ANIMATION_LAST] = { NULL };
static TweenFunc tween_funcs[LAST_FUNDAMENTAL] = { NULL };
static guint     signals[LAST_SIGNAL] = { 0 };
static gboolean  debug = FALSE;


/*
 * Tweeners for basic types.
 */
TWEEN(int);
TWEEN(uint);
TWEEN(long);
TWEEN(ulong);
TWEEN(float);
TWEEN(double);


/**
 * ppg_animation_alpha_linear:
 * @offset: (in): The position within the animation; 0.0 to 1.0.
 *
 * An alpha function to transform the offset within the animation.
 * @PPG_ANIMATION_LINEAR means no tranformation will be made.
 *
 * Returns: @offset.
 * Side effects: None.
 */
static gdouble
ppg_animation_alpha_linear (gdouble offset)
{
	return offset;
}


/**
 * ppg_animation_alpha_ease_in_quad:
 * @offset: (in): The position within the animation; 0.0 to 1.0.
 *
 * An alpha function to transform the offset within the animation.
 * @PPG_ANIMATION_EASE_IN_QUAD means that the value will be transformed
 * into a quadratic acceleration.
 *
 * Returns: A tranformation of @offset.
 * Side effects: None.
 */
static gdouble
ppg_animation_alpha_ease_in_quad (gdouble offset)
{
	return offset * offset;
}


/**
 * ppg_animation_alpha_ease_out_quad:
 * @offset: (in): The position within the animation; 0.0 to 1.0.
 *
 * An alpha function to transform the offset within the animation.
 * @PPG_ANIMATION_EASE_OUT_QUAD means that the value will be transformed
 * into a quadratic deceleration.
 *
 * Returns: A tranformation of @offset.
 * Side effects: None.
 */
static gdouble
ppg_animation_alpha_ease_out_quad (gdouble offset)
{
	return -1.0 * offset * (offset - 2.0);
}


/**
 * ppg_animation_alpha_ease_in_out_quad:
 * @offset: (in): The position within the animation; 0.0 to 1.0.
 *
 * An alpha function to transform the offset within the animation.
 * @PPG_ANIMATION_EASE_IN_OUT_QUAD means that the value will be transformed
 * into a quadratic acceleration for the first half, and quadratic
 * deceleration the second half.
 *
 * Returns: A tranformation of @offset.
 * Side effects: None.
 */
static gdouble
ppg_animation_alpha_ease_in_out_quad (gdouble offset)
{
	offset *= 2.0;
	if (offset < 1.0) {
		return 0.5 * offset * offset;
	}
	offset -= 1.0;
	return -0.5 * (offset * (offset - 2.0) - 1.0);
}


/**
 * ppg_animation_load_begin_values:
 * @animation: (in): A #PpgAnimation.
 *
 * Load the begin values for all the properties we are about to
 * animate.
 *
 * Returns: None.
 * Side effects: None.
 */
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


/**
 * ppg_animation_unload_begin_values:
 * @animation: (in): A #PpgAnimation.
 *
 * Unloads the begin values for the animation. This might be particularly
 * useful once we support pointer types.
 *
 * Returns: None.
 * Side effects: None.
 */
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


/**
 * ppg_animation_get_offset:
 * @animation: (in): A #PpgAnimation.
 *
 * Retrieves the position within the animation from 0.0 to 1.0. This
 * value is calculated using the msec of the beginning of the animation
 * and the current time.
 *
 * Returns: The offset of the animation from 0.0 to 1.0.
 * Side effects: None.
 */
static gdouble
ppg_animation_get_offset (PpgAnimation *animation)
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


/**
 * ppg_animation_update_property:
 * @animation: (in): A #PpgAnimation.
 * @target: (in): A #GObject.
 * @tween: (in): a #Tween containing the property.
 * @value: (in) The new value for the property.
 *
 * Updates the value of a property on an object using @value.
 *
 * Returns: None.
 * Side effects: The property of @target is updated.
 */
static void
ppg_animation_update_property (PpgAnimation *animation,
                               gpointer      target,
                               Tween        *tween,
                               const GValue *value)
{
	g_object_set_property(target, tween->pspec->name, value);
}


/**
 * ppg_animation_update_child_property:
 * @animation: (in): A #PpgAnimation.
 * @target: (in): A #GObject.
 * @tween: (in): A #Tween containing the property.
 * @value: (in): The new value for the property.
 *
 * Updates the value of the parent widget of the target to @value.
 *
 * Returns: None.
 * Side effects: The property of @target<!-- -->'s parent widget is updated.
 */
static void
ppg_animation_update_child_property (PpgAnimation *animation,
                                     gpointer      target,
                                     Tween        *tween,
                                     const GValue *value)
{
	GtkWidget *parent = gtk_widget_get_parent(GTK_WIDGET(target));
	gtk_container_child_set_property(GTK_CONTAINER(parent), target,
	                                 tween->pspec->name, value);
}


/**
 * ppg_animation_get_value_at_offset:
 * @animation: (in): A #PpgAnimation.
 * @offset: (in): The offset in the animation from 0.0 to 1.0.
 * @tween: (in): A #Tween containing the property.
 * @value: (out): A #GValue in which to store the property.
 *
 * Retrieves a value for a particular position within the animation.
 *
 * Returns: None.
 * Side effects: None.
 */
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
		 * If you hit the following assertion, you need to add a function
		 * to create the new value at the given offset.
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


/**
 * ppg_animation_tick:
 * @animation: (in): A #PpgAnimation.
 *
 * Moves the object properties to the next position in the animation.
 *
 * Returns: %TRUE if the animation has not completed; otherwise %FALSE.
 * Side effects: None.
 */
static gboolean
ppg_animation_tick (PpgAnimation *animation)
{
	PpgAnimationPrivate *priv;
	GdkWindow *window;
	gdouble offset;
	gdouble alpha;
	GValue value = { 0 };
	Tween *tween;
	gint i;

	g_return_val_if_fail(PPG_IS_ANIMATION(animation), FALSE);

	priv = animation->priv;

	priv->frame_count++;
	offset = ppg_animation_get_offset(animation);
	alpha = alpha_funcs[priv->mode](offset);

	/*
	 * Update property values.
	 */
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

	/*
	 * Notify anyone interested in the tick signal.
	 */
	g_signal_emit(animation, signals[TICK], 0);

	/*
	 * Flush any outstanding events to the graphics server (in the case of X).
	 */
	if (GTK_IS_WIDGET(priv->target)) {
		if ((window = gtk_widget_get_window(GTK_WIDGET(priv->target)))) {
			gdk_window_flush(window);
		}
	}

	return (offset < 1.0);
}


/**
 * ppg_animation_timeout:
 * @data: (in): A #PpgAnimation.
 *
 * Timeout from the main loop to move to the next step of the animation.
 *
 * Returns: %TRUE until the animation has completed; otherwise %FALSE.
 * Side effects: None.
 */
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


/**
 * ppg_animation_start:
 * @animation: (in): A #PpgAnimation.
 *
 * Start the animation. When the animation stops, the internal reference will
 * be dropped and the animation may be finalized.
 *
 * Returns: None.
 * Side effects: None.
 */
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
	priv->tween_handler = ppg_frame_source_add(priv->frame_rate,
	                                           ppg_animation_timeout,
	                                           animation);
}


/**
 * ppg_animation_stop:
 * @animation: (in): A #PpgAnimation.
 *
 * Stops a running animation. The internal reference to the animation is
 * dropped and therefore may cause the object to finalize.
 *
 * Returns: None.
 * Side effects: None.
 */
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


/**
 * ppg_animation_add_property:
 * @animation: (in): A #PpgAnimation.
 * @pspec: (in): A #ParamSpec of @target or a #GtkWidget<!-- -->'s parent.
 * @value: (in): The new value for the property at the end of the animation.
 *
 * Adds a new property to the set of properties to be animated during the
 * lifetime of the animation.
 *
 * Returns: None.
 * Side effects: None.
 */
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


/**
 * ppg_animation_dispose:
 * @object: (in): A #PpgAnimation.
 *
 * Releases any object references the animation contains.
 *
 * Returns: None.
 * Side effects: None.
 */
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


/**
 * ppg_animation_finalize:
 * @object: (in): A #PpgAnimation.
 *
 * Finalizes the object and releases any resources allocated.
 *
 * Returns: None.
 * Side effects: None.
 */
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

	if (debug) {
		g_print("Rendered %d frames in %d msec animation.\n",
		        priv->frame_count, priv->duration_msec);
	}

	G_OBJECT_CLASS(ppg_animation_parent_class)->finalize(object);
}


/**
 * ppg_animation_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
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
	case PROP_FRAME_RATE:
		animation->priv->frame_rate = g_value_get_uint(value);
		break;
	case PROP_MODE:
		animation->priv->mode = g_value_get_enum(value);
		break;
	case PROP_TARGET:
		animation->priv->target = g_value_dup_object(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
	}
}


/**
 * ppg_animation_class_init:
 * @klass: (in): A #PpgAnimationClass.
 *
 * Initializes the GObjectClass.
 *
 * Returns: None.
 * Side effects: Properties, signals, and vtables are initialized.
 */
static void
ppg_animation_class_init (PpgAnimationClass *klass)
{
	GObjectClass *object_class;

	debug = !!g_getenv("PPG_ANIMATION_DEBUG");

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

	g_object_class_install_property(object_class,
	                                PROP_MODE,
	                                g_param_spec_enum("mode",
	                                                  "Mode",
	                                                  "The animation mode",
	                                                  PPG_TYPE_ANIMATION_MODE,
	                                                  PPG_ANIMATION_LINEAR,
	                                                  G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property(object_class,
	                                PROP_TARGET,
	                                g_param_spec_object("target",
	                                                    "Target",
	                                                    "The target of the animation",
	                                                    G_TYPE_OBJECT,
	                                                    G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property(object_class,
	                                PROP_FRAME_RATE,
	                                g_param_spec_uint("frame-rate",
	                                                  "frame-rate",
	                                                  "frame-rate",
	                                                  1,
	                                                  G_MAXUINT,
	                                                  60,
	                                                  G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

	signals[TICK] = g_signal_new("tick",
	                             PPG_TYPE_ANIMATION,
	                             G_SIGNAL_RUN_FIRST,
	                             0,
	                             NULL,
	                             NULL,
	                             g_cclosure_marshal_VOID__VOID,
	                             G_TYPE_NONE,
	                             0);

#define SET_ALPHA(_T, _t) \
	alpha_funcs[PPG_ANIMATION_##_T] = ppg_animation_alpha_##_t

	SET_ALPHA(LINEAR, linear);
	SET_ALPHA(EASE_IN_QUAD, ease_in_quad);
	SET_ALPHA(EASE_OUT_QUAD, ease_out_quad);
	SET_ALPHA(EASE_IN_OUT_QUAD, ease_in_out_quad);

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


/**
 * ppg_animation_init:
 * @animation: (in): A #PpgAnimation.
 *
 * Initializes the #PpgAnimation instance.
 *
 * Returns: None.
 * Side effects: Everything.
 */
static void
ppg_animation_init (PpgAnimation *animation)
{
	PpgAnimationPrivate *priv;

	priv = G_TYPE_INSTANCE_GET_PRIVATE(animation, PPG_TYPE_ANIMATION,
	                                   PpgAnimationPrivate);
	animation->priv = priv;

	priv->duration_msec = 250;
	priv->frame_rate = 60;
	priv->mode = PPG_ANIMATION_LINEAR;
	priv->tweens = g_array_new(FALSE, FALSE, sizeof(Tween));
}


/**
 * ppg_animation_mode_get_type:
 *
 * Retrieves the GType for #PpgAnimationMode.
 *
 * Returns: A GType.
 * Side effects: GType registered on first call.
 */
GType
ppg_animation_mode_get_type (void)
{
	static GType type_id = 0;
	static const GEnumValue values[] = {
		{ PPG_ANIMATION_LINEAR, "PPG_ANIMATION_LINEAR", "LINEAR" },
		{ PPG_ANIMATION_EASE_IN_QUAD, "PPG_ANIMATION_EASE_IN_QUAD", "EASE_IN_QUAD" },
		{ PPG_ANIMATION_EASE_IN_OUT_QUAD, "PPG_ANIMATION_EASE_IN_OUT_QUAD", "EASE_IN_OUT_QUAD" },
		{ PPG_ANIMATION_EASE_OUT_QUAD, "PPG_ANIMATION_EASE_OUT_QUAD", "EASE_OUT_QUAD" },
		{ 0 }
	};

	if (G_UNLIKELY(!type_id)) {
		type_id = g_enum_register_static("PpgAnimationMode", values);
	}
	return type_id;
}

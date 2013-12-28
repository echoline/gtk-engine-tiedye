/* Tiedye - a GTK+ engine
 *
 * Copyright (C) 2011 Carlos Garnacho <carlosg@gnome.org>
 * Copyright (C) 2011 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Authors: Carlos Garnacho <carlosg@gnome.org>
 *          Cosimo Cecchi <cosimoc@gnome.org>
 *          Eli Cohen <eli.neoturbine.net>
 */

#include <gtk/gtk.h>
#include <gmodule.h>
#include <math.h>
#include <cairo-gobject.h>
#include "blur.h"

#define TIEDYE_NAMESPACE "tiedye"

typedef struct _TiedyeEngine TiedyeEngine;
typedef struct _TiedyeEngineClass TiedyeEngineClass;

struct _TiedyeEngine
{
  GtkThemingEngine parent_object;
};

struct _TiedyeEngineClass
{
  GtkThemingEngineClass parent_class;
};

#define TIEDYE_TYPE_ENGINE		 (tiedye_engine_get_type ())
#define TIEDYE_ENGINE(object)		 (G_TYPE_CHECK_INSTANCE_CAST ((object), TIEDYE_TYPE_ENGINE, TiedyeEngine))
#define TIEDYE_ENGINE_CLASS(klass)	 (G_TYPE_CHECK_CLASS_CAST ((klass), TIEDYE_TYPE_ENGINE, TiedyeEngineClass))
#define TIEDYE_IS_ENGINE(object)	 (G_TYPE_CHECK_INSTANCE_TYPE ((object), TIEDYE_TYPE_ENGINE))
#define TIEDYE_IS_ENGINE_CLASS(klass)	 (G_TYPE_CHECK_CLASS_TYPE ((klass), TIEDYE_TYPE_ENGINE))
#define TIEDYE_ENGINE_GET_CLASS(obj)	 (G_TYPE_INSTANCE_GET_CLASS ((obj), TIEDYE_TYPE_ENGINE, TiedyeEngineClass))

GType tiedye_engine_get_type	    (void) G_GNUC_CONST;
void  tiedye_engine_register_types (GTypeModule *module);

G_DEFINE_DYNAMIC_TYPE (TiedyeEngine, tiedye_engine, GTK_TYPE_THEMING_ENGINE)

void
tiedye_engine_register_types (GTypeModule *module)
{
  tiedye_engine_register_type (module);
}

cairo_surface_t *pool = NULL;
gint pool_width = 0;
gint pool_height = 0;

static void
tiedye_engine_init (TiedyeEngine *self)
{
}

static void
pool_gen (gint width, gint height)
{
  gdouble xs, ys, xe, ye, r, inc;
  gint i;
  gdouble area = width * height;

  if (pool)
    cairo_surface_destroy (pool);

  pool = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
  cairo_t *cr = cairo_create(pool);

  cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 1.0);
  cairo_rectangle (cr, 0, 0, width, height);
  cairo_fill (cr);

  for (i = 0; i < area; i += 4000) {
    xs = g_random_double_range(0, width);
    ys = g_random_double_range(0, height);

    cairo_set_source_rgba (cr, g_random_double_range(0.5, 1.0),
				g_random_double_range(0.5, 1.0),
				g_random_double_range(0.5, 1.0), 1.0);

    for (inc = 0; inc < 2 * G_PI; inc += G_PI / 45.0) {
      r = g_random_double_range(25.0, 150.0);
      xe = xs + r * cos(inc);
      ye = ys + r * sin(inc);

      cairo_move_to (cr, xs, ys);
      cairo_line_to (cr, xe, ye);
    }
    cairo_stroke (cr);
  }

  cairo_destroy (cr);

  blur_image_surface (pool, 15, width, height);
}

static void
tiedye_engine_render_background (GtkThemingEngine *engine,
                                  cairo_t          *cr,
                                  gdouble           x,
                                  gdouble           y,
                                  gdouble           width,
                                  gdouble           height)
{
  GdkRGBA color;
  GtkStateFlags flags = gtk_theming_engine_get_state (engine);
  gboolean generate = FALSE;

  gtk_theming_engine_get_background_color (engine, flags, &color);

  cairo_save (cr);

  if (color.red == color.green == color.blue == 0) {
    if (width > pool_width) {
      generate = TRUE;
      pool_width = width;
    }
    if (height > pool_height) {
      generate = TRUE;
      pool_height = height;
    }
    if (generate) {
      pool_gen (width, height);
    }
    cairo_set_source_surface (cr, pool, 0, 0);
  }
  else
    gdk_cairo_set_source_rgba (cr, &color);

  cairo_rectangle (cr, x, y, width, height);
  cairo_fill (cr);

  cairo_restore (cr);
}

static void
tiedye_engine_class_init (TiedyeEngineClass *klass)
{
  GtkThemingEngineClass *engine_class = GTK_THEMING_ENGINE_CLASS (klass);

  engine_class->render_background = tiedye_engine_render_background;
}

static void
tiedye_engine_class_finalize (TiedyeEngineClass *klass)
{
}

G_MODULE_EXPORT void
theme_init (GTypeModule *module)
{
  tiedye_engine_register_types (module);
}

G_MODULE_EXPORT void
theme_exit (void)
{
}

G_MODULE_EXPORT GtkThemingEngine *
create_engine (void)
{
  return GTK_THEMING_ENGINE (g_object_new (TIEDYE_TYPE_ENGINE,
                                           "name", "tiedye",
                                           NULL));
}

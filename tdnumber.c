/*
 * tractordodge
 *
 * A sample game for the ClutterMD2 renderer
 *
 * Authored By Neil Roberts  <neil@o-hand.com>
 *
 * Copyright (C) 2008 OpenedHand
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <clutter/clutter-actor.h>
#include <cogl/cogl.h>
#include <cairo.h>
#include <string.h>

#include "tdnumber.h"

#define TD_NUMBER_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TD_TYPE_NUMBER, TDNumberPrivate))

G_DEFINE_TYPE (TDNumber, td_number, CLUTTER_TYPE_ACTOR)

static void td_number_paint (ClutterActor *self);
static void td_number_dispose (GObject *self);

static void td_number_get_preferred_width (ClutterActor *self,
					   ClutterUnit   for_height,
					   ClutterUnit  *min_width_p,
					   ClutterUnit  *natural_width_p);
static void td_number_get_preferred_height (ClutterActor *self,
					    ClutterUnit   for_width,
					    ClutterUnit  *min_height_p,
					    ClutterUnit  *natural_height_p);

#define TD_NUMBER_TEX_SIZE 256
#define TD_NUMBER_GAP      8

typedef struct _TDNumberBox TDNumberBox;

struct _TDNumberBox
{
  ClutterFixed tx1, ty1;
  ClutterFixed tx2, ty2;

  int width, height, advance, y_off;
};

struct _TDNumberPrivate
{
  CoglHandle tex;

  TDNumberBox boxes[10];

  int max_ascent;

  int value;
  char digits[16];
};

static void
td_number_class_init (TDNumberClass *klass)
{
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  actor_class->paint = td_number_paint;
  actor_class->get_preferred_width = td_number_get_preferred_width;
  actor_class->get_preferred_height = td_number_get_preferred_height;

  object_class->dispose = td_number_dispose;

  g_type_class_add_private (klass, sizeof (TDNumberPrivate));
}

static void
td_number_init (TDNumber *self)
{
  TDNumberPrivate *priv;
  cairo_t *cr;
  cairo_surface_t *surface;
  int xpos = 0, ypos = 0;
  int row_height = 0;
  unsigned char *image_data, *p;
  int stride, i;

  self->priv = priv = TD_NUMBER_GET_PRIVATE (self);

  priv->value = 0;
  priv->digits[0] = '0';
  priv->digits[1] = '\0';
  priv->max_ascent = 0;

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					TD_NUMBER_TEX_SIZE,
					TD_NUMBER_TEX_SIZE);

  cr = cairo_create (surface);

  cairo_set_line_width (cr, 4.0);

  cairo_set_font_size (cr, 40.0);

  for (i = 0; i < 10; i++)
    {
      char digit_str[2] = { i + '0', '\0' };
      cairo_text_extents_t extents;
      TDNumberBox *box = priv->boxes + i;

      cairo_text_extents (cr, digit_str, &extents);

      box->width = extents.width + TD_NUMBER_GAP;
      box->height = extents.height + TD_NUMBER_GAP;
      box->advance = extents.x_advance + 2;
      box->y_off = -extents.y_bearing;

      if (box->y_off > priv->max_ascent)
	priv->max_ascent = box->y_off;

      if (xpos + box->width > TD_NUMBER_TEX_SIZE)
	{
	  xpos = 0;
	  ypos += row_height;
	}

      box->tx1 = CFX_QDIV (CLUTTER_INT_TO_FIXED (xpos),
			   CLUTTER_INT_TO_FIXED (TD_NUMBER_TEX_SIZE));
      box->tx2 = box->tx1
	+ CFX_QDIV (CLUTTER_INT_TO_FIXED (box->width),
		    CLUTTER_INT_TO_FIXED (TD_NUMBER_TEX_SIZE));

      box->ty1 = CFX_QDIV (CLUTTER_INT_TO_FIXED (ypos),
			   CLUTTER_INT_TO_FIXED (TD_NUMBER_TEX_SIZE));
      box->ty2 = box->ty1
	+ CFX_QDIV (CLUTTER_INT_TO_FIXED (box->height),
		    CLUTTER_INT_TO_FIXED (TD_NUMBER_TEX_SIZE));

      cairo_move_to (cr, xpos - extents.x_bearing + TD_NUMBER_GAP / 2, 
		     ypos - extents.y_bearing + TD_NUMBER_GAP / 2);

      cairo_text_path (cr, digit_str);

      /* Draw an outline in black */
      cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
      cairo_stroke_preserve (cr);

      /* Fill in the shape in white */
      cairo_set_source_rgb (cr, 0.4, 0.4, 0.8);
      cairo_fill (cr);

      if (box->height > row_height)
	row_height = box->height;

      xpos += box->width;
    }

  priv->max_ascent += TD_NUMBER_GAP / 2;

  cairo_destroy (cr);

  image_data = cairo_image_surface_get_data (surface);
  stride = cairo_image_surface_get_stride (surface);

  /* Convert the image from cairo format to RGBA */
  for (p = image_data + TD_NUMBER_TEX_SIZE * stride; p > image_data;)
    {
      p -= stride - TD_NUMBER_TEX_SIZE * 4;
      
      for (i = 0; i < TD_NUMBER_TEX_SIZE; i++)
	{
	  guint32 v = *(guint32 *) (p -= sizeof (guint32));
	  guchar alpha = v >> 24;

	  if (alpha == 0)
	    memset (p, 0, sizeof (guint32));
	  else
	    {
	      p[0] = ((v >> 16) & 0xff) * 255 / alpha;
	      p[1] = ((v >> 8) & 0xff) * 255 / alpha;
	      p[2] = (v & 0xff) * 255 / alpha;
	      p[3] = alpha;
	    }
	}
    }

  /* Upload the image to a texture */
  priv->tex = cogl_texture_new_from_data (TD_NUMBER_TEX_SIZE,
					  TD_NUMBER_TEX_SIZE,
					  64,
					  FALSE,
					  COGL_PIXEL_FORMAT_RGBA_8888,
					  COGL_PIXEL_FORMAT_RGBA_8888,
					  stride,
					  image_data);

  cairo_surface_destroy (surface);
}

ClutterActor *
td_number_new (void)
{
  return g_object_new (TD_TYPE_NUMBER, NULL);
}

static void
td_number_paint (ClutterActor *self)
{
  TDNumber *num = TD_NUMBER (self);
  TDNumberPrivate *priv = num->priv;
  char *p;
  int xpos = 0;
  static const ClutterColor white = { 0xff, 0xff, 0xff, 0xff };

  if (priv->tex == COGL_INVALID_HANDLE)
    return;

  cogl_color (&white);

  for (p = priv->digits; *p; p++)
    {
      TDNumberBox *box = priv->boxes + *p - '0';

      cogl_texture_rectangle (priv->tex,
			      CLUTTER_INT_TO_FIXED (xpos),
			      CLUTTER_INT_TO_FIXED (priv->max_ascent
						    - box->y_off),
			      CLUTTER_INT_TO_FIXED (xpos + box->width),
			      CLUTTER_INT_TO_FIXED (box->height
						    + priv->max_ascent
						    - box->y_off),
			      box->tx1, box->ty1, box->tx2, box->ty2);

      xpos += box->advance;
    }
}

void
td_number_set_value (TDNumber *number, int value)
{
  TDNumberPrivate *priv;
  char *src, *dst;

  g_return_if_fail (TD_IS_NUMBER (number));

  priv = number->priv;

  if (priv->value != value)
    {
      priv->value = value;
      g_snprintf (priv->digits, sizeof (priv->digits), "%i", value);

      /* Remove any non-numeric characters */
      dst = src = priv->digits;
      do
	if (*src == 0 || (*src >= '0' && *src <= '9'))
	  *(dst++) = *src;
      while (*(src++));

      clutter_actor_queue_relayout (CLUTTER_ACTOR (number));
    }
}

int
td_number_get_value (TDNumber *number)
{
  g_return_val_if_fail (TD_IS_NUMBER (number), 0);

  return number->priv->value;
}

static void
td_number_dispose (GObject *self)
{
  TDNumber *num = TD_NUMBER (self);
  TDNumberPrivate *priv = num->priv;

  if (priv->tex != COGL_INVALID_HANDLE)
    {
      cogl_texture_unref (priv->tex);
      priv->tex = COGL_INVALID_HANDLE;
    }
}

static void
td_number_get_preferred_width (ClutterActor *self,
			       ClutterUnit   for_height,
			       ClutterUnit  *min_width_p,
			       ClutterUnit  *natural_width_p)
{
  TDNumberPrivate *priv = TD_NUMBER (self)->priv;
  int width = 0;
  char *p;

  /* Sum all of the advances of the boxes for the digits */
  for (p = priv->digits; *p; p++)
    width += priv->boxes[*p - '0'].advance;

  if (min_width_p)
    *min_width_p = CLUTTER_UNITS_FROM_DEVICE (width);

  if (natural_width_p)
    *natural_width_p = CLUTTER_UNITS_FROM_DEVICE (width);
}

static void
td_number_get_preferred_height (ClutterActor *self,
				ClutterUnit   for_width,
				ClutterUnit  *min_height_p,
				ClutterUnit  *natural_height_p)
{
  TDNumberPrivate *priv = TD_NUMBER (self)->priv;
  int height = 0;
  char *p;

  /* Get the maximum of all of the heights */
  for (p = priv->digits; *p; p++)
    {
      TDNumberBox *box = priv->boxes + *p - '0';
      int box_height = box->height + priv->max_ascent - box->y_off;
      if (height < box_height)
	height = box_height;
    }

  if (min_height_p)
    *min_height_p = CLUTTER_UNITS_FROM_DEVICE (height);

  if (natural_height_p)
    *natural_height_p = CLUTTER_UNITS_FROM_DEVICE (height);
}

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

#ifndef _HAVE_TD_CORNER_LAYOUT_H
#define _HAVE_TD_CORNER_LAYOUT_H

#include <glib-object.h>
#include <clutter/clutter-actor.h>

G_BEGIN_DECLS

#define TD_TYPE_CORNER_LAYOUT			\
  (td_corner_layout_get_type ())
#define TD_CORNER_LAYOUT(obj)						\
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TD_TYPE_CORNER_LAYOUT, TDCornerLayout))
#define TD_IS_CORNER_LAYOUT(obj)				\
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TD_TYPE_CORNER_LAYOUT))
#define TD_CORNER_LAYOUT_CLASS(klass)				\
  (G_TYPE_CHECK_CLASS_CAST ((klass),  TD_TYPE_CORNER_LAYOUT,	\
			    TDCornerLayoutClass))
#define TD_IS_CORNER_LAYOUT_CLASS(klass)			\
  (G_TYPE_CHECK_CLASS_TYPE ((klass),  TD_TYPE_CORNER_LAYOUT))
#define TD_CORNER_LAYOUT_GET_CLASS(obj)				\
  (G_TYPE_INSTANCE_GET_CLASS ((obj),  TD_TYPE_CORNER_LAYOUT,	\
			      TDCornerLayoutClass))

typedef struct _TDCornerLayout             TDCornerLayout;
typedef struct _TDCornerLayoutPrivate      TDCornerLayoutPrivate;
typedef struct _TDCornerLayoutClass        TDCornerLayoutClass;

struct _TDCornerLayout
{
  ClutterActor parent_instance;

  TDCornerLayoutPrivate *priv;
};

struct _TDCornerLayoutClass
{
  ClutterActorClass parent_class;
};

GType td_corner_layout_get_type (void) G_GNUC_CONST;

ClutterActor *td_corner_layout_new (void);

G_END_DECLS

#endif /* _HAVE_TD_CORNER_LAYOUT_H */

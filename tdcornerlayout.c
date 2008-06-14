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
#include <clutter/clutter-container.h>

#include "tdcornerlayout.h"

static void clutter_container_iface_init (ClutterContainerIface *iface);

G_DEFINE_TYPE_WITH_CODE (TDCornerLayout,
                         td_corner_layout,
                         CLUTTER_TYPE_ACTOR,
                         G_IMPLEMENT_INTERFACE (CLUTTER_TYPE_CONTAINER,
                                                clutter_container_iface_init));

#define TD_CORNER_LAYOUT_GET_PRIVATE(obj)			\
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TD_TYPE_CORNER_LAYOUT,	\
				TDCornerLayoutPrivate))

struct _TDCornerLayoutPrivate
{
  ClutterActor *child;
};

static void
td_corner_layout_real_add (ClutterContainer *container,
			   ClutterActor     *actor)
{
  TDCornerLayout *cl = TD_CORNER_LAYOUT (container);
  TDCornerLayoutPrivate *priv = cl->priv;

  if (priv->child != NULL)
    {
      g_warning ("can not add multiple actors to a TDCornerLayout");
      return;
    }

  priv->child = actor;

  clutter_actor_set_parent (actor, CLUTTER_ACTOR (cl));

  /* queue relayout to allocate new item */
  clutter_actor_queue_relayout (CLUTTER_ACTOR (cl));
}

static void
td_corner_layout_real_remove (ClutterContainer *container,
			      ClutterActor     *actor)
{
  TDCornerLayout *cl = TD_CORNER_LAYOUT (container);
  TDCornerLayoutPrivate *priv = cl->priv;

  if (actor == priv->child)
    {
      priv->child = NULL;
      clutter_actor_unparent (actor);
    }
}

static void
td_corner_layout_real_foreach (ClutterContainer *container,
			       ClutterCallback   callback,
			       gpointer          user_data)
{
  TDCornerLayoutPrivate *priv = TD_CORNER_LAYOUT (container)->priv;

  if (priv->child)
    (* callback) (priv->child, user_data);
}

static void
clutter_container_iface_init (ClutterContainerIface *iface)
{
  iface->add = td_corner_layout_real_add;
  iface->remove = td_corner_layout_real_remove;
  iface->foreach = td_corner_layout_real_foreach;
}

static void
td_corner_layout_dispose (GObject *gobject)
{
  TDCornerLayout *self = TD_CORNER_LAYOUT (gobject);
  TDCornerLayoutPrivate *priv = self->priv;

  if (priv->child)
    clutter_container_remove (CLUTTER_CONTAINER (gobject), priv->child, NULL);

  G_OBJECT_CLASS (td_corner_layout_parent_class)->dispose (gobject);
}

static void
td_corner_layout_allocate (ClutterActor          *self,
			   const ClutterActorBox *box,
			   gboolean               origin_changed)
{
  TDCornerLayoutPrivate *priv;

  /* chain up to set actor->allocation */
  CLUTTER_ACTOR_CLASS (td_corner_layout_parent_class)
    ->allocate (self, box, origin_changed);

  priv = TD_CORNER_LAYOUT (self)->priv;

  if (priv->child)
    {
      ClutterUnit natural_width, natural_height;
      ClutterActorBox child_box;

      /* Put the child in the top right of our allocation at its
	 natural size */
      clutter_actor_get_preferred_size (priv->child, NULL, NULL,
					&natural_width, &natural_height);

      child_box.x1 = box->x2 - natural_width;
      child_box.x2 = box->x2;
      child_box.y1 = 0;
      child_box.y2 = natural_height;

      clutter_actor_allocate (priv->child, &child_box, origin_changed);
    }
}

static void
td_corner_layout_paint (ClutterActor *actor)
{
  TDCornerLayoutPrivate *priv = TD_CORNER_LAYOUT (actor)->priv;

  /* paint the child if it is visible */
  if (priv->child != NULL && CLUTTER_ACTOR_IS_VISIBLE (priv->child))
    clutter_actor_paint (priv->child);
}

static void
td_corner_layout_class_init (TDCornerLayoutClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  gobject_class->dispose = td_corner_layout_dispose;

  actor_class->allocate = td_corner_layout_allocate;
  actor_class->paint = td_corner_layout_paint;

  g_type_class_add_private (klass, sizeof (TDCornerLayoutPrivate));
}

static void
td_corner_layout_init (TDCornerLayout *corner_layout)
{
  corner_layout->priv = TD_CORNER_LAYOUT_GET_PRIVATE (corner_layout);

  corner_layout->priv->child = NULL;
}

ClutterActor *
td_corner_layout_new ()
{
  return g_object_new (TD_TYPE_CORNER_LAYOUT, NULL);
}

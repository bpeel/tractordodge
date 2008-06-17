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

#include <clutter/clutter.h>
#include <clutter-md2/clutter-md2.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <stdlib.h>

#include "tdnumber.h"
#include "tdcornerlayout.h"

#define LINE_WIDTH         15
#define LINE_HEIGHT        30
#define LINE_GAP           20

#define CAR_MAX_ANGLE      25

#define TRACTOR_RATE_MIN   1
#define TRACTOR_RATE_START 10

typedef struct _LineCallbackData LineCallbackData;

struct _LineCallbackData
{
  int y_offset, road_start, road_end;
  ClutterActor *line;
};

typedef struct _GameData GameData;

struct _GameData
{
  ClutterEffectTemplate *eft;
  ClutterActor *group;
  ClutterMD2Data *tractor_data;
  int tractor_size, road_width, road_left;
  int road_start, road_end;
  int add_rate;

  ClutterActor *car;
  float angle;
  int rotate_direction;
  float position;
  int stage_width;

  ClutterActor *number;
  int score;
};

#define ROTATE_SPEED     80 /* Degrees per second */
#define STRAIGHTEN_SPEED 20

#define FULL_MOVE_SPEED  0.5 /* stage widths per second */

static void
on_car_rotate_frame (ClutterTimeline *tl, int frame_num, GameData *data)
{
  guint delta = clutter_timeline_get_delta (tl, NULL);
  guint speed = clutter_timeline_get_speed (tl);

  if (data->rotate_direction < 0)
    {
      data->angle -= delta * ROTATE_SPEED / (float) speed;

      if (data->angle < -CAR_MAX_ANGLE)
	data->angle = -CAR_MAX_ANGLE;
    }
  else if (data->rotate_direction == 0)
    {
      float diff = delta * STRAIGHTEN_SPEED / (float) speed;

      if (data->angle < 0)
	{
	  data->angle += diff;
	  if (data->angle > 0)
	    data->angle = 0;
	}
      else if (data->angle > 0)
	{
	  data->angle -= diff;
	  if (data->angle < 0)
	    data->angle = 0;
	}
    }
  else
    {
      data->angle += delta * ROTATE_SPEED / (float) speed;

      if (data->angle > CAR_MAX_ANGLE)
	data->angle = CAR_MAX_ANGLE;
    }

  clutter_actor_set_rotation (data->car, CLUTTER_Z_AXIS, data->angle,
			      clutter_actor_get_width (data->car) / 2,
			      clutter_actor_get_height (data->car) / 2,
			      0);

  if (data->angle != 0)
    {
      float slide_speed = data->angle * (float) FULL_MOVE_SPEED / CAR_MAX_ANGLE;
      float offset = delta * slide_speed / speed * data->stage_width;

      data->position += offset;

      if (data->position < 0.0f)
	data->position = 0.0f;
      else if (data->position > data->stage_width)
	data->position = data->stage_width;
      
      clutter_actor_set_x (data->car,
			   data->position
			   - clutter_actor_get_width (data->car) / 2);
    }
}

static void
on_key_press (ClutterActor *stage, ClutterKeyEvent *event, GameData *data)
{
  switch (event->keyval)
    {
    case CLUTTER_Left:
      data->rotate_direction = -1;
      break;

    case CLUTTER_Right:
      data->rotate_direction = 1;
      break;

    case CLUTTER_s:
      {
	int width = clutter_actor_get_width (stage);
	int height = clutter_actor_get_height (stage);
	guchar *data = clutter_stage_read_pixels (CLUTTER_STAGE (stage),
						  0, 0, width, height);
	guchar *p;
	GdkPixbuf *pb;

	for (p = data + width * height * 4; p > data; p -= 3)
	  *(--p) = 0xff;

	pb = gdk_pixbuf_new_from_data (data, GDK_COLORSPACE_RGB, TRUE,
				       8, width, height, width * 4,
				       (GdkPixbufDestroyNotify) g_free,
				       NULL);

	gdk_pixbuf_save (pb, "screenie.png", "png", NULL, NULL);

	g_object_unref (pb);
      }
    }
}

static void
on_key_release (ClutterActor *stage, ClutterKeyEvent *event, GameData *data)
{
  switch (event->keyval)
    {
    case CLUTTER_Left:
    case CLUTTER_Right:
      data->rotate_direction = 0;
      break;
    }
}

static void
destroy_line_callback_data (gpointer data, GClosure *closure)
{
  g_slice_free (LineCallbackData, data);
}

static void
on_new_frame_for_line (ClutterTimeline *tl,
		       gint frame_num,
		       LineCallbackData *data)
{
  int num_frames = clutter_timeline_get_n_frames (tl);

  clutter_actor_set_y (data->line,
		       (data->y_offset
			- data->road_start
			+ (frame_num * (data->road_end - data->road_start)
			   / num_frames))
		       % (data->road_end - data->road_start)
		       - LINE_HEIGHT - LINE_GAP + data->road_start);
}

static ClutterTimeline *
make_line_animation (ClutterActor *group, int stage_width,
		     int road_start, int road_end)
{
  int ypos = road_start;
  ClutterTimeline *tl;
  static const ClutterColor line_color = { 0xe0, 0xe0, 0x00, 0xff };

  tl = clutter_timeline_new_for_duration (4000);

  /* Extend the road to be a multiple of the distance between lines */
  road_end += ((LINE_HEIGHT + LINE_GAP - 1)
	       - (road_end - road_start) % (LINE_HEIGHT + LINE_GAP))
    + LINE_HEIGHT + LINE_GAP;

  while (ypos < road_end)
    {
      ClutterActor *line = clutter_rectangle_new_with_color (&line_color);
      LineCallbackData *data;
      
      clutter_actor_set_position (line, stage_width / 2 - LINE_WIDTH / 2,
				  ypos - LINE_HEIGHT - LINE_GAP);
      clutter_actor_set_size (line, LINE_WIDTH, LINE_HEIGHT);
      
      clutter_container_add (CLUTTER_CONTAINER (group), line, NULL);

      data = g_slice_new (LineCallbackData);
      data->y_offset = ypos;
      data->road_start = road_start;
      data->road_end = road_end;
      data->line = line;

      g_signal_connect_data (tl, "new-frame",
			     G_CALLBACK (on_new_frame_for_line),
			     data,
			     destroy_line_callback_data,
			     0);

      ypos += LINE_HEIGHT + LINE_GAP;
    }

  return tl;
}

static ClutterMD2Data *
get_data (const char *filename)
{
  ClutterMD2Data *data = clutter_md2_data_new ();
  GError *error = NULL;

  g_object_ref_sink (data);

  if (!clutter_md2_data_load (data, filename, &error))
    {
      g_critical ("%s: %s\n", filename, error->message);
      g_error_free (error);
    }

  return data;
}

static void
add_skin (ClutterMD2Data *data, const char *filename)
{
  GError *error = NULL;

  if (!clutter_md2_data_add_skin (data, filename, &error))
    {
      g_critical ("%s: %s\n", filename, error->message);
      g_error_free (error);
    }
}

static gboolean
add_tractor (gpointer user_data)
{
  GameData *data = (GameData *) user_data;
  ClutterActor *tractor;
  int xpos;
  int num_skins;

  tractor = clutter_md2_new ();
  clutter_md2_set_data (CLUTTER_MD2 (tractor), data->tractor_data);
  clutter_actor_set_rotation (tractor, CLUTTER_Z_AXIS, 180.0,
			      data->tractor_size / 2,
			      data->tractor_size / 2,
			      0);

  clutter_actor_set_size (tractor, data->tractor_size, data->tractor_size);

  xpos = rand () % data->road_width + data->road_left;
  clutter_actor_set_position (tractor, xpos, data->road_start
			      - data->tractor_size);
  clutter_container_add (CLUTTER_CONTAINER (data->group), tractor, NULL);

  num_skins = clutter_md2_get_n_skins (CLUTTER_MD2 (tractor));
  clutter_md2_set_current_skin (CLUTTER_MD2 (tractor),
				rand () % num_skins);

  clutter_effect_move (data->eft, tractor, xpos, data->road_end,
		       (ClutterEffectCompleteFunc) clutter_actor_destroy,
		       tractor);

  /* Start another tractor some time later */
  g_timeout_add_seconds (rand () % (data->add_rate - TRACTOR_RATE_MIN + 1)
			 + TRACTOR_RATE_MIN, add_tractor, user_data);
  /* Increase the rate for the next tractor */
  if (data->add_rate > TRACTOR_RATE_MIN)
    data->add_rate--;

  /* Increase the player's score */
  td_number_set_value (TD_NUMBER (data->number), ++data->score);

  return FALSE;
}

int
main (int argc, char **argv)
{
  ClutterActor *stage, *group, *road, *car, *number_layout;
  static const ClutterColor grass_color = { 0x10, 0xa0, 0x00, 0xff };
  static const ClutterColor road_color = { 0x60, 0x60, 0x60, 0xff };
  int stage_width, stage_height;
  ClutterTimeline *line_tl, *car_rotate_tl;
  ClutterMD2Data *car_md2_data;
  int car_size, road_length;
  GameData game_data;

  clutter_init (&argc, &argv);

  stage = clutter_stage_get_default ();

  if (getenv ("FULLSCREEN"))
    clutter_stage_fullscreen (CLUTTER_STAGE (stage));

  stage_width = clutter_actor_get_width (stage);
  stage_height = clutter_actor_get_height (stage);
  road_length = stage_height * 3;

  group = clutter_group_new ();
  clutter_container_add (CLUTTER_CONTAINER (stage), group, NULL);
  clutter_actor_set_rotation (group, CLUTTER_X_AXIS, 45, 0, stage_height, 0);

  clutter_stage_set_color (CLUTTER_STAGE (stage), &grass_color);

  road = clutter_rectangle_new_with_color (&road_color);
  clutter_actor_set_position (road, stage_width / 8,
			      stage_height - road_length);
  clutter_actor_set_size (road, stage_width * 3 / 4, road_length);
  
  clutter_container_add (CLUTTER_CONTAINER (group), road, NULL);

  line_tl = make_line_animation (group, stage_width, stage_height - road_length,
				 stage_height);

  clutter_timeline_set_loop (line_tl, TRUE);
  clutter_timeline_start (line_tl);

  game_data.tractor_data = get_data ("data/tractor/tractor.md2");
  add_skin (game_data.tractor_data, "data/tractor/tractor_red.png");

  game_data.eft
    = clutter_effect_template_new_for_duration (10000, CLUTTER_ALPHA_SINE_INC);

  game_data.tractor_size = stage_width * 3 / 16;
  game_data.road_width = clutter_actor_get_width (road);
  game_data.road_left = clutter_actor_get_x (road);
  game_data.road_start = stage_height - road_length;
  game_data.road_end = stage_height;
  game_data.group = group;
  game_data.add_rate = TRACTOR_RATE_START;

  car_md2_data = get_data ("data/car/car.md2");
  car = clutter_md2_new ();
  clutter_md2_set_data (CLUTTER_MD2 (car), car_md2_data);
  g_object_unref (car_md2_data);

  car_size = game_data.tractor_size * 3 / 4;
  clutter_actor_set_position (car, stage_width / 2 - car_size / 2,
			      stage_height - car_size * 4 / 3);
  clutter_actor_set_size (car, car_size, car_size);
  clutter_container_add (CLUTTER_CONTAINER (group), car, NULL);

  game_data.car = car;
  game_data.angle = 0;
  game_data.rotate_direction = 0;
  game_data.position = stage_width / 2.0f;
  game_data.stage_width = stage_width;

  number_layout = td_corner_layout_new ();
  clutter_actor_set_size (number_layout, stage_width, stage_height);

  game_data.number = td_number_new ();
  td_number_set_value (TD_NUMBER (game_data.number), game_data.score = 0);

  clutter_container_add (CLUTTER_CONTAINER (number_layout),
			 game_data.number, NULL);

  clutter_container_add (CLUTTER_CONTAINER (stage), number_layout, NULL);

  add_tractor (&game_data);

  g_signal_connect (stage, "key-press-event",
		    G_CALLBACK (on_key_press), &game_data);
  g_signal_connect (stage, "key-release-event",
		    G_CALLBACK (on_key_release), &game_data);
  car_rotate_tl = clutter_timeline_new_for_duration (1000);
  clutter_timeline_set_loop (car_rotate_tl, TRUE);
  clutter_timeline_start (car_rotate_tl);
  g_signal_connect (car_rotate_tl, "new-frame",
		    G_CALLBACK (on_car_rotate_frame), &game_data);

  clutter_actor_show (stage);

  clutter_main ();

  /* Remove all add tractor sources */
  while (g_source_remove_by_user_data (&game_data));

  g_object_unref (car_rotate_tl);
  g_object_unref (game_data.eft);
  g_object_unref (game_data.tractor_data);
  g_object_unref (line_tl);

  return 0;
}

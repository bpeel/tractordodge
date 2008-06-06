#include <clutter/clutter.h>
#include <clutter-md2/clutter-md2.h>
#include <stdlib.h>

#define LINE_WIDTH  15
#define LINE_HEIGHT 30
#define LINE_GAP    20

typedef struct _LineCallbackData LineCallbackData;

struct _LineCallbackData
{
  int y_offset, road_start, road_end;
  ClutterActor *line;
};

typedef struct _AddTractorData AddTractorData;

struct _AddTractorData
{
  ClutterEffectTemplate *eft;
  ClutterActor *group;
  ClutterMD2Data *tractor_data;
  int tractor_size, road_width, road_left;
  int road_start, road_end;
};

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
  AddTractorData *data = (AddTractorData *) user_data;
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
  g_timeout_add_seconds (rand () % 9 + 1, add_tractor, user_data);

  return FALSE;
}

static gboolean
on_motion (ClutterActor *stage, ClutterButtonEvent *event, ClutterActor *car)
{
  clutter_actor_set_x (car, event->x - clutter_actor_get_width (car) / 2);

  return FALSE;
}

int
main (int argc, char **argv)
{
  ClutterActor *stage, *group, *road, *car;
  static const ClutterColor grass_color = { 0x10, 0xa0, 0x00, 0xff };
  static const ClutterColor road_color = { 0x60, 0x60, 0x60, 0xff };
  int stage_width, stage_height;
  ClutterTimeline *line_tl;
  AddTractorData tractor_data;
  ClutterMD2Data *car_data;
  int car_size, road_length;

  clutter_init (&argc, &argv);

  stage = clutter_stage_get_default ();

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

  tractor_data.tractor_data = get_data ("data/tractor/tractor.md2");
  add_skin (tractor_data.tractor_data, "data/tractor/tractor_red.pcx");

  tractor_data.eft
    = clutter_effect_template_new_for_duration (10000, CLUTTER_ALPHA_SINE_INC);

  tractor_data.tractor_size = stage_width * 3 / 16;
  tractor_data.road_width = clutter_actor_get_width (road);
  tractor_data.road_left = clutter_actor_get_x (road);
  tractor_data.road_start = stage_height - road_length;
  tractor_data.road_end = stage_height;
  tractor_data.group = group;

  add_tractor (&tractor_data);

  car_data = get_data ("data/car/car.md2");
  car = clutter_md2_new ();
  clutter_md2_set_data (CLUTTER_MD2 (car), car_data);
  g_object_unref (car_data);

  car_size = tractor_data.tractor_size * 3 / 4;
  clutter_actor_set_position (car, stage_width / 2 - car_size / 2,
			      stage_height - car_size * 4 / 3);
  clutter_actor_set_size (car, car_size, car_size);
  clutter_container_add (CLUTTER_CONTAINER (group), car, NULL);

  g_signal_connect (stage, "motion-event", G_CALLBACK (on_motion), car);

  clutter_actor_show (stage);

  clutter_main ();

  /* Remove all add tractor sources */
  while (g_source_remove_by_user_data (&tractor_data));

  g_object_unref (tractor_data.eft);
  g_object_unref (tractor_data.tractor_data);
  g_object_unref (line_tl);

  return 0;
}

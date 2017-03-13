#include <app_preference.h>
#include <device/led.h>
#include <private.h>

typedef struct appdata {
   Evas_Object *win;
   Evas_Object *img;
	Ecore_Animator *ea;
	Evas_Coord cx;
   Evas_Coord cy;
} appdata_s;

static int _x, _y;
static unsigned int _timestamp;

static char *
app_res_path_get(const char *res_name)
{
   char *res_path, *path;
   int pathlen;

   res_path = app_get_resource_path();
   if (!res_path)
      {
         dlog_print(DLOG_ERROR, LOG_TAG, "res_path_get ERROR!");
         return NULL;
      }

   pathlen = strlen(res_name) + strlen(res_path) + 1;
   path = malloc(sizeof(char) * pathlen);
   snprintf(path, pathlen, "%s%s", res_path, res_name);
   free(res_path);

   return path;
}

static Eina_Bool
_drag_anim_play(void *data)
{
   appdata_s *ad = data;

   int x,y, tx, ty;
   elm_win_screen_position_get(ad->win, &x, &y);
   evas_pointer_output_xy_get(evas_object_evas_get(ad->win), &tx, &ty);
   evas_object_move(ad->win, x + tx - ad->cx, y + ty - ad->cy);

   return 1;
}

static Eina_Bool
_hide_anim_play(void *data)
{
   appdata_s *ad = data;

   int x, y, w, h;
   evas_object_geometry_get(ad->img, &x, &y, &w, &h);
   if (w < 2 || h < 2)
      {
         device_flash_set_brightness(0);
         ui_app_exit();
         return 0;
      }
   evas_object_resize(ad->img, w - 12, h - 12);
   evas_object_move(ad->img, x + 6, y + 6);

   return 1;
}

static void
_button_press_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   appdata_s *ad = data;
   Evas_Event_Mouse_Down *ev = event_info;
   _timestamp = ev->timestamp;

   if (ad->ea == NULL)  ad->ea = ecore_animator_add(_drag_anim_play, ad);
   evas_pointer_output_xy_get(evas_object_evas_get(ad->win), &ad->cx, &ad->cy);
   evas_object_geometry_get(ad->win, &_x, &_y, NULL, NULL);
}

static void
_button_unpress_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   appdata_s *ad = data;
   int x, y;
   Evas_Event_Mouse_Up *ev = event_info;

   ecore_animator_del(ad->ea);
   ad->ea = NULL;

   evas_object_geometry_get(ad->win, &x, &y, NULL, NULL);

   preference_set_int("wx", x);
   preference_set_int("wy", y);

   if (ev->timestamp - _timestamp < 300 && _x - x < 20 && _y - y < 20)
      ecore_animator_add(_hide_anim_play, ad);
}

static void
create_base_gui(appdata_s *ad)
{
   int wx = -1, wy = -1;

   preference_get_int("wx", &wx);
   preference_get_int("wy", &wy);

   if (wx < 0 || wy < 0)
      {
         wx = 300;
         wy = 300;
      }

   ad->win = elm_win_add(NULL, "HappyPuppy", ELM_WIN_NOTIFICATION);
   //efl_util_set_notification_window_level(ad->win, EFL_UTIL_NOTIFICATION_LEVEL_2);
   elm_win_alpha_set(ad->win, EINA_TRUE);
   evas_object_resize(ad->win, ELM_SCALE_SIZE(60), ELM_SCALE_SIZE(60));
   evas_object_move(ad->win, wx, wy);
   elm_win_override_set(ad->win, EINA_TRUE);

   char *path;
   Evas_Object *img = elm_image_add(ad->win);
   ad->img = img;
   path = app_res_path_get("images/officon.png");
   elm_image_file_set(img, path, NULL);
   free(path);
   evas_object_resize(ad->img, ELM_SCALE_SIZE(60), ELM_SCALE_SIZE(60));
   evas_object_event_callback_add(img, EVAS_CALLBACK_MOUSE_DOWN, _button_press_cb, ad);
   evas_object_event_callback_add(img, EVAS_CALLBACK_MOUSE_UP, _button_unpress_cb, ad);
   evas_object_show(img);

   evas_object_show(ad->win);
}

static bool
app_create(void *data)
{
	/* Hook to take necessary actions before main event loop starts
		Initialize UI resources and application's data
		If this function returns true, the main loop of application starts
		If this function returns false, the application is terminated */
	appdata_s *ad = data;

	create_base_gui(ad);

	return true;
}

static void
app_control(app_control_h app_control, void *data)
{
   dlog_print(DLOG_ERROR, LOG_TAG, "app_control");

   int max_brightness;
   device_flash_get_max_brightness(&max_brightness);
   device_flash_set_brightness(max_brightness);
   dlog_print(DLOG_ERROR, LOG_TAG, "max_brightness = %d.", max_brightness);
}

static void
app_pause(void *data)
{
}

static void
app_resume(void *data)
{
}

static void
app_terminate(void *data)
{
}

static void
ui_app_lang_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LANGUAGE_CHANGED*/

	int ret;
	char *language;

	ret = app_event_get_language(event_info, &language);
	if (ret != APP_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "app_event_get_language() failed. Err = %d.", ret);
		return;
	}

	if (language != NULL) {
		elm_language_set(language);
		free(language);
	}
}

static void
ui_app_orient_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_DEVICE_ORIENTATION_CHANGED*/
	return;
}

static void
ui_app_region_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_REGION_FORMAT_CHANGED*/
}

static void
ui_app_low_battery(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_BATTERY*/
}

static void
ui_app_low_memory(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_MEMORY*/
}

int
main(int argc, char *argv[])
{
	appdata_s ad = {0,};
	int ret = 0;

	ui_app_lifecycle_callback_s event_callback = {0,};
	app_event_handler_h handlers[5] = {NULL, };

	event_callback.create = app_create;
	event_callback.terminate = app_terminate;
	event_callback.pause = app_pause;
	event_callback.resume = app_resume;
	event_callback.app_control = app_control;

	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_BATTERY], APP_EVENT_LOW_BATTERY, ui_app_low_battery, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_MEMORY], APP_EVENT_LOW_MEMORY, ui_app_low_memory, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_DEVICE_ORIENTATION_CHANGED], APP_EVENT_DEVICE_ORIENTATION_CHANGED, ui_app_orient_changed, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED], APP_EVENT_LANGUAGE_CHANGED, ui_app_lang_changed, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED], APP_EVENT_REGION_FORMAT_CHANGED, ui_app_region_changed, &ad);

	ret = ui_app_main(argc, argv, &event_callback, &ad);
	if (ret != APP_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "app_main() is failed. err = %d", ret);
	}

	return ret;
}

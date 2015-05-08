#include <pebble.h>
#include "gbitmap_tools.h"

enum DataKeys {
	W_CKEY=1,
	W_TEMP=2,
	W_COND=3,
	W_ICON=4,
	W_CITY=5
};	

typedef struct persist {
    uint32_t w_ckey;
	int8_t w_temp;
	uint8_t w_icon;
	char w_cond[32];
	char w_city[32];
} __attribute__((__packed__)) persist;

persist settings = {
    .w_ckey = 2950159,	//Default: Berlin
	.w_temp = 127,		//no temp received jet
	.w_icon = 0,
	.w_cond = "\0",
	.w_city = "\0"
};

static Window *s_main_window;
static Layer *s_clock_layer, *s_cal_layer;
static GBitmap *s_ClockBG, *s_Numbers, *s_WeatherAll;
static GFont s_TempFont, s_CondFont;
static uint8_t s_HH, s_MM, s_SS;
static AppTimer *timer_weather;

//-----------------------------------------------------------------------------------------------------------------------
static void clock_layer_update_callback(Layer *layer, GContext* ctx) 
{
	graphics_context_set_compositing_mode(ctx, GCompOpSet);

	//Background
	GSize bg_size = gbitmap_get_bounds(s_ClockBG).size;
	graphics_draw_bitmap_in_rect(ctx, s_ClockBG, GRect(0, 0, bg_size.w, bg_size.h));

	//Hour+Minutes
	uint8_t nNr, x, y=5;
	for (int i=0; i<4; i++)
	{
		if (i == 0) { x=4; nNr = s_HH / 10; }		//Hour tens
		else if (i == 1) { x=35; nNr = s_HH % 10; }	//Hour ones
		else if (i == 2) { x=73; nNr = s_MM / 10; }	//Minute tens
		else if (i == 3) { x=104; nNr = s_MM % 10; }//Minute ones
		
		GBitmap *bmpNumber = gbitmap_create_as_sub_bitmap(s_Numbers, GRect(36*nNr, 0, 36, 50));
		graphics_draw_bitmap_in_rect(ctx, bmpNumber, GRect(x, y, 36, 50));
		gbitmap_destroy(bmpNumber);
	}
	
	//Weather, only if a valid temperature exist
	if (settings.w_temp < 127)
	{
		graphics_context_set_text_color(ctx, GColorWhite);
		graphics_draw_text(ctx, settings.w_city, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GRect(4, 60, 96, 14 + 2), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
		graphics_draw_text(ctx, settings.w_cond, s_CondFont, GRect(4, 76, 96, 10 + 2), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

		char sTemp[] = "-00.0°";
		snprintf(sTemp, sizeof(sTemp), "%d°", settings.w_temp);
		GSize szTemp = graphics_text_layout_get_content_size(sTemp, s_TempFont, GRect(0, 0, 144, 168), GTextOverflowModeFill, GTextAlignmentRight);
		graphics_draw_text(ctx, sTemp, s_TempFont, GRect(bg_size.w-4-szTemp.w, bg_size.h-19-szTemp.h/2-3, szTemp.w, szTemp.h), GTextOverflowModeFill, GTextAlignmentRight, NULL);

		GBitmap *bmpWeather = gbitmap_create_as_sub_bitmap(s_WeatherAll, GRect(60*(settings.w_icon % 13), 0, 60, 50));
		graphics_draw_bitmap_in_rect(ctx, bmpWeather, GRect(bg_size.w/2-60/2, bg_size.h-50, 60, 50));
		gbitmap_destroy(bmpWeather);
	}
}
//-----------------------------------------------------------------------------------------------------------------------
static void cal_layer_update_callback(Layer *layer, GContext* ctx) 
{
	GRect rcBG = layer_get_frame(layer);
	graphics_context_set_stroke_color(ctx, GColorWhite);
	graphics_draw_round_rect(ctx, GRect(0,0,rcBG.size.w,rcBG.size.h), 5);
}
//-----------------------------------------------------------------------------------------------------------------------
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) 
{
	s_HH = tick_time->tm_hour;
	s_MM = tick_time->tm_min;
	s_SS = tick_time->tm_sec;
	layer_mark_dirty(s_clock_layer);
}
//-----------------------------------------------------------------------------------------------------------------------
static bool update_weather() 
{
	strcpy(settings.w_cond, "updating...");
	layer_mark_dirty(s_clock_layer);

	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);

	if (iter == NULL) 
	{
		app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Iter is NULL!");
		return false;
	};

	Tuplet val_ckey = TupletInteger(W_CKEY, settings.w_ckey);
	dict_write_tuplet(iter, &val_ckey);
	dict_write_end(iter);

	app_message_outbox_send();
	app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Send message with data: ckey=%d", (int)settings.w_ckey);
	return true;
}
//-----------------------------------------------------------------------------------------------------------------------
static void timerCallback(void *data) 
{
	update_weather();
}
//-----------------------------------------------------------------------------------------------------------------------
void in_received_handler(DictionaryIterator *received, void *context) 
{
	app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Received data:");
	
	Tuple *akt_tuple = dict_read_first(received);
    while (akt_tuple)
    {
        app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "KEY %d=%s/%d", (int16_t)akt_tuple->key, akt_tuple->value->cstring, akt_tuple->value->int16);

		if (akt_tuple->key == W_CKEY)
		{
			settings.w_ckey = akt_tuple->value->int32;
			persist_write_int(W_CKEY, akt_tuple->value->int32);
		}
	
		if (akt_tuple->key == W_TEMP)
			settings.w_temp = akt_tuple->value->int16;
	
		if (akt_tuple->key == W_COND)
			strcpy(settings.w_cond, akt_tuple->value->cstring);
	
		if (akt_tuple->key == W_ICON)
			settings.w_icon = akt_tuple->value->int16;
	
		if (akt_tuple->key == W_CITY)
			strcpy(settings.w_city, akt_tuple->value->cstring);
			
		akt_tuple = dict_read_next(received);
	}
	
	//Update Weather layer
	layer_mark_dirty(s_clock_layer);
}
//-----------------------------------------------------------------------------------------------------------------------
static void main_window_load(Window *window) 
{
	s_ClockBG = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CLOCK_BG);
	s_WeatherAll = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_WEATHER_ALL);
	s_Numbers = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_NUMBERS);

	s_TempFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_SUBSET_32));
	s_CondFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_SUBSET_10));

	Layer *window_layer = window_get_root_layer(s_main_window);
	GRect bounds = layer_get_frame(window_layer);
	window_set_background_color(window, GColorBlue);

	GRect rcClock = gbitmap_get_bounds(s_ClockBG);
	s_clock_layer = layer_create(rcClock);
	layer_set_update_proc(s_clock_layer, clock_layer_update_callback);
	layer_add_child(window_layer, s_clock_layer);
	
	s_cal_layer = layer_create(GRect(0, rcClock.size.h, bounds.size.w, bounds.size.h-rcClock.size.h));
	layer_set_update_proc(s_cal_layer, cal_layer_update_callback);
	layer_add_child(window_layer, s_cal_layer);
}
//-----------------------------------------------------------------------------------------------------------------------
static void main_window_unload(Window *window) 
{
	layer_destroy(s_cal_layer);
	layer_destroy(s_clock_layer);
	
	gbitmap_destroy(s_ClockBG);
	gbitmap_destroy(s_Numbers);
	gbitmap_destroy(s_WeatherAll);
	
	fonts_unload_custom_font(s_TempFont);
	fonts_unload_custom_font(s_CondFont);
}
//-----------------------------------------------------------------------------------------------------------------------
static void init() 
{
	s_main_window = window_create();
	window_set_window_handlers(s_main_window, (WindowHandlers) {
		.load = main_window_load,
		.unload = main_window_unload,
	});
	window_stack_push(s_main_window, true);
	
	tick_timer_service_subscribe(SECOND_UNIT, (TickHandler)tick_handler);
	
	//Get a time structure so that it doesn't start blank
	time_t temp = time(NULL);
	struct tm *t = localtime(&temp);

	//Manually call the tick handler when the window is loading
	tick_handler(t, MINUTE_UNIT);

	//Configure app messages
    app_message_register_inbox_received(in_received_handler);
    const uint32_t inbound_size = 128;
    const uint32_t outbound_size = 128;
    app_message_open(inbound_size, outbound_size);
	
	timer_weather = app_timer_register(1000, timerCallback, NULL);
}
//-----------------------------------------------------------------------------------------------------------------------
static void deinit() 
{
	app_timer_cancel(timer_weather);
	tick_timer_service_unsubscribe();
	window_destroy(s_main_window);
}
//-----------------------------------------------------------------------------------------------------------------------
int main(void) 
{
	init();
	app_event_loop();
	deinit();
}
//-----------------------------------------------------------------------------------------------------------------------
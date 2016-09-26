#include <pebble.h>
#include <time.h>
	
#include "gbitmap_tools.h"
#include "netdownload.h"

enum DataKeys {
	JS_READY=0,
	W_CKEY=1,
	W_TEMP=2,
	W_COND=3,
	W_ICON=4,
	W_CITY=5,
	W_CKEY_FC=6,
	W_DATE_FC=7,
	W_TEMP_H_FC=8,
	W_TEMP_L_FC=9,
	W_COND_FC=10,
	W_ICON_FC=11,
	W_CITY_FC=12,
	C_AMPM=13,
	C_SMART=14,
	C_FIRSTWD=15,
	C_GRID=16,
	C_INVERT=17,
	C_SHOWMY=18,
	C_PREWEEKS=19,
	C_WEATHER=20,
	C_WEATHER_FC=21,
	C_UNITS=22,
	C_UPDATE=23,
	C_CITYID=24,
	C_COL_BG=25,
	C_COL_CALBG=26,
	C_COL_CALGR=27,
	C_COL_CALTX=28,
	C_COL_CALHL=29,
	C_COL_CALMY=30,
	C_VIBR_ALL=31,
	C_QUIETF=32,
	C_QUIETT=33,
	C_VIBR_HR=34,
	C_VIBR_BL=35,
	C_VIBR_BC=36,
	C_WEATHER_ASO=37,
	C_DEBUG=38,
	C_HC_MODE=39,
	C_SHOWWN=40,
	C_COL_CALWN=41,
	C_WNOFFS=42,
	FC_CKEY=99,
	FC_DATE1=100,
	FC_TEMP_H1=101,
	FC_TEMP_L1=102,
	FC_ICON1=103,
	FC_COND1=104,
	FC_DATE2=105,
	FC_TEMP_H2=106,
	FC_TEMP_L2=107,
	FC_ICON2=108,
	FC_COND2=109,
	FC_DATE3=110,
	FC_TEMP_H3=111,
	FC_TEMP_L3=112,
	FC_ICON3=113,
	FC_COND3=114,
	FC_DATE4=115,
	FC_TEMP_H4=116,
	FC_TEMP_L4=117,
	FC_ICON4=118,
	FC_COND4=119,
	FC_DATE5=120,
	FC_TEMP_H5=121,
	FC_TEMP_L5=122,
	FC_ICON5=123,
	FC_COND5=124
};	

enum StorrageKeys {
	PK_SETTINGS = 0,
	PK_WEATHER = 1
};

typedef struct {
	uint32_t w_time;
	int16_t w_temp;
	uint8_t w_icon;
	char w_cond[32];
	char w_city[32];
} __attribute__((__packed__)) Weather_Data_P;

typedef struct {
	Weather_Data_P p;
	GBitmap *w_bitmap;
	bool bPictureLoading;
	bool bWeatherUpdateRetry;
	//For forecast
	bool bFCIsShowing;
	int8_t nCurrFCIcon;
	bool bWeatherFCUpdateRetry;
} Weather_Data;

Weather_Data w_data = {
	.p = {.w_time = 0, .w_temp = 0, .w_icon = 0, .w_cond = "\0", .w_city = "\0"},
	.w_bitmap = NULL,
	.bPictureLoading = false,
	.bWeatherUpdateRetry = true,
	//For forecast
	.bFCIsShowing = false,
	.nCurrFCIcon = -1,
	.bWeatherFCUpdateRetry = true
};

typedef struct {
    uint32_t date;
	int8_t w_temp_h;
	int8_t w_temp_l;
	uint8_t w_icon;
	char w_cond[32];
	Layer *w_layer;
	GBitmap *w_bitmap;
	PropertyAnimation *s_pa_anim;
} Forecast_Data;

Forecast_Data fc_data[] = {
	{ .date = 0, .w_temp_h = 0, .w_temp_l = 0, .w_icon = 0, .w_cond = "", .w_layer = NULL, .w_bitmap = NULL, .s_pa_anim = NULL },
	{ .date = 0, .w_temp_h = 0, .w_temp_l = 0, .w_icon = 0, .w_cond = "", .w_layer = NULL, .w_bitmap = NULL, .s_pa_anim = NULL },
	{ .date = 0, .w_temp_h = 0, .w_temp_l = 0, .w_icon = 0, .w_cond = "", .w_layer = NULL, .w_bitmap = NULL, .s_pa_anim = NULL },
	{ .date = 0, .w_temp_h = 0, .w_temp_l = 0, .w_icon = 0, .w_cond = "", .w_layer = NULL, .w_bitmap = NULL, .s_pa_anim = NULL },
	{ .date = 0, .w_temp_h = 0, .w_temp_l = 0, .w_icon = 0, .w_cond = "", .w_layer = NULL, .w_bitmap = NULL, .s_pa_anim = NULL }
};

const char weekdays[4][7][3] = {
	{"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"},
	{"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"},
	{"do", "lu", "ma", "mi", "ju", "vi", "sá"},
	{"di", "lu", "ma", "me", "je", "ve", "sa"}
};

typedef struct {
	//General
	bool ampm, smart, debug, hc_mode;
	//Calendar
	bool firstwd, grid, invert, showmy, showwn;
	uint8_t preweeks;
	int8_t wnoffs;
	//Weather
	bool weather, units, weather_fc, weather_aso;
	uint8_t update;
	uint32_t cityid;
	//Colors
	char col_bg[7], col_calbg[7], col_calgr[7], col_caltx[7], col_calhl[7], col_calmy[7], col_calwn[7];
	//Vibrations
	bool vibr_all;
	uint8_t quietf, quiett, vibr_hr, vibr_bl, vibr_bc;
} __attribute__((__packed__)) Settings_Data;

Settings_Data settings = {
	.ampm = false,
	.smart = true,
	.debug = false,
	.hc_mode = false,
	.firstwd = false,	//So=true, Mo=false
	.grid = true,
	.invert = true,
	.showmy = true,
	.showwn = true,
	.wnoffs = 0,
	.preweeks = 1,
	.weather = true,
	.units = false,		//°C = false, °F = °C × 1,8 + 32
	.weather_fc = true,
	.weather_aso = true,
	.update = 60,		//minutes
    .cityid = 0,	//Default: 0 (Berlin=2950159, VS=2817220)
	.col_bg = "000000",
	.col_calbg = "000055",
	.col_calgr = "aaaaaa",
	.col_caltx = "ffffff",
	.col_calhl = "ffffff",
	.col_calmy = "ffffff",
	.col_calwn = "aaaaaa",
	.vibr_all = true,
	.quietf = 22,
	.quiett = 6,
	.vibr_hr = 0, 
	.vibr_bl = 3, 
	.vibr_bc = 0
};

static Window *s_main_window;
static Layer *s_clock_layer, *s_cal_layer;
BitmapLayer *radio_layer, *battery_layer, *weather_layer;
TextLayer* fc_location_layer;
static PropertyAnimation *s_pa_location;
static GBitmap *s_ClockBG, *s_Numbers, *s_BmpBattAkt, *s_BmpRadio, *s_StatusAll;
static GFont s_TempFont, s_CondFont;
static uint8_t s_HH, s_MM, s_SS, aktBatt, aktBattAnim, nDLRetries;
static AppTimer *timer_weather, *timer_weather_fc, *timer_batt, *timer_request, *timer_slide, *timer_slide_fix;
static char AdressBuffer[] = "http://panicman.github.io/images/weather_big00.png", sLang[] = "en";
static bool s_bCharging;

//-----------------------------------------------------------------------------------------------------------------------
uint32_t HexToInt(char* hexstring)
{
	uint32_t nRet = 0, nPow;
	uint8_t len = strlen(hexstring), rem = 0;
	if (len >= 2 && (hexstring[1] == 'x' || hexstring[1] == 'X')) rem = 2; //remove 0x
	for (uint8_t i=0; i<len-rem; i++) {
		char x = hexstring[len-i-1];
		uint8_t num = (x >= '0' && x <= '9') ? x-'0' : (x >= 'A' && x <= 'F') ? x-'A'+10 : x-'a'+10;
		if (i == 0 || num == 0) nPow = 1; 								//16^0
		else { nPow = 16; for (uint8_t j=0; j<i-1; j++) nPow *= 16; }	//16^i
		nRet += nPow*num;
	}
	return nRet;
}
//-----------------------------------------------------------------------------------------------------------------------
static void clock_layer_update_callback(Layer *layer, GContext* ctx) 
{
	GRect rcFrame = layer_get_frame(layer);
	graphics_context_set_compositing_mode(ctx, GCompOpSet);
	GSize bg_size = gbitmap_get_bounds(s_ClockBG).size;

	//Background
	if (!settings.hc_mode)
		graphics_draw_bitmap_in_rect(ctx, s_ClockBG, GRect(0, 0, bg_size.w, bg_size.h));
	else
	{
		//Manual Background Draw
		//Upper back
		graphics_context_set_stroke_color(ctx, GColorWhite);
		graphics_context_set_fill_color(ctx, GColorLightGray);
		graphics_fill_rect(ctx, GRect(-1, 5, rcFrame.size.w+2, 60), 10, GCornersTop);
		graphics_draw_round_rect(ctx, GRect(-1, 5, rcFrame.size.w+2, 60), 10);

		//Bottom back
		graphics_context_set_fill_color(ctx, GColorDarkGray);
		graphics_fill_rect(ctx, GRect(0, 20, rcFrame.size.w, 45), 6, GCornersTop);

		//Black Bottom
		graphics_context_set_stroke_color(ctx, GColorDarkGray);
		graphics_context_set_fill_color(ctx, GColorBlack);
		graphics_fill_rect(ctx, GRect(0, 58, rcFrame.size.w, rcFrame.size.h-58), 6, GCornersAll);
		graphics_draw_round_rect(ctx, GRect(0, 58, rcFrame.size.w, rcFrame.size.h-58), 6);

		//Hour Back
		graphics_context_set_stroke_color(ctx, GColorLightGray);
		graphics_context_set_fill_color(ctx, GColorWhite);
		graphics_fill_rect(ctx, GRect(6, 0, 64, 58), 5, GCornersAll);
		graphics_draw_round_rect(ctx, GRect(6, 0, 64, 58), 5);
		graphics_fill_rect(ctx, GRect(rcFrame.size.w-64-6, 0, 64, 58), 5, GCornersAll);
		graphics_draw_round_rect(ctx, GRect(rcFrame.size.w-64-6, 0, 64, 58), 5);
/*	
		//Hour Bottom shadow
		graphics_context_set_fill_color(ctx, GColorLightGray);
		graphics_fill_rect(ctx, GRect(7, 58-16, 62, 16), 5, GCornersBottom);
		graphics_fill_rect(ctx, GRect(rcFrame.size.w-62-7, 58-16, 62, 16), 5, GCornersBottom);

		//Hour middle shadow	
		graphics_fill_rect(ctx, GRect(7, 29-16, 62, 16), 5, GCornerNone);
		graphics_fill_rect(ctx, GRect(rcFrame.size.w-62-7, 29-16, 62, 16), 5, GCornerNone);
*/	
		//Middle Line
		graphics_context_set_stroke_color(ctx, GColorLightGray);
		graphics_draw_line(ctx, GPoint(7, 29), GPoint(62+7, 29));	
		graphics_draw_line(ctx, GPoint(rcFrame.size.w-62-7, 29), GPoint(rcFrame.size.w-7, 29));	
	}

	//Hour+Minutes
	uint8_t nNr, x, y=5, w=32, h=47;
	for (int i=0; i<4; i++)
	{
		if (i == 0) { x=7; nNr = (s_HH > 12 && settings.ampm ? s_HH - 12 : s_HH) / 10; }		//Hour tens
		else if (i == 1) { x=37; nNr = (s_HH > 12 && settings.ampm ? s_HH - 12 : s_HH) % 10; }	//Hour ones
		else if (i == 2) { x=75; nNr = s_MM / 10; }	//Minute tens
		else if (i == 3) { x=105; nNr = s_MM % 10; }//Minute ones
		
		GBitmap *bmpNumber = gbitmap_create_as_sub_bitmap(s_Numbers, GRect(w*nNr, 0, w, h));
		graphics_draw_bitmap_in_rect(ctx, bmpNumber, GRect(x, y, w, h));
		gbitmap_destroy(bmpNumber);
	}
	
	//AM/PM
	if (settings.ampm)
	{
		graphics_context_set_text_color(ctx, GColorWhite);
		graphics_draw_text(ctx, s_HH > 12 ? "p" : "a", s_CondFont, GRect(0, 22, 6, 10 + 2), GTextOverflowModeFill, GTextAlignmentLeft, NULL);
	}
	
	//Weather, only if a valid temperature exist
	if (settings.weather)
	{
		if (w_data.p.w_time > 0)
		{
			graphics_context_set_text_color(ctx, GColorWhite);
			graphics_draw_text(ctx, w_data.p.w_city, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GRect(4, 60, 96, 14 + 2), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
			graphics_draw_text(ctx, w_data.p.w_cond, s_CondFont, GRect(4, 76, 96, 10 + 2), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

			char sTemp[] = "-00.0°";
			snprintf(sTemp, sizeof(sTemp), "%d°", (int16_t)((double)w_data.p.w_temp * (settings.units ? 1.8 : 1) + (settings.units ? 32 : 0))); //°C or °F?
			GSize szTemp = graphics_text_layout_get_content_size(sTemp, s_TempFont, GRect(0, 0, 144, 168), GTextOverflowModeFill, GTextAlignmentRight);
			graphics_draw_text(ctx, sTemp, s_TempFont, GRect(bg_size.w-4-szTemp.w, bg_size.h-19-szTemp.h/2-5, szTemp.w, szTemp.h), GTextOverflowModeFill, GTextAlignmentRight, NULL);
		}
		else
			graphics_draw_text(ctx, "Updating...", s_CondFont, GRect(4, 76, 96, 10 + 2), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
	}
}
//-----------------------------------------------------------------------------------------------------------------------
static void cal_layer_update_callback(Layer *layer, GContext* ctx) 
{
	GRect rcFrame = layer_get_frame(layer);

	//Get a time structure
	time_t timeAkt = time(NULL)+0*86400;
	struct tm *tmAkt = localtime(&timeAkt);
	uint8_t wdAkt = tmAkt->tm_wday;
	
	int8_t cal_days = settings.showwn ? 8 : 7,	// number of columns (days of the week)
		cal_weeks = 3,							// always display 3 weeks: # previous, current, # next
		cal_width = settings.showwn ? 18 : 20,	// width of columns
		cal_height = (settings.smart || settings.showmy) ? 14 : 18,		// height of columns, depens on if we need space on the bottom
		cal_vgap = -1,							// vertical gap
		text_shift = (cal_height == 14) ? -2 : 0,
		cal_border = settings.showwn ? 1 : 2; 	// side of calendar
	
	GFont current = fonts_get_system_font(FONT_KEY_GOTHIC_14);
	GColor8 col_caltx = GColorFromHEX(HexToInt(settings.col_caltx)), col_calwn = GColorFromHEX(HexToInt(settings.col_calwn)),
			col_calhl = GColorFromHEX(HexToInt(settings.col_calhl)), col_calhl_i = GColorFromHEX(0xFFFFFF-HexToInt(settings.col_calhl));
	
	//Calculate days visible bevore
	time_t timeFirst = timeAkt - (settings.preweeks*7 + wdAkt + (settings.firstwd ? 0 : wdAkt == 0 ? 6 : -1)) * 86400;
	
	graphics_context_set_stroke_color(ctx, GColorFromHEX(HexToInt(settings.col_calgr)));
	graphics_context_set_fill_color(ctx, GColorFromHEX(HexToInt(settings.col_calbg)));
	graphics_fill_rect(ctx, GRect(0, 0, rcFrame.size.w, rcFrame.size.h), 5, GCornersTop);
	
	//All Week days
	for (uint8_t col=0; col<cal_days; col++)
		for (uint8_t row=0; row<cal_weeks; row++)
		{
			//Current Day
			char sWDay[] = "00";
			time_t timeCurr = timeFirst + (row*7 + col - (col != 0 && settings.showwn ? 1 : 0)) * 86400;
			struct tm *tmCurr = localtime(&timeCurr);
		
			strftime (sWDay, 3, col == 0 && settings.showwn ? (settings.firstwd ? "%U" : "%W") : "%d", tmCurr); //Week Number or Day
			
			//Week 00-52 -> 01-53
			if (col == 0 && settings.showwn)
			{
				snprintf(sWDay, 3, "%d", atoi(sWDay)+settings.wnoffs); //Korrektur der Wochennummer
				graphics_context_set_text_color(ctx, col_calwn);
			}
			else
				graphics_context_set_text_color(ctx, col_caltx);
		
			GRect rc = GRect(cal_width * col + cal_border, cal_height * (row+1) + cal_vgap, cal_width, cal_height);
						
			if (timeCurr == timeAkt && (!settings.showwn || col != 0))
			{
				current = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);

				if (settings.invert)
				{
					graphics_context_set_text_color(ctx, col_calhl_i);
					graphics_context_set_fill_color(ctx, col_calhl);
					graphics_fill_rect(ctx, GRect(rc.origin.x+1, rc.origin.y, rc.size.w-1, rc.size.h), 0, GCornerNone);
				}
			}

			graphics_draw_text(ctx, sWDay, current, GRect(rc.origin.x, rc.origin.y + text_shift, rc.size.w, rc.size.h), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
			
			//Weekdays and vertical lines only at first row
			if (row == settings.preweeks) 
			{
				if (!settings.showwn || col != 0)
				{
					//Test the old behaviour
					char sTmp[] = "0000";
					strftime(sTmp, sizeof(sTmp), "%a", tmCurr);
				
					//Workaround, as %a doesn't work since FW 3.4
					strftime(sWDay, 2, "%w", tmCurr);
				
					int8_t nWDay= atoi(sWDay);
					strncpy(sWDay, weekdays[strcmp(sLang, "de") == 0 ? 1 : strcmp(sLang, "es") == 0 ? 2 : strcmp(sLang, "fr") == 0 ? 3 : 0][nWDay], 3);

					//APP_LOG(APP_LOG_LEVEL_DEBUG, "Weekday, My: %s, strftime: %s", sWDay, sTmp);

					if (timeCurr == timeAkt && settings.invert && (!settings.showwn || col != 0))
						graphics_context_set_text_color(ctx, col_caltx);
				}
				else
					strncpy(sWDay, "#", 2);
				
				rc = GRect(cal_width * col + cal_border, cal_vgap + text_shift, cal_width, cal_height);
				graphics_draw_text(ctx, sWDay, current, rc, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);

				//Vertical line not on the last one
				if (col != cal_days-1 && settings.grid)
					graphics_draw_line(ctx, GPoint(cal_width * (col+1) + cal_border, cal_height + cal_vgap), GPoint(cal_width * (col+1) + cal_border, (cal_weeks+1) * cal_height + cal_vgap));

				//Extra Vertical line on the Weeks
				if (col != cal_days-1 && settings.grid && col == 0 && settings.showwn)
					graphics_draw_line(ctx, GPoint(cal_width * (col+1) + cal_border - 1, cal_height + cal_vgap), GPoint(cal_width * (col+1) + cal_border - 1, (cal_weeks+1) * cal_height + cal_vgap));
			}
			
			if (timeCurr == timeAkt && (!settings.showwn || col != 0))
				current = fonts_get_system_font(FONT_KEY_GOTHIC_14);
			
			//Horizontal line
			if (settings.grid)
				graphics_draw_line(ctx, GPoint(cal_border, cal_vgap + cal_height * (row+1)), GPoint(rcFrame.size.w - cal_border - 1, cal_height * (row+1) + cal_vgap));
			
			//At the end, last line
			if (row == cal_weeks-1 && (settings.smart || settings.showmy) && settings.grid) 
				graphics_draw_line(ctx, GPoint(cal_border, cal_vgap + cal_height * (row+2)), GPoint(rcFrame.size.w - cal_border - 1, cal_height * (row+2) + cal_vgap));
		}
	
	//Rect for the Bottom space
	GRect rcBot = GRect((settings.smart ? 10 : 0), cal_vgap + cal_height * 4 + 1, rcFrame.size.w-(settings.smart ? 20 : 0), rcFrame.size.h - (cal_vgap + cal_height * 4 + 1));
	
	//Draw Month and Year
	if (settings.showmy)
	{
		char sMonYear[32];
		tmAkt = localtime(&timeAkt);
		strftime(sMonYear, sizeof(sMonYear), settings.showwn && false ? (settings.firstwd ? "%B, W%U/%G" : "%B, %W/%G") : "%B, %G", tmAkt);
		graphics_context_set_text_color(ctx, GColorFromHEX(HexToInt(settings.col_calmy)));
		graphics_draw_text(ctx, sMonYear, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GRect(rcBot.origin.x, rcBot.origin.y - 2, rcBot.size.w, rcBot.size.h), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
	}
}
//-----------------------------------------------------------------------------------------------------------------------
static void fcx_layer_update_callback(Layer *layer, GContext* ctx) 
{
	char sTemp[32];
	GRect rcFrame = layer_get_frame(layer);
	graphics_context_set_compositing_mode(ctx, GCompOpSet);
	
	//Fill background black
	graphics_context_set_fill_color(ctx, GColorBlack);	
	graphics_fill_rect(ctx, GRect(0, 0, rcFrame.size.w, rcFrame.size.h), 0, GCornerNone);	
	
	//Get the layer number
	uint8_t nAkt = 0;
	while (layer != fc_data[nAkt].w_layer && nAkt<4)
		nAkt++;

	//Get a time structure
	time_t timeAkt = fc_data[nAkt].date;
	struct tm *tmAkt = localtime(&timeAkt);
	
	graphics_context_set_text_color(ctx, GColorWhite);
	graphics_context_set_stroke_color(ctx, GColorLightGray);
	
	//Draw top line
	graphics_draw_line(ctx, GPoint(0, 0), GPoint(rcFrame.size.w, 0));
	
	//Draw Icon
	if (fc_data[nAkt].w_bitmap)
		graphics_draw_bitmap_in_rect(ctx, fc_data[nAkt].w_bitmap, GRect(0, 0, 36, 30));

	//Darw Data only if there is something
	if (fc_data[nAkt].date != 0)
	{
		//Draw Weekday
		strftime(sTemp, sizeof(sTemp), "%A", tmAkt);
		graphics_draw_text(ctx, sTemp, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GRect(36, 0, rcFrame.size.w-36-25, 20), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

		//Draw condition
		graphics_draw_text(ctx, fc_data[nAkt].w_cond, s_CondFont, GRect(36, 20-2-1, rcFrame.size.w-36-25, 10+2), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

		//Draw Temperatures
		graphics_context_set_fill_color(ctx, GColorDarkCandyAppleRed);	
		graphics_fill_rect(ctx, GRect(rcFrame.size.w-25-1, 1, 25, 14), 3, GCornersAll);	
		snprintf(sTemp, sizeof(sTemp), "%d°", (int16_t)((double)fc_data[nAkt].w_temp_h * (settings.units ? 1.8 : 1) + (settings.units ? 32 : 0))); //°C or °F?
		graphics_draw_text(ctx, sTemp, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GRect(rcFrame.size.w-25-1, 1-2, 25, 14), GTextOverflowModeFill, GTextAlignmentCenter, NULL);

		graphics_context_set_fill_color(ctx, GColorOxfordBlue);	
		graphics_fill_rect(ctx, GRect(rcFrame.size.w-25-1, 16, 25, 14), 3, GCornersAll);	
		snprintf(sTemp, sizeof(sTemp), "%d°", (int16_t)((double)fc_data[nAkt].w_temp_l * (settings.units ? 1.8 : 1) + (settings.units ? 32 : 0))); //°C or °F?
		graphics_draw_text(ctx, sTemp, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GRect(rcFrame.size.w-25-1, 16-2, 25, 14), GTextOverflowModeFill, GTextAlignmentCenter, NULL);
	}
}
//-----------------------------------------------------------------------------------------------------------------------
void generate_vibe(uint8_t vibe_pattern_number) 
{
	//No Vbration in quiet time or if disabled
	if (!settings.vibr_all || 
		(settings.quietf != 0 && s_HH >= settings.quietf) || 
		(settings.quiett != 0 && s_HH < settings.quiett))
		return;
	
	vibes_cancel();
	switch ( vibe_pattern_number ) {
		case 0: // No Vibration
			return;
		case 1: // Single short
			vibes_short_pulse();
			break;
		case 2: // Double short
			vibes_double_pulse();
			break;
		case 3: // Triple
			vibes_enqueue_custom_pattern( (VibePattern) {
				.durations = (uint32_t []) {200, 100, 200, 100, 200},
					.num_segments = 5
			} );
		case 4: // Long
			vibes_long_pulse();
			break;
		case 5: // Subtle
			vibes_enqueue_custom_pattern( (VibePattern) {
				.durations = (uint32_t []) {50, 200, 50, 200, 50, 200, 50},
					.num_segments = 7
			} );
			break;
		case 6: // Less Subtle
			vibes_enqueue_custom_pattern( (VibePattern) {
				.durations = (uint32_t []) {100, 200, 100, 200, 100, 200, 100},
					.num_segments = 7
			} );
			break;
		case 7: // Not Subtle
			vibes_enqueue_custom_pattern( (VibePattern) {
				.durations = (uint32_t []) {500, 250, 500, 250, 500, 250, 500},
					.num_segments = 7
			} );
			break;
		case 8: // S-S-L-S-S
			vibes_enqueue_custom_pattern( (VibePattern) {
				.durations = (uint32_t []) {100, 100, 100, 100, 400, 400, 100, 100, 100},
					.num_segments = 9
			} );
			break;
		default: // No Vibration
			return;
	}
}
//-----------------------------------------------------------------------------------------------------------------------
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) 
{
	s_HH = tick_time->tm_hour;
	s_MM = tick_time->tm_min;
	s_SS = tick_time->tm_sec;
	//s_HH = s_MM = 0;
	
	layer_mark_dirty(s_clock_layer);
	
	if (s_MM == 0)
		generate_vibe(settings.vibr_hr);
		
	if (s_MM == 0 || units_changed == HOUR_UNIT)
		layer_mark_dirty(s_cal_layer);
}
//-----------------------------------------------------------------------------------------------------------------------
static bool update_weather() 
{
	strcpy(w_data.p.w_cond, "Updating...");
	layer_mark_dirty(s_clock_layer);

	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);

	if (iter == NULL) 
	{
		if (settings.debug)
			app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Iter is NULL!");
		return false;
	};

	Tuplet val_ckey = TupletInteger(W_CKEY, settings.cityid);
	dict_write_tuplet(iter, &val_ckey);
	dict_write_end(iter);

	app_message_outbox_send();
	if (settings.debug)
		app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Send message with data: w_ckey=%d", (int)settings.cityid);
	return true;
}
//-----------------------------------------------------------------------------------------------------------------------
static void timerCallbackWeather(void *data) 
{
	if (w_data.bWeatherUpdateRetry && !layer_get_hidden(bitmap_layer_get_layer(radio_layer)))
	{
		update_weather();
		timer_weather = app_timer_register(30000, timerCallbackWeather, NULL); //Try again in 30 sec
	}
	else if (settings.update != 0)
	{
		//Update was successfull, again in x min
		w_data.bWeatherUpdateRetry = true;
		timer_weather = app_timer_register(60000*settings.update, timerCallbackWeather, NULL);
	}
}
//-----------------------------------------------------------------------------------------------------------------------
static bool update_weather_forecast() 
{
	text_layer_set_text(fc_location_layer, "Updating...");

	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);

	if (iter == NULL) 
	{
		if (settings.debug)
			app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Iter is NULL!");
		return false;
	};

	Tuplet val_ckey = TupletInteger(FC_CKEY, settings.cityid);
	dict_write_tuplet(iter, &val_ckey);
	dict_write_end(iter);

	app_message_outbox_send();
	if (settings.debug)
		app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Send message with data: fc_ckey=%d", (int)settings.cityid);
	return true;
}
//-----------------------------------------------------------------------------------------------------------------------
static void timerCallbackWeatherForecast(void *data) 
{
	if (w_data.bWeatherFCUpdateRetry && !layer_get_hidden(bitmap_layer_get_layer(radio_layer)))
	{
		update_weather_forecast();
		timer_weather_fc = app_timer_register(30000, timerCallbackWeatherForecast, NULL); //Try again in 30 sec
	}
	else if (settings.update != 0)
	{
		//Update was successfull, again in x min, allways 3 times longer
		w_data.bWeatherFCUpdateRetry = true;
		timer_weather_fc = app_timer_register(60000*settings.update*3, timerCallbackWeatherForecast, NULL);
	}
}
//-----------------------------------------------------------------------------------------------------------------------
static void timerCallbackBattery(void *data) 
{
	if (s_bCharging)
	{
		int nImage = 10 - (aktBattAnim / 10);
		
		bitmap_layer_set_bitmap(battery_layer, NULL);
		gbitmap_destroy(s_BmpBattAkt);
		s_BmpBattAkt = gbitmap_create_as_sub_bitmap(s_StatusAll, GRect(10*nImage, 0, 10, 16));
		bitmap_layer_set_bitmap(battery_layer, s_BmpBattAkt);

		aktBattAnim += 10;
		if (aktBattAnim > 100)
			aktBattAnim = aktBatt;
		timer_batt = app_timer_register(1000, timerCallbackBattery, NULL);
	}
}
//-----------------------------------------------------------------------------------------------------------------------
void battery_state_service_handler(BatteryChargeState charge_state) 
{
	int nImage = 0;
	aktBatt = charge_state.charge_percent;
	
	if (charge_state.is_charging)
	{
		if (!s_bCharging)
		{
			nImage = 10;
			s_bCharging = true;
			aktBattAnim = aktBatt;
			timer_batt = app_timer_register(1000, timerCallbackBattery, NULL);
		}
	}
	else
	{
		nImage = 10 - (aktBatt / 10);
		s_bCharging = false;
	}
	
	bitmap_layer_set_bitmap(battery_layer, NULL);
	gbitmap_destroy(s_BmpBattAkt);
	s_BmpBattAkt = gbitmap_create_as_sub_bitmap(s_StatusAll, GRect(10*nImage, 0, 10, 16));
	bitmap_layer_set_bitmap(battery_layer, s_BmpBattAkt);
}
//-----------------------------------------------------------------------------------------------------------------------
void bluetooth_connection_handler(bool connected)
{
	layer_set_hidden(bitmap_layer_get_layer(radio_layer), connected != true);
	generate_vibe(connected ? settings.vibr_bc : settings.vibr_bl);
}
//-----------------------------------------------------------------------------------------------------------------------
void fix_forecast_in_out()
{
	//Location
	GRect rc = layer_get_frame(text_layer_get_layer(fc_location_layer));
	rc.origin.y = w_data.bFCIsShowing ? 0 : rc.size.h * -1;
	layer_set_frame(text_layer_get_layer(fc_location_layer), rc);
	
	//Now the five day layers
	for (uint8_t i=0; i<5; i++)
	{
		rc = layer_get_frame(fc_data[i].w_layer);
		rc.origin.x = w_data.bFCIsShowing ? 0 : rc.size.w;
		layer_set_frame(fc_data[i].w_layer, rc);
	}
}
//-----------------------------------------------------------------------------------------------------------------------
static void timerCallbackSlideFix(void *data) 
{
	fix_forecast_in_out();
}
//-----------------------------------------------------------------------------------------------------------------------
void slide_forecast_in_out()
{
	//Animate Location
	GRect rc_from = layer_get_frame(text_layer_get_layer(fc_location_layer)), rc_to = rc_from;
	rc_to.origin.y += rc_to.size.h * (rc_from.origin.y < 0 ? 1 : -1);

	w_data.bFCIsShowing = (rc_from.origin.y < 0);
	
	s_pa_location = property_animation_create_layer_frame(text_layer_get_layer(fc_location_layer), &rc_from, &rc_to);
	animation_set_curve((Animation*)s_pa_location, rc_from.origin.y < 0 ? AnimationCurveEaseOut : AnimationCurveEaseIn);
	animation_set_delay((Animation*)s_pa_location, 500);
	animation_set_duration((Animation*)s_pa_location, 1000);
	animation_schedule((Animation*)s_pa_location);
	
	//Now the five day layers
	for (uint8_t i=0; i<5; i++)
	{
		rc_from = layer_get_frame(fc_data[i].w_layer); rc_to = rc_from;
		rc_to.origin.x += rc_to.size.w * (rc_from.origin.x > 143 ? -1 : 1);

		fc_data[i].s_pa_anim = property_animation_create_layer_frame(fc_data[i].w_layer, &rc_from, &rc_to);
		animation_set_curve((Animation*)fc_data[i].s_pa_anim, rc_from.origin.x > 143 ? AnimationCurveEaseOut : AnimationCurveEaseIn);
		animation_set_delay((Animation*)fc_data[i].s_pa_anim, 1000+500*i);
		animation_set_duration((Animation*)fc_data[i].s_pa_anim, 1000);
		animation_schedule((Animation*)fc_data[i].s_pa_anim);
	}
	timer_slide_fix = app_timer_register(5000, timerCallbackSlideFix, NULL);
}
//-----------------------------------------------------------------------------------------------------------------------
static void timerCallbackSlide(void *data) 
{
	if (w_data.bFCIsShowing || settings.debug)
		slide_forecast_in_out();
	
	if (settings.debug)
		timer_slide = app_timer_register(30000, timerCallbackSlide, NULL);
}
//-----------------------------------------------------------------------------------------------------------------------
static void tap_handler(AccelAxisType axis, int32_t direction) 
{
	if (!settings.weather_fc)
		return;
	
	slide_forecast_in_out();
	if (w_data.bFCIsShowing && settings.weather_aso)
		timer_slide = app_timer_register(30000, timerCallbackSlide, NULL);
}
//-----------------------------------------------------------------------------------------------------------------------
static void update_configuration(void)
{
	if (persist_exists(PK_SETTINGS))
		persist_read_data(PK_SETTINGS, &settings, sizeof(settings));

	if (persist_exists(PK_WEATHER))
		persist_read_data(PK_WEATHER, &w_data.p, sizeof(w_data.p));

	Layer *window_layer = window_get_root_layer(s_main_window);
	window_set_background_color(s_main_window, GColorFromHEX(HexToInt(settings.col_bg)));
	
	gbitmap_destroy(s_Numbers);
	s_Numbers = gbitmap_create_with_resource(settings.hc_mode ? RESOURCE_ID_IMAGE_NUMBERS_HC : RESOURCE_ID_IMAGE_NUMBERS);

	layer_remove_from_parent(bitmap_layer_get_layer(radio_layer));
	layer_remove_from_parent(bitmap_layer_get_layer(battery_layer));
	if (settings.smart)
	{
		layer_add_child(window_layer, bitmap_layer_get_layer(radio_layer));
		layer_add_child(window_layer, bitmap_layer_get_layer(battery_layer));
	}
	
	layer_remove_from_parent(bitmap_layer_get_layer(weather_layer));
	if (settings.weather)
		layer_add_child(window_layer, bitmap_layer_get_layer(weather_layer));	

	for (uint8_t i=0; i<5; i++)
		layer_remove_from_parent(fc_data[i].w_layer);
	if (settings.weather_fc)
		for (uint8_t i=0; i<5; i++)
			layer_add_child(window_layer, fc_data[i].w_layer);
	
	//Get a time structure so that it doesn't start blank
	time_t temp = time(NULL);
	struct tm *t = localtime(&temp);
	tick_handler(t, HOUR_UNIT);

	//Set Battery state
	BatteryChargeState btchg = battery_state_service_peek();
	battery_state_service_handler(btchg);
	
	//Set Bluetooth state
	bool connected = bluetooth_connection_service_peek();
	bluetooth_connection_handler(connected);
	
	if (settings.debug)
		tap_handler(ACCEL_AXIS_X, 0);
	
	light_enable(settings.debug);
}
//-----------------------------------------------------------------------------------------------------------------------
void load_picture(uint8_t nNr, bool bBig)
{
	if (bBig)
		snprintf(AdressBuffer, sizeof(AdressBuffer), "http://panicman.github.io/images/weather_big%d.png", nNr);
	else
		snprintf(AdressBuffer, sizeof(AdressBuffer), "http://panicman.github.io/images/weather_mini%d.png",nNr);
	
	if (settings.debug)
		app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Requesting image: %s", AdressBuffer);
	nDLRetries = 10;
	netdownload_request(AdressBuffer);
}
//-----------------------------------------------------------------------------------------------------------------------
void in_received_handler(DictionaryIterator *received, void *context) 
{
	if (settings.debug)
		app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Received data:");
	bool bSaveSettings = false, bUpdateWeather = false, bLoadIcon = false, bLoadFCIcons = false;
	time_t tmAkt = time(NULL);
	
	Tuple *akt_tuple = dict_read_first(received);
    while (akt_tuple)
    {
		int intVal = atoi(akt_tuple->value->cstring);
 		if (settings.debug)
	       app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "KEY %d=%s/%d/%d", (int16_t)akt_tuple->key, akt_tuple->value->cstring, akt_tuple->value->int16, intVal);

		switch (akt_tuple->key) 
		{
		case JS_READY:
			bUpdateWeather = true;
			break;
		case W_TEMP:
			w_data.p.w_time = tmAkt;
			w_data.p.w_temp = akt_tuple->value->int16;
			w_data.bWeatherUpdateRetry = false; //Update successful, usual update wait time
			break;
		case W_COND:
			strcpy(w_data.p.w_cond, akt_tuple->value->cstring);
			break;
		case W_ICON:
			w_data.p.w_icon = akt_tuple->value->int16;
			bLoadIcon = true;
			break;
		case W_CITY:
			strcpy(w_data.p.w_city, akt_tuple->value->cstring);
			break;
		case C_AMPM:
			settings.ampm = (strcmp(akt_tuple->value->cstring, "12h") == 0);
			bSaveSettings = true; //One of the settings, so we came from the Settings Page, save them
			break;
		case C_SMART:
			settings.smart = (intVal == 1);
			break;
		case C_FIRSTWD:
			settings.firstwd = (strcmp(akt_tuple->value->cstring, "so") == 0);
			break;
		case C_GRID:
			settings.grid = (intVal == 1);
			break;
		case C_INVERT:
			settings.invert = (intVal == 1);
			break;
		case C_SHOWMY:
			settings.showmy = (intVal == 1);
			break;
		case C_SHOWWN:
			settings.showwn = (intVal == 1);
		case C_WNOFFS:
			settings.wnoffs = intVal;
			app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Week# offset: %d", settings.wnoffs);
			break;
		case C_PREWEEKS:
			settings.preweeks = intVal;
			break;
		case C_WEATHER:
			settings.weather = (intVal == 1);
			break;
		case C_UNITS:
			settings.units = (strcmp(akt_tuple->value->cstring, "f") == 0);
			break;
		case C_WEATHER_FC:
			settings.weather_fc = (intVal == 1);
			break;
		case C_WEATHER_ASO:
			settings.weather_aso = (intVal == 1);
			break;
		case C_UPDATE:
			settings.update = intVal;
			break;
		case C_CITYID:
			settings.cityid = intVal;
			break;			
		case C_COL_BG:
			strcpy(settings.col_bg, akt_tuple->value->cstring);
			break;
		case C_COL_CALBG:
			strcpy(settings.col_calbg, akt_tuple->value->cstring);
			break;
		case C_COL_CALGR:
			strcpy(settings.col_calgr, akt_tuple->value->cstring);
			break;
		case C_COL_CALTX:
			strcpy(settings.col_caltx, akt_tuple->value->cstring);
			break;
		case C_COL_CALHL:
			strcpy(settings.col_calhl, akt_tuple->value->cstring);
			break;
		case C_COL_CALMY:
			strcpy(settings.col_calmy, akt_tuple->value->cstring);
			break;
		case C_COL_CALWN:
			strcpy(settings.col_calwn, akt_tuple->value->cstring);
		case C_VIBR_ALL:
			settings.vibr_all = (intVal == 1);
			break;
		case C_QUIETF:
			settings.quietf = intVal;
			break;
		case C_QUIETT:
			settings.quiett = intVal;
			break;
		case C_VIBR_HR:
			settings.vibr_hr = intVal;
			break;
		case C_VIBR_BL:
			settings.vibr_bl = intVal;
			break;
		case C_VIBR_BC:
			settings.vibr_bc = intVal;
			break;
		case C_DEBUG:
			settings.debug = (intVal == 1);
			break;
		case C_HC_MODE:
			settings.hc_mode = (intVal == 1);
			break;
		default:
			if (akt_tuple->key >= FC_DATE1 && akt_tuple->key <= FC_COND5)
			{
				bLoadFCIcons = true;
				w_data.bWeatherFCUpdateRetry = false; //Update successful, usual update wait time
				uint8_t nCurrArrPos = (akt_tuple->key - FC_DATE1) / (FC_COND1-FC_DATE1+1), 
					nCurrValPos = (akt_tuple->key - FC_DATE1) % (FC_COND1-FC_DATE1+1);
				
				switch (nCurrValPos)
				{
				case 0: //FC_DATEx		{ .date = 0, . = 0, .w_temp_l = 0, .w_icon = 0, .w_cond = "", .w_layer = NULL, .w_bitmap = NULL },
					fc_data[nCurrArrPos].date = intVal;
					break;
				case 1: //FC_TEMP_Hx
					fc_data[nCurrArrPos].w_temp_h = akt_tuple->value->uint8;
					break;
				case 2: //FC_TEMP_Lx
					fc_data[nCurrArrPos].w_temp_l = akt_tuple->value->uint8;
					break;
				case 3: //FC_ICONx
					fc_data[nCurrArrPos].w_icon = akt_tuple->value->uint8;
					break;
				case 4: //FC_CONDx
					strcpy(fc_data[nCurrArrPos].w_cond, akt_tuple->value->cstring);
					break;
				}
			}
			break;
		}
			
		akt_tuple = dict_read_next(received);
	}
	
	//Save Configuration
	if (bSaveSettings) 
	{
		int result = persist_write_data(PK_SETTINGS, &settings, sizeof(settings) );
		if (settings.debug)
			app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Wrote %d bytes into settings", result);
		
		//Force weather to update
		w_data.p.w_time = 0;
		bitmap_layer_set_bitmap(weather_layer, NULL);
		persist_delete(PK_WEATHER);
		bUpdateWeather = true;
		update_configuration();
	}
	
	//Update Weather
	if (bUpdateWeather && settings.weather)
	{
		w_data.bWeatherUpdateRetry = true;
		if (settings.debug)
			app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Saved Time: %d, diff from now: %d sec", (int)w_data.p.w_time, (int)(tmAkt - w_data.p.w_time));
		
		if (w_data.p.w_time == 0 || (tmAkt - w_data.p.w_time) > 60*settings.update)
			timer_weather = app_timer_register(100, timerCallbackWeather, NULL);
		else
		{
			bLoadIcon = true;
			timer_weather = app_timer_register((60*settings.update-(tmAkt-w_data.p.w_time))*1000, timerCallbackWeather, NULL);
		}
	}
	
	if (bUpdateWeather && settings.weather_fc)
	{
		w_data.bWeatherFCUpdateRetry = true;
		timer_weather_fc = app_timer_register(10000, timerCallbackWeatherForecast, NULL);
	}

	//Load new weather icon
	if (bLoadIcon)
	{
		//Got weather data, save them
		int result = persist_write_data(PK_WEATHER, &w_data.p, sizeof(w_data.p) );
		if (settings.debug)
			app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Wrote %d bytes into weather data", result);
		
		bitmap_layer_set_bitmap(weather_layer, NULL);
		w_data.bPictureLoading = true;
		load_picture(w_data.p.w_icon, true);
	}
	
	//Load weather forecast icons
	if (bLoadFCIcons)
	{
		w_data.nCurrFCIcon = 0;
		text_layer_set_text(fc_location_layer, w_data.p.w_city);
		if (!w_data.bPictureLoading)
			load_picture(fc_data[w_data.nCurrFCIcon].w_icon, false);
	}
	
}
//-----------------------------------------------------------------------------------------------------------------------
void download_complete_handler(NetDownload *download) 
{
	if (settings.debug)
		app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Loaded image with %u bytes, Heap free is %lu bytes", (unsigned int)download->length, (long unsigned int)heap_bytes_free());
	
	//Create Bitmap from png data
	GBitmap *bmp = gbitmap_create_from_png_data(download->data, download->length);
	
	GRect rcBmp = gbitmap_get_bounds(bmp);
	if (settings.debug)
		app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Created Image with Dimensions: %dx%d", rcBmp.size.w, rcBmp.size.h);

	//Main Picture?
	if (w_data.bPictureLoading) 
	{
		w_data.bPictureLoading = false;
		
		// free current bitmap
		bitmap_layer_set_bitmap(weather_layer, NULL);
		gbitmap_destroy(w_data.w_bitmap);
		w_data.w_bitmap = bmp;
		bitmap_layer_set_bitmap(weather_layer, w_data.w_bitmap);

		//if there is a queue for small images, load it
		if (w_data.nCurrFCIcon != -1)
			load_picture(fc_data[w_data.nCurrFCIcon].w_icon, false);
	} 
	else if (w_data.nCurrFCIcon != -1) 
	{
		// free current bitmap
		gbitmap_destroy(fc_data[w_data.nCurrFCIcon].w_bitmap);
		fc_data[w_data.nCurrFCIcon].w_bitmap = bmp;
		layer_mark_dirty(fc_data[w_data.nCurrFCIcon].w_layer);
		
		//Load the next one
		if (w_data.nCurrFCIcon++ < 4) 
			load_picture(fc_data[w_data.nCurrFCIcon].w_icon, false);
		else
			w_data.nCurrFCIcon = -1;
	} else //belongs to nothing
		gbitmap_destroy(bmp);

	// Free the memory now
	free(download->data);
	download->data = NULL;
	netdownload_destroy(download);
}
//-----------------------------------------------------------------------------------------------------------------------
static void timerCallbackRequest(void *data) 
{
	netdownload_request(AdressBuffer);
}
//-----------------------------------------------------------------------------------------------------------------------
void download_error_handler(DictionaryIterator *iter, AppMessageResult reason, void *context)
{
	if (settings.debug)
		app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "custom error handler,  %s, retries left: %d", netdownload_translate_error(reason), nDLRetries);
	
	//Request again in 3sec
	if (reason == APP_MSG_SEND_TIMEOUT && nDLRetries > 0)
	{
		nDLRetries--;
		timer_request = app_timer_register(3000, timerCallbackRequest, NULL);
	}
}
//-----------------------------------------------------------------------------------------------------------------------
static void main_window_load(Window *window) 
{
	if (settings.debug)	app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Initial Heap: used: %lu, free: %lu bytes", (long unsigned int)heap_bytes_used(), (long unsigned int)heap_bytes_free());
	s_ClockBG = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CLOCK_BG);
	if (settings.debug)	app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Glock BG loaded, Heap: used: %lu, free: %lu bytes", (long unsigned int)heap_bytes_used(), (long unsigned int)heap_bytes_free());
	s_Numbers = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_NUMBERS);
	if (settings.debug)	app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Numbers loaded, Heap: used: %lu, free: %lu bytes", (long unsigned int)heap_bytes_used(), (long unsigned int)heap_bytes_free());
	s_StatusAll = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SMART_STATUS);
	if (settings.debug)	app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Smart status loaded, Heap: used: %lu, free: %lu bytes", (long unsigned int)heap_bytes_used(), (long unsigned int)heap_bytes_free());
	s_BmpRadio = gbitmap_create_as_sub_bitmap(s_StatusAll, GRect(110, 0, 10, 16));
	
	s_TempFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_SUBSET_32));
	s_CondFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_SUBSET_10));
	if (settings.debug)	app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Fonts loaded, Heap: used: %lu, free: %lu bytes", (long unsigned int)heap_bytes_used(), (long unsigned int)heap_bytes_free());

	Layer *window_layer = window_get_root_layer(s_main_window);
	GRect bounds = layer_get_frame(window_layer);

	GRect rcClock = gbitmap_get_bounds(s_ClockBG);
	s_clock_layer = layer_create(rcClock);
	layer_set_update_proc(s_clock_layer, clock_layer_update_callback);
	layer_add_child(window_layer, s_clock_layer);
	
	s_cal_layer = layer_create(GRect(0, rcClock.size.h, bounds.size.w, bounds.size.h-rcClock.size.h));
	layer_set_update_proc(s_cal_layer, cal_layer_update_callback);
	layer_add_child(window_layer, s_cal_layer);

	//Init bluetooth radio
	radio_layer = bitmap_layer_create(GRect(1, bounds.size.h-16, 10, 16));
	bitmap_layer_set_background_color(radio_layer, GColorClear);
	bitmap_layer_set_compositing_mode(radio_layer, GCompOpSet);
	bitmap_layer_set_bitmap(radio_layer, s_BmpRadio);
		
	//Init battery
	battery_layer = bitmap_layer_create(GRect(bounds.size.w-11, bounds.size.h-16, 10, 16)); 
	bitmap_layer_set_background_color(battery_layer, GColorClear);
	bitmap_layer_set_compositing_mode(battery_layer, GCompOpSet);
	
	//Init Weather
	weather_layer = bitmap_layer_create(GRect(rcClock.size.w/2-60/2, rcClock.size.h-50-5, 60, 50)); 
	bitmap_layer_set_background_color(weather_layer, GColorClear);
	bitmap_layer_set_compositing_mode(weather_layer, GCompOpSet);
	
	//Init Forecast Layer
	fc_location_layer = text_layer_create(GRect(0, -18, bounds.size.w, 18));
	text_layer_set_text_alignment(fc_location_layer, GTextAlignmentCenter);
	text_layer_set_font(fc_location_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
	text_layer_set_background_color(fc_location_layer, GColorBlack);
	text_layer_set_text_color(fc_location_layer, GColorWhite);
	text_layer_set_text(fc_location_layer, "Updating..."); //Initial allways updating
	layer_add_child(window_layer, text_layer_get_layer(fc_location_layer));
	
	for (uint8_t i=0; i<5; i++) {
		fc_data[i].w_layer = layer_create(GRect(bounds.size.w, 18+30*i, bounds.size.w, 30));
		layer_set_update_proc(fc_data[i].w_layer, fcx_layer_update_callback);
	}
}
//-----------------------------------------------------------------------------------------------------------------------
static void main_window_appear(Window *window) 
{
	fix_forecast_in_out();
	if (w_data.bFCIsShowing && settings.weather_aso)
		timer_slide = app_timer_register(30000, timerCallbackSlide, NULL);
}
//-----------------------------------------------------------------------------------------------------------------------
static void main_window_unload(Window *window) 
{
	layer_destroy(s_cal_layer);
	layer_destroy(s_clock_layer);
	bitmap_layer_destroy(weather_layer);
	bitmap_layer_destroy(battery_layer);
	bitmap_layer_destroy(radio_layer);
	text_layer_destroy(fc_location_layer);
	
	for (uint8_t i=0; i<5; i++) {
		layer_destroy(fc_data[i].w_layer);
		gbitmap_destroy(fc_data[i].w_bitmap);
	}	
	
	gbitmap_destroy(s_ClockBG);
	gbitmap_destroy(s_Numbers);
	gbitmap_destroy(s_BmpRadio);
	gbitmap_destroy(s_BmpBattAkt);
	gbitmap_destroy(s_StatusAll);
	gbitmap_destroy(w_data.w_bitmap);
	
	fonts_unload_custom_font(s_TempFont);
	fonts_unload_custom_font(s_CondFont);
}
//-----------------------------------------------------------------------------------------------------------------------
static void init() 
{
	//Initialize dynamic weather image load
	netdownload_initialize(download_complete_handler, in_received_handler, download_error_handler);
	
	//Initialize local language
	char* sLocale = setlocale(LC_TIME, "");
	if (strncmp(sLocale, "en", 2) == 0)
		strcpy(sLang, "en");
	else if (strncmp(sLocale, "de", 2) == 0)
		strcpy(sLang, "de");
	else if (strncmp(sLocale, "es", 2) == 0)
		strcpy(sLang, "es");
	else if (strncmp(sLocale, "fr", 2) == 0)
		strcpy(sLang, "fr");
	
	app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "Time locale is set to: %s/%s", sLocale, sLang);

	//Create main window
	s_main_window = window_create();
	window_set_window_handlers(s_main_window, (WindowHandlers) {
		.load = main_window_load,
		.unload = main_window_unload,
		.appear = main_window_appear,
	});
	window_stack_push(s_main_window, true);
	
	//Subscribe ticks
	tick_timer_service_subscribe(MINUTE_UNIT, (TickHandler)tick_handler);

	//Subscribe smart status
	battery_state_service_subscribe(&battery_state_service_handler);
	bluetooth_connection_service_subscribe(&bluetooth_connection_handler);
	
	//Subscribe taps
	accel_tap_service_subscribe(tap_handler);
	
	//Initialize configuration
	update_configuration();
}
//-----------------------------------------------------------------------------------------------------------------------
static void deinit() 
{
	netdownload_deinitialize(); // call this to avoid 20B memory leak
	animation_unschedule_all();
	app_timer_cancel(timer_weather_fc);
	app_timer_cancel(timer_weather);
	app_timer_cancel(timer_batt);
	app_timer_cancel(timer_request);
	app_timer_cancel(timer_slide);
	app_timer_cancel(timer_slide_fix);
	tick_timer_service_unsubscribe();
	accel_tap_service_unsubscribe();
	battery_state_service_unsubscribe();
	bluetooth_connection_service_unsubscribe();
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
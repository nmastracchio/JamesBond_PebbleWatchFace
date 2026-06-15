#include <pebble.h>

#define textColor PBL_IF_COLOR_ELSE(GColorMintGreen, GColorWhite)

#define KEY_TEMPERATURE 0
#define KEY_WEATHER 1
#define KEY_ISCELS 2
#define KEY_ISAUTHENTIC 3
#define KEY_LAST_BATTERY_LEVEL 4
#define KEY_LAST_BATTERY_LEVEL_DATE 5
#define KEY_HEALTH 6

static Window *window;

static TextLayer *timeLayer, *watchLazer, *battery, *qWatch, *date, *weather;
static GFont timeFont, smallFont, medFont;
static bool use_custom_fonts;

static Layer *health, *armour, *background, *batteryBar;

static int batteryLevel;

  Tuple *temp_tuple;
  Tuple *conditions_tuple;
  Tuple *cels_tuple;
  Tuple *authentic_tuple;


// Store incoming information
static char cels_buffer[8];
static int authentic_buffer;
static char conditions_buffer[32];
static char weather_layer_buffer[32];

static char version[25] = "Q WATCH V4.1\n";

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  Tuple *isConfig = dict_find(iterator, 2);

  // Read tuples for data
  if(isConfig) {
    cels_tuple = dict_find(iterator, KEY_ISCELS);
    authentic_tuple = dict_find(iterator, KEY_ISAUTHENTIC);
    persist_write_int(KEY_ISCELS, (int)cels_tuple->value->int32);
    persist_write_int(KEY_ISAUTHENTIC, (int)authentic_tuple->value->int32);
    printf("saved celsius %d", (int)cels_tuple->value->int32);
  }  else{
    temp_tuple = dict_find(iterator, KEY_TEMPERATURE);
    conditions_tuple = dict_find(iterator, KEY_WEATHER);
    snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", conditions_tuple->value->cstring);
  }

  // If all data is available, use it
  int isCels = (int)persist_read_int(KEY_ISCELS);
  printf("read celsius %d", isCels);
  int isAuthentic = (int)persist_read_int(KEY_ISAUTHENTIC);
  if(isCels) {
    snprintf(cels_buffer, sizeof(cels_buffer), "%d", isCels);
  }

    authentic_buffer = (int)authentic_tuple->value->int32;
    text_layer_set_text(watchLazer, isAuthentic ? "WATCH  \nLASER  " : "\n BATTERY");
    static char LazerLevel[5];
    if(isAuthentic){
       snprintf(LazerLevel, sizeof(LazerLevel), "%d0", batteryLevel);
    }else{
      snprintf(LazerLevel, sizeof(LazerLevel), "%d%%", batteryLevel);
    }
    text_layer_set_text(battery, LazerLevel);


  if(temp_tuple && conditions_tuple){
    int temp = isCels ? (int)temp_tuple->value->int32 : (int)temp_tuple->value->int32 * 1.8 + 32;
    snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s\n%d%s", conditions_buffer, temp, isCels ? "C" : "F");
    text_layer_set_text(weather, weather_layer_buffer);
  }

}


static void drawBatteryBar(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  int width = (bounds.size.w - 18)/5;

  for (int16_t i = 0; i < 5; i++) {
    graphics_context_set_fill_color(ctx, batteryLevel > i * 20 ? textColor : PBL_IF_COLOR_ELSE(GColorIslamicGreen, GColorBlack));
    graphics_fill_rect(ctx, GRect(3 + i * (width + 3), 0, width/2, bounds.size.h), 0, GCornerNone);
    graphics_context_set_fill_color(ctx, batteryLevel > ((i * 20)+10)  ? textColor : PBL_IF_COLOR_ELSE(GColorIslamicGreen, GColorBlack));
    graphics_fill_rect(ctx, GRect(3 + i * (width + 3)+ width/2, 0, width/2, bounds.size.h), 0, GCornerNone);
  }
}

// Segment y-positions and heights are scaled proportionally from a 167px reference height.
// The original watchface was designed for a 168px screen (health layer height = 167).
// SY() scales a y-offset; SH() scales a height (adds 1px to avoid rounding to 0 on small values).
#define SY(orig, h) ((int16_t)((orig) * (h) / 167))
#define SH(orig, h) ((int16_t)((orig) * (h) / 167 + 1))

static void drawHealth(Layer *layer, GContext *ctx) {
  int curHealth = persist_read_int(KEY_HEALTH);
  printf("health ================================== %d", curHealth);
  GRect bounds = layer_get_bounds(layer);
  int16_t w = bounds.size.w;
  int16_t h = bounds.size.h;

  if(curHealth > 98){
    graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorBulgarianRose,GColorWhite));
    graphics_fill_rect(ctx, GRect(1, SY(7,h), w, SH(21,h)), 0, GCornerNone);
  }else{
    graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorBulgarianRose,GColorWhite));
    graphics_draw_rect(ctx, GRect(1, SY(7,h), w-1, SH(21,h)));
    if(curHealth > 91.5){
      graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorBulgarianRose,GColorWhite));
      graphics_fill_rect(ctx, GRect(1, SY(17,h), w-1, SH(10,h)), 0, GCornerNone);
    }
  }

  if(curHealth > 83.2){
    graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorDarkCandyAppleRed,GColorWhite));
    graphics_fill_rect(ctx, GRect(1, SY(35,h), w, SH(21,h)), 0, GCornerNone);
  }else{
    graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorDarkCandyAppleRed,GColorWhite));
    graphics_draw_rect(ctx, GRect(1, SY(35,h), w-1, SH(21,h)));
    if(curHealth > 74.9){
      graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorDarkCandyAppleRed,GColorWhite));
      graphics_fill_rect(ctx, GRect(1, SY(45,h), w-1, SH(10,h)), 0, GCornerNone);
    }
  }

  if(curHealth > 66.6){
    graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorRed,GColorWhite));
    graphics_fill_rect(ctx, GRect(1, SY(63,h), w, SH(21,h)), 0, GCornerNone);
  }else{
    graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorRed,GColorWhite));
    graphics_draw_rect(ctx, GRect(1, SY(63,h), w-1, SH(21,h)));
    if(curHealth > 58.3){
      graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorRed,GColorWhite));
      graphics_fill_rect(ctx, GRect(1, SY(73,h), w-1, SH(10,h)), 0, GCornerNone);
    }
  }

  if(curHealth >= 50){
    graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorOrange,GColorWhite));
    graphics_fill_rect(ctx, GRect(1, SY(91,h), w, SH(8,h)), 0, GCornerNone);
  }else{
    graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorOrange,GColorWhite));
    graphics_draw_rect(ctx, GRect(1, SY(91,h), w-1, SH(8,h)));
  }
  if(curHealth >= 40){
    graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorChromeYellow,GColorWhite));
    graphics_fill_rect(ctx, GRect(1, SY(106,h), w, SH(8,h)), 0, GCornerNone);
  }else{
    graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorChromeYellow,GColorWhite));
    graphics_draw_rect(ctx, GRect(1, SY(106,h), w-1, SH(8,h)));
  }
  if(curHealth >= 30){
    graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorYellow,GColorWhite));
    graphics_fill_rect(ctx, GRect(1, SY(121,h), w, SH(8,h)), 0, GCornerNone);
  }else{
    graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorYellow,GColorWhite));
    graphics_draw_rect(ctx, GRect(1, SY(121,h), w-1, SH(8,h)));
  }
  if(curHealth >= 20){
    graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorIcterine,GColorWhite));
    graphics_fill_rect(ctx, GRect(1, SY(136,h), w, SH(8,h)), 0, GCornerNone);
  }else{
    graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorIcterine,GColorWhite));
    graphics_draw_rect(ctx, GRect(1, SY(136,h), w-1, SH(8,h)));
  }
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorPastelYellow,GColorWhite));
  graphics_fill_rect(ctx, GRect(1, SY(151,h), w, SH(8,h)), 0, GCornerNone);
}

static void battery_handler(BatteryChargeState new_state) {
  static char LazerLevel[5];
  batteryLevel = (int)new_state.charge_percent;
  if(authentic_buffer){
    snprintf(LazerLevel, sizeof(LazerLevel), "%d0", batteryLevel);
  }else{
    snprintf(LazerLevel, sizeof(LazerLevel), "%d%%", batteryLevel);
  }
  int lastLevel = (int)persist_read_int(KEY_LAST_BATTERY_LEVEL);
  if(batteryLevel != lastLevel){
    int lastTime = persist_read_int(KEY_LAST_BATTERY_LEVEL_DATE);
    int timeNow = (int)time(NULL);
    int timeDif = timeNow - lastTime;
    WatchInfoModel watch = watch_info_get_model();
    int lifeSpan = 7 * 86400;
    if(watch == WATCH_INFO_MODEL_PEBBLE_TIME_STEEL){
       lifeSpan = 10 * 86400;
    }
    if(batteryLevel == 100 || !lastTime || batteryLevel > lastLevel){
      persist_write_int(KEY_HEALTH, 100);
    }else if(batteryLevel == 90){
      persist_write_int(KEY_HEALTH, (int)((timeDif * 100) / (lifeSpan  / 20)));
    }else{
      persist_write_int(KEY_HEALTH, (int)((timeDif * 100) / (lifeSpan / 10)));
    }
    persist_write_int(KEY_LAST_BATTERY_LEVEL_DATE, timeNow);
    persist_write_int(KEY_LAST_BATTERY_LEVEL, batteryLevel);
  }

  layer_set_update_proc(health, drawHealth);
  text_layer_set_text(battery, LazerLevel);
  GRect bgBounds = layer_get_bounds(background);
  batteryBar = layer_create(GRect(0, bgBounds.size.h - 9, bgBounds.size.w, 6));
  layer_set_update_proc(batteryBar, drawBatteryBar);
}

static void update_time() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
  text_layer_set_text(timeLayer, s_buffer);

  static char date_buffer[16];
  strftime(date_buffer, sizeof(date_buffer), "%a, %b %d", tick_time);
  text_layer_set_text(date, date_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  if(tick_time->tm_min % 30 == 0) {
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    dict_write_uint8(iter, 0, 0);
    app_message_outbox_send();
  }
}


static void drawArmour(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  int16_t w = bounds.size.w;
  int16_t h = bounds.size.h;
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorOxfordBlue, GColorWhite));
  graphics_fill_rect(ctx, GRect(1, SY(7,h), w, SH(21,h)), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorDukeBlue, GColorWhite));
  graphics_fill_rect(ctx, GRect(1, SY(35,h), w, SH(21,h)), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorBlue, GColorWhite));
  graphics_fill_rect(ctx, GRect(1, SY(63,h), w, SH(21,h)), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorBlueMoon, GColorWhite));
  graphics_fill_rect(ctx, GRect(1, SY(91,h), w, SH(8,h)), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorVividCerulean, GColorWhite));
  graphics_fill_rect(ctx, GRect(1, SY(106,h), w, SH(8,h)), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorPictonBlue, GColorWhite));
  graphics_fill_rect(ctx, GRect(1, SY(121,h), w, SH(8,h)), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorElectricBlue, GColorWhite));
  graphics_fill_rect(ctx, GRect(1, SY(136,h), w, SH(8,h)), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorCeleste, GColorWhite));
  graphics_fill_rect(ctx, GRect(1, SY(151,h), w, SH(8,h)), 0, GCornerNone);
}

static void drawArmourEmpty(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  int16_t w = bounds.size.w - 1;
  int16_t h = bounds.size.h;
  graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorOxfordBlue, GColorWhite));
  graphics_draw_rect(ctx, GRect(1, SY(7,h), w, SH(21,h)));
  graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorDukeBlue, GColorWhite));
  graphics_draw_rect(ctx, GRect(1, SY(35,h), w, SH(21,h)));
  graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorBlue, GColorWhite));
  graphics_draw_rect(ctx, GRect(1, SY(63,h), w, SH(21,h)));
  graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorBlueMoon, GColorWhite));
  graphics_draw_rect(ctx, GRect(1, SY(91,h), w, SH(8,h)));
  graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorVividCerulean, GColorWhite));
  graphics_draw_rect(ctx, GRect(1, SY(106,h), w, SH(8,h)));
  graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorPictonBlue, GColorWhite));
  graphics_draw_rect(ctx, GRect(1, SY(121,h), w, SH(8,h)));
  graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorElectricBlue, GColorWhite));
  graphics_draw_rect(ctx, GRect(1, SY(136,h), w, SH(8,h)));
  graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorCeleste, GColorWhite));
  graphics_draw_rect(ctx, GRect(1, SY(151,h), w, SH(8,h)));
}

static void drawBackground(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorDarkGreen, GColorBlack));
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
}


static void bt_handler(bool connected) {
  if (connected) {
    layer_set_update_proc(armour, drawArmour);
  } else {
    layer_set_update_proc(armour, drawArmourEmpty);
  }
}

static void windowLoad(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect screen_bounds = layer_get_bounds(window_layer);
  int16_t sw = screen_bounds.size.w;
  int16_t sh = screen_bounds.size.h;

  // Center panel sits between 16px side bars, with 6px gutters on each side
  // Basalt reference: GRect(23, 4, 98, 160) for 144×168 screen
  background = layer_create(GRect(23, 4, sw - 46, sh - 8));
  layer_set_update_proc(background, drawBackground);
  GRect bgBounds = layer_get_bounds(background);
  int16_t bw = bgBounds.size.w;
  int16_t bh = bgBounds.size.h;

  // On emery (200×228) the custom fonts sized for 144×168 are too small;
  // use system fonts that are ~1.4× larger instead.
#ifdef PBL_PLATFORM_EMERY
  timeFont  = fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD);
  smallFont = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  medFont   = fonts_get_system_font(FONT_KEY_GOTHIC_18);
  use_custom_fonts = false;
#else
  timeFont  = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_CLOCK_29));
  smallFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_SMALL_9));
  medFont   = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_MED_12));
  use_custom_fonts = true;
#endif

  // All text-layer y-positions are scaled proportionally from the 160px reference
  // background height (basalt), so the layout adapts to the taller emery panel.
  int16_t time_y   = bh * 55 / 160;
  int16_t time_h   = bh * 50 / 160;
  int16_t date_y   = bh * 82 / 160;
  int16_t date_h   = bh * 22 / 160 + 10;
  int16_t lazer_y  = bh * 130 / 160;
  int16_t batt_y   = bh * 139 / 160;

  timeLayer = text_layer_create(GRect(0, time_y, bw, time_h));
  text_layer_set_font(timeLayer, timeFont);
  text_layer_set_background_color(timeLayer, GColorClear);
  text_layer_set_text_color(timeLayer, textColor);
  text_layer_set_text_alignment(timeLayer, GTextAlignmentCenter);

  date = text_layer_create(GRect(0, date_y, bw - 1, date_h));
  text_layer_set_font(date, medFont);
  text_layer_set_text_alignment(date, GTextAlignmentRight);
  text_layer_set_background_color(date, GColorClear);
  text_layer_set_text_color(date, textColor);

  qWatch = text_layer_create(GRect(0, 0, bw, 20));
  text_layer_set_font(qWatch, smallFont);
  text_layer_set_text(qWatch, version);
  text_layer_set_background_color(qWatch, GColorClear);
  text_layer_set_text_color(qWatch, textColor);
  text_layer_set_text_alignment(qWatch, GTextAlignmentCenter);

  weather = text_layer_create(GRect(0, 15, bw - 3, 50));
  text_layer_set_font(weather, medFont);
  text_layer_set_text(weather, "fetching from mi6..");
  text_layer_set_background_color(weather, GColorClear);
  text_layer_set_text_color(weather, textColor);
  text_layer_set_text_alignment(weather, GTextAlignmentRight);

  watchLazer = text_layer_create(GRect(0, lazer_y, 50, 50));
  text_layer_set_font(watchLazer, smallFont);
  text_layer_set_background_color(watchLazer, GColorClear);
  text_layer_set_text_color(watchLazer, textColor);
  text_layer_set_text_alignment(watchLazer, GTextAlignmentCenter);

  battery = text_layer_create(GRect(45, batt_y, 50, 20));
  text_layer_set_font(battery, smallFont);
  text_layer_set_text_alignment(battery, GTextAlignmentRight);
  text_layer_set_background_color(battery, GColorClear);
  text_layer_set_text_color(battery, textColor);

  // Side bars span full screen height; right bar anchors to screen right edge
  health = layer_create(GRect(1, 1, 16, sh - 2));
  armour = layer_create(GRect(sw - 18, 1, 16, sh - 2));
  bt_handler(connection_service_peek_pebble_app_connection());
  battery_handler(battery_state_service_peek());

  layer_add_child(window_layer, background);
  layer_add_child(window_layer, health);
  layer_add_child(window_layer, armour);
  layer_add_child(background, text_layer_get_layer(qWatch));
  layer_add_child(background, text_layer_get_layer(weather));
  layer_add_child(background, text_layer_get_layer(watchLazer));
  layer_add_child(background, text_layer_get_layer(battery));
  layer_add_child(background, text_layer_get_layer(date));
  layer_add_child(background, batteryBar);
  layer_add_child(background, text_layer_get_layer(timeLayer));
}

static void windowUnload(Window *window) {
  battery_state_service_unsubscribe();
  connection_service_unsubscribe();
  text_layer_destroy(timeLayer);
  text_layer_destroy(qWatch);
  text_layer_destroy(weather);
  text_layer_destroy(watchLazer);
  text_layer_destroy(battery);
  layer_destroy(batteryBar);
  layer_destroy(background);
  layer_destroy(armour);
  layer_destroy(health);
  if (use_custom_fonts) {
    fonts_unload_custom_font(timeFont);
    fonts_unload_custom_font(smallFont);
    fonts_unload_custom_font(medFont);
  }
}

static void init() {
  window = window_create();

  window_set_window_handlers(window, (WindowHandlers) {
    .load = windowLoad,
    .unload = windowUnload
  });

  window_stack_push(window, true);

  window_set_background_color(window, GColorBlack);

  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bt_handler
  });

  battery_state_service_subscribe(battery_handler);

  update_time();

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void deinit() {
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

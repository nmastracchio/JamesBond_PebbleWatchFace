#include <pebble.h>

#define textColor PBL_IF_COLOR_ELSE(GColorMintGreen, GColorWhite)

#define KEY_TEMPERATURE 0
#define KEY_WEATHER 1
#define KEY_ISCELS 2
#define KEY_ISAUTHENTIC 3

static Window *window;

static TextLayer *timeLayer, *watchLazer, *battery, *qWatch, *date, *weather;
static GFont timeFont, smallFont, medFont;

static Layer *health, *armour, *background, *batteryBar;

static int batteryLevel;
static int stepCount;

  Tuple *temp_tuple;
  Tuple *conditions_tuple;
  Tuple *cels_tuple;
  Tuple *authentic_tuple;

static char cels_buffer[8];
static char conditions_buffer[32];
static char weather_layer_buffer[32];

static char version[25] = "Q WATCH V4.2\n";

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

  if(isConfig) {
    cels_tuple = dict_find(iterator, KEY_ISCELS);
    authentic_tuple = dict_find(iterator, KEY_ISAUTHENTIC);
    persist_write_int(KEY_ISCELS, (int)cels_tuple->value->int32);
    persist_write_int(KEY_ISAUTHENTIC, (int)authentic_tuple->value->int32);
  } else {
    temp_tuple = dict_find(iterator, KEY_TEMPERATURE);
    conditions_tuple = dict_find(iterator, KEY_WEATHER);
    snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", conditions_tuple->value->cstring);
  }

  int isCels = (int)persist_read_int(KEY_ISCELS);
  if(isCels) {
    snprintf(cels_buffer, sizeof(cels_buffer), "%d", isCels);
  }

  if(temp_tuple && conditions_tuple){
    int temp = isCels ? (int)temp_tuple->value->int32 : (int)temp_tuple->value->int32 * 1.8 + 32;
    snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s\n%d%s", conditions_buffer, temp, isCels ? "C" : "F");
    text_layer_set_text(weather, weather_layer_buffer);
  }
}


// 10 segments × 2,500 steps = 25,000 step goal
static void drawStepsBar(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  int width = (bounds.size.w - 18) / 5;

  for (int16_t i = 0; i < 5; i++) {
    graphics_context_set_fill_color(ctx, stepCount > i * 5000 ? textColor : PBL_IF_COLOR_ELSE(GColorIslamicGreen, GColorBlack));
    graphics_fill_rect(ctx, GRect(3 + i * (width + 3), 0, width / 2, bounds.size.h), 0, GCornerNone);
    graphics_context_set_fill_color(ctx, stepCount > i * 5000 + 2500 ? textColor : PBL_IF_COLOR_ELSE(GColorIslamicGreen, GColorBlack));
    graphics_fill_rect(ctx, GRect(3 + i * (width + 3) + width / 2, 0, width / 2, bounds.size.h), 0, GCornerNone);
  }
}

// Compute segment layout from layer height h.
// Top half (0..h/2): 3 large blocks with 2px gaps.
// Bottom half (h/2..h): 5 small blocks with 2px gaps.
// 8 segments (3 large + 5 small), uniform 4px gap between every pair.
// large = 2×small; solving 3*large + 5*small + 7*gap = h gives small = (h - 28) / 11.
static void seg_layout(int16_t h,
                       int16_t *large_h, int16_t *small_h,
                       int16_t b[8]) {
  const int16_t g = 4;
  *small_h = (h - 7 * g) / 11;
  *large_h = 2 * (*small_h);
  b[0] = 0;
  b[1] = *large_h + g;
  b[2] = 2 * (*large_h + g);
  b[3] = 3 * (*large_h + g);
  b[4] = b[3] + *small_h + g;
  b[5] = b[3] + 2 * (*small_h + g);
  b[6] = b[3] + 3 * (*small_h + g);
  b[7] = b[3] + 4 * (*small_h + g);
}

static void drawHealth(Layer *layer, GContext *ctx) {
  int cur = batteryLevel;
  GRect bounds = layer_get_bounds(layer);
  int16_t w = bounds.size.w;
  int16_t h = bounds.size.h;

  int16_t lh, sh2, b[8];
  seg_layout(h, &lh, &sh2, b);
  int16_t half_h = lh - lh / 2;

  // Block 0 — top large (Bulgarian Rose)
  if(cur > 98){
    graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorBulgarianRose, GColorWhite));
    graphics_fill_rect(ctx, GRect(1, b[0], w, lh), 0, GCornerNone);
  } else {
    graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorBulgarianRose, GColorWhite));
    graphics_draw_rect(ctx, GRect(1, b[0], w-1, lh));
    if(cur > 91.5){
      graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorBulgarianRose, GColorWhite));
      graphics_fill_rect(ctx, GRect(1, b[0] + lh/2, w-1, half_h), 0, GCornerNone);
    }
  }

  // Block 1 — Dark Candy Apple Red
  if(cur > 83.2){
    graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorDarkCandyAppleRed, GColorWhite));
    graphics_fill_rect(ctx, GRect(1, b[1], w, lh), 0, GCornerNone);
  } else {
    graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorDarkCandyAppleRed, GColorWhite));
    graphics_draw_rect(ctx, GRect(1, b[1], w-1, lh));
    if(cur > 74.9){
      graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorDarkCandyAppleRed, GColorWhite));
      graphics_fill_rect(ctx, GRect(1, b[1] + lh/2, w-1, half_h), 0, GCornerNone);
    }
  }

  // Block 2 — Red
  if(cur > 66.6){
    graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorRed, GColorWhite));
    graphics_fill_rect(ctx, GRect(1, b[2], w, lh), 0, GCornerNone);
  } else {
    graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorRed, GColorWhite));
    graphics_draw_rect(ctx, GRect(1, b[2], w-1, lh));
    if(cur > 58.3){
      graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorRed, GColorWhite));
      graphics_fill_rect(ctx, GRect(1, b[2] + lh/2, w-1, half_h), 0, GCornerNone);
    }
  }

  // Blocks 3–7 — small segments (Orange → Pastel Yellow)
  if(cur >= 50){
    graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorOrange, GColorWhite));
    graphics_fill_rect(ctx, GRect(1, b[3], w, sh2), 0, GCornerNone);
  } else {
    graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorOrange, GColorWhite));
    graphics_draw_rect(ctx, GRect(1, b[3], w-1, sh2));
  }
  if(cur >= 40){
    graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorChromeYellow, GColorWhite));
    graphics_fill_rect(ctx, GRect(1, b[4], w, sh2), 0, GCornerNone);
  } else {
    graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorChromeYellow, GColorWhite));
    graphics_draw_rect(ctx, GRect(1, b[4], w-1, sh2));
  }
  if(cur >= 30){
    graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorYellow, GColorWhite));
    graphics_fill_rect(ctx, GRect(1, b[5], w, sh2), 0, GCornerNone);
  } else {
    graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorYellow, GColorWhite));
    graphics_draw_rect(ctx, GRect(1, b[5], w-1, sh2));
  }
  if(cur >= 20){
    graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorIcterine, GColorWhite));
    graphics_fill_rect(ctx, GRect(1, b[6], w, sh2), 0, GCornerNone);
  } else {
    graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorIcterine, GColorWhite));
    graphics_draw_rect(ctx, GRect(1, b[6], w-1, sh2));
  }
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorPastelYellow, GColorWhite));
  graphics_fill_rect(ctx, GRect(1, b[7], w, sh2), 0, GCornerNone);
}

static void battery_handler(BatteryChargeState new_state) {
  batteryLevel = (int)new_state.charge_percent;
  layer_set_update_proc(health, drawHealth);
  GRect bgBounds = layer_get_bounds(background);
  batteryBar = layer_create(GRect(0, bgBounds.size.h - 9, bgBounds.size.w, 6));
  layer_set_update_proc(batteryBar, drawStepsBar);
}

static void health_handler(HealthEventType event, void *context) {
  if(event == HealthEventSignificantUpdate || event == HealthEventMovementUpdate) {
    stepCount = (int)health_service_sum_today(HealthMetricStepCount);
    static char steps_buf[8];
    snprintf(steps_buf, sizeof(steps_buf), "%d", stepCount);
    text_layer_set_text(battery, steps_buf);
    if(batteryBar) layer_mark_dirty(batteryBar);
  }
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
  int16_t lh, sh2, b[8];
  seg_layout(h, &lh, &sh2, b);

  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorOxfordBlue, GColorWhite));
  graphics_fill_rect(ctx, GRect(1, b[0], w, lh), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorDukeBlue, GColorWhite));
  graphics_fill_rect(ctx, GRect(1, b[1], w, lh), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorBlue, GColorWhite));
  graphics_fill_rect(ctx, GRect(1, b[2], w, lh), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorBlueMoon, GColorWhite));
  graphics_fill_rect(ctx, GRect(1, b[3], w, sh2), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorVividCerulean, GColorWhite));
  graphics_fill_rect(ctx, GRect(1, b[4], w, sh2), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorPictonBlue, GColorWhite));
  graphics_fill_rect(ctx, GRect(1, b[5], w, sh2), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorElectricBlue, GColorWhite));
  graphics_fill_rect(ctx, GRect(1, b[6], w, sh2), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorCeleste, GColorWhite));
  graphics_fill_rect(ctx, GRect(1, b[7], w, sh2), 0, GCornerNone);
}

static void drawArmourEmpty(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  int16_t w = bounds.size.w - 1;
  int16_t h = bounds.size.h;
  int16_t lh, sh2, b[8];
  seg_layout(h, &lh, &sh2, b);

  graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorOxfordBlue, GColorWhite));
  graphics_draw_rect(ctx, GRect(1, b[0], w, lh));
  graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorDukeBlue, GColorWhite));
  graphics_draw_rect(ctx, GRect(1, b[1], w, lh));
  graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorBlue, GColorWhite));
  graphics_draw_rect(ctx, GRect(1, b[2], w, lh));
  graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorBlueMoon, GColorWhite));
  graphics_draw_rect(ctx, GRect(1, b[3], w, sh2));
  graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorVividCerulean, GColorWhite));
  graphics_draw_rect(ctx, GRect(1, b[4], w, sh2));
  graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorPictonBlue, GColorWhite));
  graphics_draw_rect(ctx, GRect(1, b[5], w, sh2));
  graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorElectricBlue, GColorWhite));
  graphics_draw_rect(ctx, GRect(1, b[6], w, sh2));
  graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorCeleste, GColorWhite));
  graphics_draw_rect(ctx, GRect(1, b[7], w, sh2));
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

  background = layer_create(GRect(23, 4, sw - 46, sh - 8));
  layer_set_update_proc(background, drawBackground);
  GRect bgBounds = layer_get_bounds(background);
  int16_t bw = bgBounds.size.w;
  int16_t bh = bgBounds.size.h;

#if defined(PBL_PLATFORM_EMERY)
  timeFont  = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_CLOCK_38));
  smallFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_SMALL_12));
  medFont   = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_MED_16));
#elif defined(PBL_PLATFORM_BASALT)
  timeFont  = fonts_get_system_font(FONT_KEY_LECO_32_BOLD_NUMBERS);
  smallFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_SMALL_9));
  medFont   = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_MED_12));
#else  // aplite
  timeFont  = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_CLOCK_29));
  smallFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_SMALL_9));
  medFont   = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_MED_12));
#endif

  int16_t time_y  = bh * 55 / 160;
  int16_t time_h  = bh * 50 / 160;
  int16_t date_y  = bh * 82 / 160;
  int16_t date_h  = bh * 22 / 160 + 10;
  int16_t label_h = 18;
  int16_t lazer_y = bh - 9 - label_h;

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
  text_layer_set_text(weather, "FETCHING...");
  text_layer_set_background_color(weather, GColorClear);
  text_layer_set_text_color(weather, textColor);
  text_layer_set_text_alignment(weather, GTextAlignmentRight);

  watchLazer = text_layer_create(GRect(0, lazer_y, bw, label_h));
  text_layer_set_font(watchLazer, smallFont);
  text_layer_set_text(watchLazer, "WATCH LASER");
  text_layer_set_background_color(watchLazer, GColorClear);
  text_layer_set_text_color(watchLazer, textColor);
  text_layer_set_text_alignment(watchLazer, GTextAlignmentLeft);

  battery = text_layer_create(GRect(0, lazer_y, bw, label_h));
  text_layer_set_font(battery, smallFont);
  text_layer_set_text_alignment(battery, GTextAlignmentRight);
  text_layer_set_background_color(battery, GColorClear);
  text_layer_set_text_color(battery, textColor);

  // Initial step count
  stepCount = (int)health_service_sum_today(HealthMetricStepCount);
  static char steps_buf[8];
  snprintf(steps_buf, sizeof(steps_buf), "%d", stepCount);
  text_layer_set_text(battery, steps_buf);

  // Side bars flush with the green background panel
  health = layer_create(GRect(1,     4, 16, sh - 8));
  armour = layer_create(GRect(sw-18, 4, 16, sh - 8));
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
  health_service_events_unsubscribe();
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
#if defined(PBL_PLATFORM_BASALT)
  fonts_unload_custom_font(smallFont);
  fonts_unload_custom_font(medFont);
#else
  fonts_unload_custom_font(timeFont);
  fonts_unload_custom_font(smallFont);
  fonts_unload_custom_font(medFont);
#endif
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
  health_service_events_subscribe(health_handler, NULL);

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

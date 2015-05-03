#include "main_window.h"

static Window *s_main_window;
static TextLayer *s_weekday_layer, *s_day_in_month_layer, *s_month_layer;
static Layer *s_canvas_layer, *s_bg_layer;

static Time s_last_time, s_anim_time;
static char s_weekday_buffer[8], s_month_buffer[8], s_day_in_month_buffer[3];
static bool s_animating, s_connected;

static void tick_handler(struct tm *tick_time, TimeUnits changed) {
  s_last_time.days = tick_time->tm_mday;
  s_last_time.hours = tick_time->tm_hour;
  s_last_time.minutes = tick_time->tm_min;
  s_last_time.seconds = tick_time->tm_sec;

  s_last_time.hours -= (s_last_time.hours > 12) ? 12 : 0;

  snprintf(s_day_in_month_buffer, sizeof(s_day_in_month_buffer), "%d", s_last_time.days);
  strftime(s_weekday_buffer, sizeof(s_weekday_buffer), "%a", tick_time);
  strftime(s_month_buffer, sizeof(s_month_buffer), "%b", tick_time);

  text_layer_set_text(s_weekday_layer, s_weekday_buffer);
  text_layer_set_text(s_day_in_month_layer, s_day_in_month_buffer);
  text_layer_set_text(s_month_layer, s_month_buffer);

  // Finally
  layer_mark_dirty(s_canvas_layer);
}

/****************************** AnimationImplementation ***********************/

static void animation_started(Animation *anim, void *context) {
  // s_animating = true;
}

static void animation_stopped(Animation *anim, bool stopped, void *context) {
  // s_animating = false;

  // if(config_get(PERSIST_KEY_SECOND_HAND)) {
  //   tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  // } else {
  //   tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  // }  

#ifdef PBL_PLATFORM_APLITE
  // animation_destroy(anim);
#endif
}

static void animate(int duration, int delay, AnimationImplementation *implementation, bool handlers) {
  Animation *anim = animation_create();
  if(anim) {
    animation_set_duration(anim, duration);
    animation_set_delay(anim, delay);
    animation_set_curve(anim, AnimationCurveEaseInOut);
    animation_set_implementation(anim, implementation);
    if(handlers) {
      animation_set_handlers(anim, (AnimationHandlers) {
        .started = animation_started,
        .stopped = animation_stopped
      }, NULL);
    }
    animation_schedule(anim);
  }
}

/****************************** Drawing Functions *****************************/

static void bg_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);

#ifdef PBL_PLATFORM_BASALT
  graphics_context_set_antialiased(ctx, ANTIALIASING);
#endif

  BatteryChargeState state = battery_state_service_peek();
  bool plugged = state.is_plugged;
  int perc = state.charge_percent;
  int batt_hours = (int)(12.0F * ((float)perc / 100.0F)) + 1;

  for(int h = 0; h < 12; h++) {
    for(int y = 0; y < THICKNESS; y++) {
      for(int x = 0; x < THICKNESS; x++) {
        GPoint point = (GPoint) {
          .x = (int16_t)(sin_lookup(TRIG_MAX_ANGLE * h / 12) * (int32_t)(3 * HAND_LENGTH_SEC) / TRIG_MAX_RATIO) + center.x,
          .y = (int16_t)(-cos_lookup(TRIG_MAX_ANGLE * h / 12) * (int32_t)(3 * HAND_LENGTH_SEC) / TRIG_MAX_RATIO) + center.y,
        };

#ifdef PBL_COLOR
        if(config_get(PERSIST_KEY_BATTERY)) {
          if(h < batt_hours) {
            if(plugged) {
              // Charging
              graphics_context_set_stroke_color(ctx, GColorGreen);
            } else {
              // Discharging at this level
              graphics_context_set_stroke_color(ctx, GColorWhite);
            }
          } else {
            // Empty segment
            graphics_context_set_stroke_color(ctx, GColorDarkGray);
          }
        } else {
          // No battery indicator
          graphics_context_set_stroke_color(ctx, GColorWhite);
        }
#else
        if(config_get(PERSIST_KEY_BATTERY)) {
          if(h < batt_hours) {
            // Discharging at this level
            graphics_context_set_stroke_color(ctx, GColorWhite);
          } else {
            // Empty segment
            graphics_context_set_stroke_color(ctx, GColorBlack);
          }
        } else {
          // No battery indicator
          graphics_context_set_stroke_color(ctx, GColorWhite);
        }
#endif
        graphics_draw_line(ctx, GPoint(center.x + x, center.y + y), GPoint(point.x + x, point.y + y));
      }
    }
  }

  // Make markers
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(MARGIN, MARGIN, bounds.size.w - (2 * MARGIN), bounds.size.h - (2 * MARGIN)), 0, GCornerNone);
}

static GPoint make_hand_point(int quantity, int intervals, int len, GPoint center) {
  return (GPoint) {
    .x = (int16_t)(sin_lookup(TRIG_MAX_ANGLE * quantity / intervals) * (int32_t)len / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(TRIG_MAX_ANGLE * quantity / intervals) * (int32_t)len / TRIG_MAX_RATIO) + center.y,
  };
}

static int hours_to_minutes(int hours_out_of_12) {
  return (int)(float)(((float)hours_out_of_12 / 12.0F) * 60.0F);
}

static void draw_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);  

#ifdef PBL_PLATFORM_BASALT
  graphics_context_set_antialiased(ctx, ANTIALIASING);
#endif

  Time mode_time = (s_animating) ? s_anim_time : s_last_time;

  // Plot hand ends
  GPoint second_hand_long = make_hand_point(mode_time.seconds, 60, HAND_LENGTH_SEC, center);
  GPoint minute_hand_long = make_hand_point(mode_time.minutes, 60, HAND_LENGTH_MIN, center);
  GPoint second_hand_short = make_hand_point(mode_time.seconds, 60, (HAND_LENGTH_SEC - MARGIN + 2), center);
  GPoint minute_hand_short = make_hand_point(mode_time.minutes, 60, (HAND_LENGTH_MIN - MARGIN + 2), center);

  float minute_angle = TRIG_MAX_ANGLE * mode_time.minutes / 60;
  float hour_angle;
  if(s_animating) {
    // Hours out of 60 for smoothness
    hour_angle = TRIG_MAX_ANGLE * mode_time.hours / 60;
  } else {
    hour_angle = TRIG_MAX_ANGLE * mode_time.hours / 12;
  }
  hour_angle += (minute_angle / TRIG_MAX_ANGLE) * (TRIG_MAX_ANGLE / 12);

  // Hour is more accurate
  GPoint hour_hand_long = (GPoint) {
    .x = (int16_t)(sin_lookup(hour_angle) * (int32_t)HAND_LENGTH_HOUR / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(hour_angle) * (int32_t)HAND_LENGTH_HOUR / TRIG_MAX_RATIO) + center.y,
  };
  GPoint hour_hand_short = (GPoint) {
    .x = (int16_t)(sin_lookup(hour_angle) * (int32_t)(HAND_LENGTH_HOUR - MARGIN + 2) / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(hour_angle) * (int32_t)(HAND_LENGTH_HOUR - MARGIN + 2) / TRIG_MAX_RATIO) + center.y,
  };

  // Draw hands
#ifdef PBL_COLOR
  graphics_context_set_stroke_color(ctx, GColorDarkGray);
#elif PBL_BW 
  graphics_context_set_stroke_color(ctx, GColorWhite);
#endif
  for(int y = 0; y < THICKNESS; y++) {
    for(int x = 0; x < THICKNESS; x++) {
      graphics_draw_line(ctx, GPoint(center.x + x, center.y + y), GPoint(minute_hand_short.x + x, minute_hand_short.y + y));
      graphics_draw_line(ctx, GPoint(center.x + x, center.y + y), GPoint(hour_hand_short.x + x, hour_hand_short.y + y));
    }
  }
  graphics_context_set_stroke_color(ctx, GColorWhite);
  for(int y = 0; y < THICKNESS; y++) {
    for(int x = 0; x < THICKNESS; x++) {
      graphics_draw_line(ctx, GPoint(minute_hand_short.x + x, minute_hand_short.y + y), GPoint(minute_hand_long.x + x, minute_hand_long.y + y));
      graphics_draw_line(ctx, GPoint(hour_hand_short.x + x, hour_hand_short.y + y), GPoint(hour_hand_long.x + x, hour_hand_long.y + y));
    }
  }

  // Draw second hand
  if(config_get(PERSIST_KEY_SECOND_HAND)) {
    // Use loops
    for(int y = 0; y < THICKNESS - 1; y++) {
      for(int x = 0; x < THICKNESS - 1; x++) {
#ifdef PBL_COLOR
        graphics_context_set_stroke_color(ctx, GColorDarkCandyAppleRed);
#elif PBL_BW
        graphics_context_set_stroke_color(ctx, GColorWhite);
#endif
        graphics_draw_line(ctx, GPoint(center.x + x, center.y + y), GPoint(second_hand_short.x + x, second_hand_short.y + y));

        // Draw second hand tip
#ifdef PBL_COLOR
        graphics_context_set_stroke_color(ctx, GColorChromeYellow);
#elif PBL_BW
        graphics_context_set_stroke_color(ctx, GColorWhite);
#endif
        graphics_draw_line(ctx, GPoint(second_hand_short.x + x, second_hand_short.y + y), GPoint(second_hand_long.x + x, second_hand_long.y + y));
      }
    }
  }

  // Center
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, GPoint(center.x + 1, center.y + 1), 4);

  // Draw black if disconnected
  if(config_get(PERSIST_KEY_BT) && !s_connected) {
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_circle(ctx, GPoint(center.x + 1, center.y + 1), 3);
  }
}

static void bt_handler(bool connected) {
  // Notify disconnection
  if(!connected && s_connected) {
    vibes_long_pulse();
  }

  s_connected = connected;
  layer_mark_dirty(s_canvas_layer);
}

static void batt_handler(BatteryChargeState state) {
  layer_mark_dirty(s_canvas_layer);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_bg_layer = layer_create(bounds);
  layer_set_update_proc(s_bg_layer, bg_update_proc);
  layer_add_child(window_layer, s_bg_layer);

  s_weekday_layer = text_layer_create(GRect(90, 55, 44, 40));
  text_layer_set_text_alignment(s_weekday_layer, GTextAlignmentCenter);
  text_layer_set_font(s_weekday_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_color(s_weekday_layer, GColorWhite);
  text_layer_set_background_color(s_weekday_layer, GColorClear);

  s_day_in_month_layer = text_layer_create(GRect(90, 68, 44, 40));
  text_layer_set_text_alignment(s_day_in_month_layer, GTextAlignmentCenter);
  text_layer_set_font(s_day_in_month_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
#ifdef PBL_COLOR
  text_layer_set_text_color(s_day_in_month_layer, GColorChromeYellow);
#elif PBL_BW
  text_layer_set_text_color(s_day_in_month_layer, GColorWhite);
#endif
  text_layer_set_background_color(s_day_in_month_layer, GColorClear);

  s_month_layer = text_layer_create(GRect(90, 95, 44, 40));
  text_layer_set_text_alignment(s_month_layer, GTextAlignmentCenter);
  text_layer_set_font(s_month_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_color(s_month_layer, GColorWhite);
  text_layer_set_background_color(s_month_layer, GColorClear);

  if(config_get(PERSIST_KEY_DAY)) {
    layer_add_child(window_layer, text_layer_get_layer(s_day_in_month_layer));
  }
  if(config_get(PERSIST_KEY_DATE)) {
    layer_add_child(window_layer, text_layer_get_layer(s_weekday_layer));
    layer_add_child(window_layer, text_layer_get_layer(s_month_layer));
  }

  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, draw_proc);
  layer_add_child(window_layer, s_canvas_layer);
}

static void window_unload(Window *window) {
  layer_destroy(s_canvas_layer);
  layer_destroy(s_bg_layer);

  text_layer_destroy(s_weekday_layer);
  text_layer_destroy(s_day_in_month_layer);
  text_layer_destroy(s_month_layer);

  // Self destroying
  window_destroy(s_main_window);
}

static int anim_percentage(AnimationProgress dist_normalized, int max) {
  return (int)(float)(((float)dist_normalized / (float)ANIMATION_NORMALIZED_MAX) * (float)max);
}

static void hands_update(Animation *anim, AnimationProgress dist_normalized) {
  s_anim_time.hours = anim_percentage(dist_normalized, hours_to_minutes(s_last_time.hours));
  s_anim_time.minutes = anim_percentage(dist_normalized, s_last_time.minutes);
  s_anim_time.seconds = anim_percentage(dist_normalized, s_last_time.seconds);

  layer_mark_dirty(s_canvas_layer);
}

void main_window_push() {
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_main_window, true);

  // TODO make charging pref
  battery_state_service_subscribe(batt_handler);

  time_t t = time(NULL);
  struct tm *tm_now = localtime(&t);
  s_last_time.hours = tm_now->tm_hour;
  s_last_time.minutes = tm_now->tm_min;
  s_last_time.seconds = tm_now->tm_sec;  

  if(config_get(PERSIST_KEY_BT)) {
    bluetooth_connection_service_subscribe(bt_handler);
    bt_handler(bluetooth_connection_service_peek());
  }

  // Begin smooth animation
  AnimationImplementation hands_impl = {
    .update = hands_update
  };
  animate(ANIMATION_DURATION, ANIMATION_DELAY, &hands_impl, true);
}
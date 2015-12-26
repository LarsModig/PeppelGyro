#include <pebble.h>
#include <math.h>

#define TIMER_INTERVAL_MS 50
#define PI 3.14159265359



Window *s_main_window;
static Layer *s_canvas_layer;
GRect window_bounds;


int counter = 25545;

int will_crash() {
  // CompassHeadingData data;
  // compass_service_peek(&data);
  // return data.magnetic_heading
  APP_LOG(APP_LOG_LEVEL_DEBUG, "counter = %i", counter);
  //counter = (counter + (TRIG_MAX_ANGLE / 100)) % TRIG_MAX_ANGLE;
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "returning counter = %i", counter);
  return counter;
}

int will_not_crash() {
  return 25545;  //Same as counter
}

GPoint get_compas_pointer_coords(GPoint center) {
  #define PIXEL_FROM_CENTER 70
  APP_LOG(APP_LOG_LEVEL_DEBUG, "get_compas_pointer_coords(GPoint *center)");
  
  //CHANGE THIS METHOD TO will_not_crash()
  float rads = (float) will_not_crash();
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Will crash after this depending on above method! rads: %i", (int) rads);
  
  int x = (int) (sinf(rads) * PIXEL_FROM_CENTER) + center.x;
  int y = (int) (cosf(rads) * PIXEL_FROM_CENTER) + center.y;
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Post crash x:y %i:%i", x, y);
  GPoint g;
  g.x = x;
  g.y = y;
  
  return g;
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GPoint center;
  center.x = window_bounds.size.w/2;
  center.y = window_bounds.size.h/2;
  
  GPoint outer = get_compas_pointer_coords(center);
  
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 3);
  graphics_draw_line(ctx, outer, center);
}

static void timer_callback(void *context) {
  layer_mark_dirty(s_canvas_layer);
  app_timer_register(TIMER_INTERVAL_MS, timer_callback, NULL);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  window_bounds = layer_get_bounds(window_layer);

  // Create Layer
  s_canvas_layer = layer_create(GRect(0, 0, window_bounds.size.w, window_bounds.size.h));
  layer_add_child(window_layer, s_canvas_layer);

  // Set the update_proc
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
}

static void main_window_unload(Window *window) {
  // Destroy Layer
  layer_destroy(s_canvas_layer);
}

void handle_init(void) {
  s_main_window = window_create();
  
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  
  window_stack_push(s_main_window, true);
  
  //app_timer_register(TIMER_INTERVAL_MS, timer_callback, NULL);
}

void handle_deinit(void) {
  window_destroy(s_main_window);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}

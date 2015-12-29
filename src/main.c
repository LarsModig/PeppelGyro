#include <pebble.h>
#include <math.h>
#include <pebble_fonts.h>

#define TIMER_INTERVAL_MS 50
#define PI 3.14159265359



Window *s_main_window;
static Layer *s_canvas_layer;
GRect window_bounds;

char debug1[20];
char debug2[20];
int16_t counter = 0;


int32_t rotator() {
  counter = counter + 1 % 365;
  int32_t toreturn = (TRIG_MAX_ANGLE * counter / 365);
  return toreturn;
}

int get_compas_heading() {
  CompassHeadingData data;
  compass_service_peek(&data);
  return data.magnetic_heading;
}

GPoint get_compas_pointer_coords(GPoint center) {
  #define LENGTH_OF_ARROW 70

  int angle = rotator();
  
  GPoint g;
  g.x = (sin_lookup(angle) * LENGTH_OF_ARROW / TRIG_MAX_RATIO) + center.x;
  g.y = (-cos_lookup(angle) * LENGTH_OF_ARROW / TRIG_MAX_RATIO) + center.y;
  
  snprintf(debug1, sizeof(debug1), "x: %d", (int) (sin_lookup(angle) * LENGTH_OF_ARROW / TRIG_MAX_RATIO));
  snprintf(debug2, sizeof(debug1), "y: %d", (int) (-cos_lookup(angle) * LENGTH_OF_ARROW / TRIG_MAX_RATIO));
  
  return g;
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GPoint center = GPoint(window_bounds.size.w/2, window_bounds.size.h/2);
  
  GPoint outer = get_compas_pointer_coords(center);
  
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 3);
  graphics_draw_line(ctx, outer, center);
  
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, debug1, fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(1,1, 144,20), GTextOverflowModeFill, GTextAlignmentLeft, NULL);
  graphics_draw_text(ctx, debug2, fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(1,20, 144,32), GTextOverflowModeFill, GTextAlignmentLeft, NULL);
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
  
  app_timer_register(TIMER_INTERVAL_MS, timer_callback, NULL);
}

void handle_deinit(void) {
  window_destroy(s_main_window);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}

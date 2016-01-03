#include <pebble.h>
#include <math.h>
#include <pebble_fonts.h>

#define TIMER_INTERVAL_MS 50
#define PI 3.14159265359

#define DEBUG 1



Window *s_main_window;
static Layer *s_canvas_layer;
GRect window_bounds;

char debug1[20];
char debug2[20];
int16_t counter = 0;

static int16_t xtilt = 0;
enum XTiltState {
  XTiltStateUnknown = 0,
  XTiltStateNormal = 1,
  XTiltStateDown = 2,
  XTiltStateDoubleDown = 3
};
static int xtiltstate = 0;
static AccelRawData acc_hist[100];
static int acc_hist_point = 0;
static bool acc_hist_loaded = false;

int32_t rotator() {
  counter = (counter + 1) % 365;
  int32_t toreturn = TRIG_MAX_ANGLE * counter / 365;
  return toreturn;
}

int get_compas_heading() {
  CompassHeadingData data;
  compass_service_peek(&data);
  return data.magnetic_heading;
}

GPoint get_compas_pointer_coords(GPoint center, int angle) {
  //not used, could have use later
  
  #define LENGTH_OF_ARROW 70
  
  GPoint g;
  g.x = (sin_lookup(angle) * LENGTH_OF_ARROW / TRIG_MAX_RATIO) + center.x;
  g.y = (-cos_lookup(angle) * LENGTH_OF_ARROW / TRIG_MAX_RATIO) + center.y;
    
  return g;
}

void draw_disc_pies(GContext *ctx, int trig_angle, int8_t piesize, int radius, GPoint center, GColor c1, GColor c2, bool draw_outline) {
  
  int offset = trig_angle % (TRIG_MAX_ANGLE * piesize / 360);

  int coloroffset = (trig_angle % (TRIG_MAX_ANGLE * piesize / 180)) / (TRIG_MAX_ANGLE * piesize / 360);
  
  for(int i = -1; i < (180+piesize)/piesize; i++) {
    if(i % 2) {
      graphics_context_set_fill_color(ctx, c1);
    } else {
      graphics_context_set_fill_color(ctx, c2);
    }
    graphics_fill_radial(
      ctx, 
      GRect(center.x-radius, center.y-radius, radius*2, radius*2), 
      GOvalScaleModeFitCircle, 
      radius, 
      offset + (i - coloroffset) * DEG_TO_TRIGANGLE(piesize), 
      offset + (i - coloroffset + 1) * DEG_TO_TRIGANGLE(piesize)
    );
  }
  
  if(draw_outline) {
    graphics_context_set_stroke_color(ctx, c2);
    graphics_draw_circle(ctx, center, radius);
  }
}

void draw_disc(GContext *ctx, int trig_angle) {
  GPoint center = GPoint(-40, window_bounds.size.h/2);
  GColor c1 = GColorCadetBlue;
  GColor c2 = GColorMidnightGreen;
  
  draw_disc_pies(ctx, trig_angle, 30, 100, center, c1, c2, true);  
  draw_disc_pies(ctx, trig_angle, 90, 70, center, c1, c2, false);  
}

void draw_debug_info(GContext *ctx){
  //APP_LOG(2, "xtilt: %i", xtilt);
  snprintf(debug1, sizeof(debug1), "xtilt: %d", xtilt);
  snprintf(debug2, sizeof(debug2), "state: %d", xtiltstate);
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, debug1, fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(40,1, 144,20), GTextOverflowModeFill, GTextAlignmentLeft, NULL);
  graphics_draw_text(ctx, debug2, fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(40,20, 144,32), GTextOverflowModeFill, GTextAlignmentLeft, NULL);  
}


static void canvas_update_proc(Layer *layer, GContext *ctx) {
  //int trig_angle = rotator();
  int trig_angle = get_compas_heading();
  
  draw_disc(ctx, trig_angle);
  
  if(DEBUG) {
    draw_debug_info(ctx);
  }
}

static void timer_callback(void *context) {
  layer_mark_dirty(s_canvas_layer);
  app_timer_register(TIMER_INTERVAL_MS, timer_callback, NULL);
}

void inc_acc_hist_point() {
  acc_hist_point = (acc_hist_point + 1) % 100;
  if(!acc_hist_loaded && acc_hist_point == 99) {
    acc_hist_loaded = true;
  }
}

static void data_acc_handler(AccelData *data, uint32_t num_samples) {
  for(uint8_t i = 0; i < num_samples; i++) {
    inc_acc_hist_point();
    AccelRawData ard;
    ard.x = data[i].x;
    ard.y = data[i].y;
    ard.z = data[i].z;
    acc_hist[acc_hist_point] = ard;
  }
  if(!acc_hist_loaded){
    return;
  }
  
  int xtilt80 = 0;
  int xtilt20 = 0;
  //get sum x
  for(uint8_t i = 0; i < 100; i++) {
    inc_acc_hist_point();
    if(i < 80) {
      xtilt80 += acc_hist[acc_hist_point].x;
    } else {
      xtilt20 += acc_hist[acc_hist_point].x;
    }
  }
  xtilt80 = xtilt80 / 80;
  xtilt20 = xtilt20 / 20;
  
  int xtilt = xtilt20 - xtilt80;

  if(xtilt > 400) {
    if(xtiltstate == XTiltStateNormal) {
      APP_LOG(1, "XTiltStateDoubleDown %d %d %d", xtilt, xtilt20, xtilt80);
      xtiltstate = XTiltStateDoubleDown;
    }
  } else if(xtilt > 200) {
    if(xtiltstate == XTiltStateNormal) {
      APP_LOG(1, "XTiltStateDown %d %d %d", xtilt, xtilt20, xtilt80);
      xtiltstate = XTiltStateDown;
    }
  } else {
    APP_LOG(1, "XTiltStateNormal %d %d %d", xtilt, xtilt20, xtilt80);
    xtiltstate = XTiltStateNormal;
  }
}

//register_x_degres_callback(function* limit_triggered, int degrees, int saftey_margin)

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
  
  uint32_t num_samples = 10;
  accel_data_service_subscribe(num_samples, data_acc_handler);
  accel_service_set_sampling_rate(ACCEL_SAMPLING_10HZ);
}

void handle_deinit(void) {
  window_destroy(s_main_window);
  accel_data_service_unsubscribe();
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}

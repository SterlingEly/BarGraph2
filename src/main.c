#include <pebble.h>

// ============================================================
// CONSTANTS
// ============================================================
#define SETTINGS_KEY      1
#define DATE_HEIGHT       24   // px reserved at bottom for date text
#define BAR_MARGIN        8    // px from screen edge to outer bar edge
#define BAR_WIDTH         24   // px width of each bar
#define BAR_GAP           16   // px gap between the two bars
#define TICK_LEN          5    // px length of tick marks
#define LABEL_W           28   // px width of label text column

// ============================================================
// SETTINGS
// ============================================================
typedef struct {
  bool invert;   // swap black/white
} BarSettings;

static BarSettings s_settings;

static void prv_default_settings(void) {
  s_settings.invert = false;
}

static void prv_save_settings(void) {
  persist_write_data(SETTINGS_KEY, &s_settings, sizeof(s_settings));
}

static void prv_load_settings(void) {
  prv_default_settings();
  persist_read_data(SETTINGS_KEY, &s_settings, sizeof(s_settings));
}

// ============================================================
// STATE
// ============================================================
static Window *s_window;
static Layer  *s_canvas_layer;

static int  s_hour   = 0;
static int  s_minute = 0;
static char s_date_buf[16];

// ============================================================
// HELPERS
// ============================================================

// Foreground and background colors, respecting invert setting.
// On B&W platforms InvertBW is the only meaningful option.
// On color platforms this gives a clean B&W look as a baseline.
static GColor col_fg(void) {
  return s_settings.invert ? GColorBlack : GColorWhite;
}
static GColor col_bg(void) {
  return s_settings.invert ? GColorWhite : GColorBlack;
}

// ============================================================
// DRAW
// ============================================================
static void draw_layer(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_unobstructed_bounds(layer);
  int w = bounds.size.w;
  int h = bounds.size.h;

  // ---- Background ----
  graphics_context_set_fill_color(ctx, col_bg());
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // ---- Layout ----
  // Bar area sits above the date strip.
  // Two bars side by side, centered horizontally.
  // Labels flank each bar on their outer sides.
  //
  // Horizontal layout (left→right):
  //   BAR_MARGIN | LABEL_W | TICK_LEN | BAR_WIDTH | BAR_GAP | BAR_WIDTH | TICK_LEN | LABEL_W | BAR_MARGIN
  //
  // We center the whole assembly.

  int bar_area_h = h - DATE_HEIGHT;  // vertical space for bars

  // Total width of the two-bar assembly
  int assembly_w = LABEL_W + TICK_LEN + BAR_WIDTH + BAR_GAP + BAR_WIDTH + TICK_LEN + LABEL_W;
  int asm_x = (w - assembly_w) / 2;  // left edge of assembly

  // Key x positions
  int lbl_l_x  = asm_x;                                    // left label column x
  int ltick_x  = lbl_l_x + LABEL_W;                        // left tick x
  int lbar_x   = ltick_x + TICK_LEN;                       // left bar x
  int rbar_x   = lbar_x + BAR_WIDTH + BAR_GAP;             // right bar x
  int rtick_x  = rbar_x + BAR_WIDTH;                       // right tick x
  int lbl_r_x  = rtick_x + TICK_LEN;                       // right label column x

  // Vertical: bar bottom sits just above date strip, bar top at top margin
  int bar_top    = BAR_MARGIN;
  int bar_bottom = bar_area_h - BAR_MARGIN;
  int bar_h      = bar_bottom - bar_top;   // total drawable bar height

  // ---- Hour bar ----
  // 12h display: 1–12, each hour = bar_h/12 px
  int disp_hour = s_hour % 12;
  if (disp_hour == 0) disp_hour = 12;

  int hour_px = bar_h * disp_hour / 12;

  // Empty track outline
  graphics_context_set_stroke_color(ctx, col_fg());
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_rect(ctx, GRect(lbar_x, bar_top, BAR_WIDTH, bar_h));

  // Filled portion (grows upward from bottom)
  if (hour_px > 0) {
    graphics_context_set_fill_color(ctx, col_fg());
    graphics_fill_rect(ctx, GRect(lbar_x, bar_bottom - hour_px, BAR_WIDTH, hour_px), 0, GCornerNone);
  }

  // Hour tick marks and labels (left side, one per hour, 1–12)
  graphics_context_set_text_color(ctx, col_fg());
  for (int hr = 1; hr <= 12; hr++) {
    int ty = bar_bottom - bar_h * hr / 12;  // y of this tick (top of segment boundary)

    // Tick mark on the left side of the left bar
    graphics_draw_line(ctx, GPoint(ltick_x, ty), GPoint(lbar_x - 1, ty));

    // Label
    static char lbl[4];
    snprintf(lbl, sizeof(lbl), "%02d", hr);
    graphics_draw_text(ctx, lbl,
      fonts_get_system_font(FONT_KEY_GOTHIC_14),
      GRect(lbl_l_x, ty - 8, LABEL_W, 16),
      GTextOverflowModeFill, GTextAlignmentRight, NULL);
  }

  // ---- Minute bar ----
  // 0–59 minutes: fill proportionally
  int min_px = bar_h * s_minute / 59;  // 59 so minute 59 = full bar

  // Empty track outline
  graphics_draw_rect(ctx, GRect(rbar_x, bar_top, BAR_WIDTH, bar_h));

  // Filled portion
  if (min_px > 0) {
    graphics_context_set_fill_color(ctx, col_fg());
    graphics_fill_rect(ctx, GRect(rbar_x, bar_bottom - min_px, BAR_WIDTH, min_px), 0, GCornerNone);
  }

  // Minute tick marks and labels (right side, every 5 minutes)
  for (int mn = 5; mn <= 60; mn += 5) {
    int mn_src = (mn == 60) ? 59 : mn;  // 60 label sits at top of bar
    int ty = bar_bottom - bar_h * mn_src / 59;

    // Tick mark on the right side of the right bar
    graphics_draw_line(ctx, GPoint(rbar_x + BAR_WIDTH, ty), GPoint(rtick_x + TICK_LEN - 1, ty));

    // Label
    static char lbl[4];
    snprintf(lbl, sizeof(lbl), "%d", mn);
    graphics_draw_text(ctx, lbl,
      fonts_get_system_font(FONT_KEY_GOTHIC_14),
      GRect(lbl_r_x, ty - 8, LABEL_W, 16),
      GTextOverflowModeFill, GTextAlignmentLeft, NULL);
  }

  // ---- Date strip ----
  graphics_context_set_text_color(ctx, col_fg());
  graphics_draw_text(ctx, s_date_buf,
    fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
    GRect(0, h - DATE_HEIGHT - 2, w, DATE_HEIGHT + 4),
    GTextOverflowModeFill, GTextAlignmentCenter, NULL);
}

// ============================================================
// EVENT HANDLERS
// ============================================================
static void tick_handler(struct tm *t, TimeUnits units_changed) {
  s_hour   = t->tm_hour;
  s_minute = t->tm_min;
  strftime(s_date_buf, sizeof(s_date_buf), "%a, %b %e", t);
  layer_mark_dirty(s_canvas_layer);
}

static void inbox_received(DictionaryIterator *iter, void *context) {
  Tuple *t = dict_find(iter, MESSAGE_KEY_invert);
  if (t) s_settings.invert = (t->value->int32 == 1);
  prv_save_settings();
  layer_mark_dirty(s_canvas_layer);
}

// ============================================================
// WINDOW / APP LIFECYCLE
// ============================================================
static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  s_canvas_layer = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_canvas_layer, draw_layer);
  layer_add_child(root, s_canvas_layer);
}

static void window_unload(Window *window) {
  layer_destroy(s_canvas_layer);
}

static void init(void) {
  prv_load_settings();

  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load   = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);

  // Seed time immediately
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  tick_handler(t, MINUTE_UNIT | DAY_UNIT);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  app_message_register_inbox_received(inbox_received);
  app_message_open(64, 64);
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

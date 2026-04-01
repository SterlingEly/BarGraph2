#include <pebble.h>

// ============================================================
// Bar Graph 2 — main.c
// v2.1: exact replica of v1 layout — no ring, no dim track
// Two vertical bars hugging screen edges:
//   Left  = hours  (fills bottom-up)
//   Right = minutes (fills bottom-up)
// ============================================================

#define SETTINGS_KEY 1

// Rect layout (matched to v1 pixel measurements)
#define BAR_W          30
#define LABEL_W        26
#define TICK_LEN        5
#define DATE_H         22
#define BAR_MARGIN_TOP  4
#define BAR_MARGIN_BOT  4

// Round layout
#define ROUND_BAR_W    24
#define ROUND_BAR_GAP  10
#define ROUND_TICK_M   28
#define ROUND_DATE_H   18
#define ROUND_DATE_GAP  4

static int prv_isqrt(int n) {
  if (n <= 0) return 0;
  int x = n, y = (x + 1) / 2;
  while (y < x) { x = y; y = (x + n / x) / 2; }
  return x;
}

// ============================================================
// SETTINGS
// ============================================================
typedef struct {
  GColor BackgroundColor;
  GColor BarLitColor;
  GColor BarDimColor;
  GColor DateTextColor;
  GColor RingBattColor;
  GColor RingStepsColor;
  GColor RingDimColor;
  int    StepGoal;
  bool   ShowRing;
  bool   InvertBW;
  bool   Show24h;
} BarSettings;

static BarSettings s_settings;

static void prv_default_settings(void) {
  s_settings.BackgroundColor = GColorBlack;
  s_settings.BarLitColor     = GColorWhite;
  s_settings.BarDimColor     = GColorDarkGray;
  s_settings.DateTextColor   = GColorWhite;
  s_settings.RingBattColor   = GColorWhite;
  s_settings.RingStepsColor  = GColorWhite;
  s_settings.RingDimColor    = GColorDarkGray;
  s_settings.StepGoal        = 10000;
  s_settings.ShowRing        = false;
  s_settings.InvertBW        = false;
  s_settings.Show24h         = false;
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
static int     s_hour   = 0;
static int     s_minute = 0;
static char    s_date_buf[16];

// ============================================================
// DRAW HELPERS
// ============================================================

// Draw a vertical bar with tick separator lines.
// Fills from bottom up. Ticks are bg-colored cuts across bar width.
static void draw_bar(GContext *ctx,
                     int bx, int by, int bw, int bh,
                     int filled_px, int tick_count,
                     GColor col_lit, GColor col_bg) {
  if (filled_px > 0) {
    graphics_context_set_fill_color(ctx, col_lit);
    graphics_fill_rect(ctx, GRect(bx, by + bh - filled_px, bw, filled_px), 0, GCornerNone);
  }
  for (int i = 1; i <= tick_count; i++) {
    int ty = by + (bh * i) / (tick_count + 1);
    graphics_context_set_fill_color(ctx, col_bg);
    graphics_fill_rect(ctx, GRect(bx, ty, bw, 1), 0, GCornerNone);
  }
}

// Draw short tick lines outside the bar edge
static void draw_outer_ticks(GContext *ctx,
                              int bx, int by, int bw, int bh,
                              int tick_count, bool left_side,
                              int tick_len, GColor col) {
  graphics_context_set_stroke_color(ctx, col);
  graphics_context_set_stroke_width(ctx, 1);
  for (int i = 1; i <= tick_count; i++) {
    int ty = by + (bh * i) / (tick_count + 1);
    if (left_side) {
      graphics_draw_line(ctx, GPoint(bx - tick_len, ty), GPoint(bx - 1, ty));
    } else {
      graphics_draw_line(ctx, GPoint(bx + bw, ty), GPoint(bx + bw + tick_len - 1, ty));
    }
  }
}

// ============================================================
// RECT DRAW
// ============================================================
static void draw_rect(GContext *ctx, int w, int h) {
  bool show24 = s_settings.Show24h;

#if defined(PBL_BW)
  GColor col_bg  = s_settings.InvertBW ? GColorWhite : GColorBlack;
  GColor col_lit = s_settings.InvertBW ? GColorBlack : GColorWhite;
  GColor col_txt = col_lit;
#else
  GColor col_bg  = s_settings.BackgroundColor;
  GColor col_lit = s_settings.BarLitColor;
  GColor col_txt = s_settings.DateTextColor;
#endif

  int bar_y = BAR_MARGIN_TOP;
  int bar_h = h - BAR_MARGIN_TOP - BAR_MARGIN_BOT - DATE_H;
  if (bar_h < 10) bar_h = 10;

  // Hour bar: [LABEL_W][TICK_LEN][BAR_W] from left edge
  int hx = LABEL_W + TICK_LEN;

  // Minute bar: [BAR_W][TICK_LEN][LABEL_W] from right edge
  int mx = w - LABEL_W - TICK_LEN - BAR_W;

  int max_h = show24 ? 24 : 12;
  int dh    = show24 ? s_hour : (s_hour % 12);
  int hfill = (bar_h * dh) / max_h;
  int mfill = (s_minute == 0) ? 0 : (bar_h * s_minute) / 59;
  int hticks = max_h - 1;
  int mticks = 11;

  draw_bar(ctx, hx, bar_y, BAR_W, bar_h, hfill, hticks, col_lit, col_bg);
  draw_outer_ticks(ctx, hx, bar_y, BAR_W, bar_h, hticks, true, TICK_LEN, col_lit);
  draw_bar(ctx, mx, bar_y, BAR_W, bar_h, mfill, mticks, col_lit, col_bg);
  draw_outer_ticks(ctx, mx, bar_y, BAR_W, bar_h, mticks, false, TICK_LEN, col_lit);

  // Hour labels (right-aligned, left of bar)
  {
    GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_14);
    graphics_context_set_text_color(ctx, col_txt);
    char buf[4];
    for (int i = 1; i <= max_h; i++) {
      if (show24 && (i % 2 != 0)) continue;
      int ty = bar_y + bar_h - (bar_h * i) / max_h;
      snprintf(buf, sizeof(buf), show24 ? "%02d" : "%d", i);
      graphics_draw_text(ctx, buf, font,
        GRect(0, ty - 8, LABEL_W, 16),
        GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);
    }
  }

  // Minute labels (left-aligned, right of bar)
  {
    GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_14);
    graphics_context_set_text_color(ctx, col_txt);
    char buf[4];
    int lx = mx + BAR_W + TICK_LEN;
    for (int m = 5; m <= 60; m += 5) {
      int ty = bar_y + bar_h - (bar_h * m) / 60;
      snprintf(buf, sizeof(buf), "%d", m);
      graphics_draw_text(ctx, buf, font,
        GRect(lx, ty - 8, LABEL_W, 16),
        GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
    }
  }

  // Date
  graphics_context_set_text_color(ctx, col_txt);
  graphics_draw_text(ctx, s_date_buf,
    fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
    GRect(0, h - DATE_H, w, DATE_H),
    GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
}

// ============================================================
// ROUND DRAW
// ============================================================
static void draw_round(GContext *ctx, int w, int h) {
  bool show24 = s_settings.Show24h;
  int cx = w / 2, cy = h / 2, r = cx;

#if defined(PBL_BW)
  GColor col_bg  = GColorBlack;
  GColor col_lit = GColorWhite;
  GColor col_txt = GColorWhite;
#else
  GColor col_bg  = s_settings.BackgroundColor;
  GColor col_lit = s_settings.BarLitColor;
  GColor col_txt = s_settings.DateTextColor;
#endif

  int bw      = ROUND_BAR_W;
  int gap     = ROUND_BAR_GAP;
  int date_h  = ROUND_DATE_H;
  int dgap    = ROUND_DATE_GAP;
  int bar_h   = h - ROUND_TICK_M * 2 - date_h - dgap;
  if (bar_h < 10) bar_h = 10;
  int bar_top = (h - bar_h - date_h - dgap) / 2;
  int hx      = cx - gap / 2 - bw;
  int mx      = cx + gap / 2;

  int max_h  = show24 ? 24 : 12;
  int dh     = show24 ? s_hour : (s_hour % 12);
  int hfill  = (bar_h * dh) / max_h;
  int mfill  = (s_minute == 0) ? 0 : (bar_h * s_minute) / 59;
  int hticks = max_h - 1;
  int mticks = 11;

  draw_bar(ctx, hx, bar_top, bw, bar_h, hfill, hticks, col_lit, col_bg);
  draw_bar(ctx, mx, bar_top, bw, bar_h, mfill, mticks, col_lit, col_bg);

  graphics_context_set_stroke_color(ctx, col_lit);
  graphics_context_set_stroke_width(ctx, 1);
  for (int i = 1; i <= hticks; i++) {
    int ty = bar_top + (bar_h * i) / (hticks + 1);
    int dy = ty - cy;
    int chord = (dy*dy < r*r) ? prv_isqrt(r*r - dy*dy) : 0;
    graphics_draw_line(ctx, GPoint(cx - chord, ty), GPoint(hx - 1, ty));
  }
  for (int i = 1; i <= mticks; i++) {
    int ty = bar_top + (bar_h * i) / (mticks + 1);
    int dy = ty - cy;
    int chord = (dy*dy < r*r) ? prv_isqrt(r*r - dy*dy) : 0;
    graphics_draw_line(ctx, GPoint(mx + bw, ty), GPoint(cx + chord, ty));
  }

  graphics_context_set_text_color(ctx, col_txt);
  graphics_draw_text(ctx, s_date_buf,
    fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
    GRect(0, bar_top + bar_h + dgap, w, date_h),
    GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
}

// ============================================================
// MAIN DRAW CALLBACK
// ============================================================
static void draw_layer(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  int w = bounds.size.w, h = bounds.size.h;

#if defined(PBL_BW)
  GColor col_bg = s_settings.InvertBW ? GColorWhite : GColorBlack;
#else
  GColor col_bg = s_settings.BackgroundColor;
#endif
  graphics_context_set_fill_color(ctx, col_bg);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

#if defined(PBL_ROUND)
  draw_round(ctx, w, h);
#else
  draw_rect(ctx, w, h);
#endif
}

// ============================================================
// EVENT HANDLERS
// ============================================================
static void tick_handler(struct tm *t, TimeUnits u) {
  s_hour   = t->tm_hour;
  s_minute = t->tm_min;
  strftime(s_date_buf, sizeof(s_date_buf), "%a, %b %e", t);
  layer_mark_dirty(s_canvas_layer);
}

static void inbox_received(DictionaryIterator *iter, void *context) {
  Tuple *t;
  t = dict_find(iter, MESSAGE_KEY_BackgroundColor);
  if (t) s_settings.BackgroundColor = GColorFromHEX(t->value->int32);
  t = dict_find(iter, MESSAGE_KEY_BarLitColor);
  if (t) s_settings.BarLitColor     = GColorFromHEX(t->value->int32);
  t = dict_find(iter, MESSAGE_KEY_BarDimColor);
  if (t) s_settings.BarDimColor     = GColorFromHEX(t->value->int32);
  t = dict_find(iter, MESSAGE_KEY_DateTextColor);
  if (t) s_settings.DateTextColor   = GColorFromHEX(t->value->int32);
  t = dict_find(iter, MESSAGE_KEY_RingBattColor);
  if (t) s_settings.RingBattColor   = GColorFromHEX(t->value->int32);
  t = dict_find(iter, MESSAGE_KEY_RingStepsColor);
  if (t) s_settings.RingStepsColor  = GColorFromHEX(t->value->int32);
  t = dict_find(iter, MESSAGE_KEY_RingDimColor);
  if (t) s_settings.RingDimColor    = GColorFromHEX(t->value->int32);
  t = dict_find(iter, MESSAGE_KEY_StepGoal);
  if (t) s_settings.StepGoal        = (int)t->value->int32;
  t = dict_find(iter, MESSAGE_KEY_ShowRing);
  if (t) s_settings.ShowRing        = (t->value->int32 == 1);
  t = dict_find(iter, MESSAGE_KEY_InvertBW);
  if (t) s_settings.InvertBW        = (t->value->int32 == 1);
  t = dict_find(iter, MESSAGE_KEY_Show24h);
  if (t) s_settings.Show24h         = (t->value->int32 == 1);
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
  window_set_background_color(s_window, GColorBlack);
  window_set_window_handlers(s_window, (WindowHandlers){
    .load   = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
  time_t now = time(NULL);
  tick_handler(localtime(&now), MINUTE_UNIT);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  app_message_register_inbox_received(inbox_received);
  app_message_open(256, 64);
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

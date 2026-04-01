#include <pebble.h>

// ============================================================
// Bar Graph 2 — main.c
// v2.2: layout fixes — correct label positions, no negatives
// ============================================================

#define SETTINGS_KEY 1

// Rect layout
#define BAR_W          30
#define LABEL_W        24    // column for labels
#define TICK_LEN        4    // outer tick line length
#define DATE_H         20
#define BAR_TOP         2    // top margin
#define BAR_BOT         2    // gap above date row

// Round layout
#define ROUND_BAR_W    22
#define ROUND_BAR_GAP   8
#define ROUND_TICK_M   26
#define ROUND_DATE_H   18
#define ROUND_DATE_GAP  4

// Font height for GOTHIC_14: cap ~10px, total ~14px. We center on tick.
#define LABEL_H        14
#define LABEL_HALF      7    // LABEL_H / 2

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

// Vertical bar, fills bottom-up. Tick cuts in bg color across full width.
static void draw_bar(GContext *ctx,
                     int bx, int by, int bw, int bh,
                     int filled_px, int num_segments,
                     GColor col_lit, GColor col_bg) {
  // Lit fill
  if (filled_px > 0) {
    int fy = by + bh - filled_px;
    if (fy < by) { filled_px = bh; fy = by; }
    graphics_context_set_fill_color(ctx, col_lit);
    graphics_fill_rect(ctx, GRect(bx, fy, bw, filled_px), 0, GCornerNone);
  }
  // Segment separator lines (num_segments-1 internal lines for num_segments slots)
  // Segments are equal height: line i is at by + bh * i / num_segments
  for (int i = 1; i < num_segments; i++) {
    int ty = by + (bh * i) / num_segments;
    graphics_context_set_fill_color(ctx, col_bg);
    graphics_fill_rect(ctx, GRect(bx, ty, bw, 1), 0, GCornerNone);
  }
}

// Short tick marks outside bar edge, at same positions as segment lines
static void draw_outer_ticks(GContext *ctx,
                              int bx, int by, int bw, int bh,
                              int num_segments, bool left_side,
                              int tick_len, GColor col) {
  graphics_context_set_stroke_color(ctx, col);
  graphics_context_set_stroke_width(ctx, 1);
  for (int i = 1; i < num_segments; i++) {
    int ty = by + (bh * i) / num_segments;
    if (left_side) {
      graphics_draw_line(ctx, GPoint(bx - tick_len, ty), GPoint(bx - 1, ty));
    } else {
      graphics_draw_line(ctx, GPoint(bx + bw, ty), GPoint(bx + bw + tick_len - 1, ty));
    }
  }
}

// Draw a label centered vertically on tick_y, clamped to [min_y, max_y]
// Returns false if the label would be entirely outside bounds (skip it)
static bool label_rect(int tick_y, int min_y, int max_y, GRect *out) {
  int top = tick_y - LABEL_HALF;
  int bot = top + LABEL_H;
  // Clamp
  if (top < min_y) top = min_y;
  if (bot > max_y) bot = max_y;
  if (bot - top < 6) return false;  // too squished, skip
  *out = GRect(0, top, 0, bot - top);  // x/w filled in by caller
  return true;
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

  int bar_y = BAR_TOP;
  int bar_h = h - BAR_TOP - BAR_BOT - DATE_H;
  if (bar_h < 20) bar_h = 20;

  // Hour bar x: [LABEL_W][TICK_LEN][BAR_W=30] from left
  int hx = LABEL_W + TICK_LEN;

  // Minute bar x: [BAR_W=30][TICK_LEN][LABEL_W] from right
  int mx = w - LABEL_W - TICK_LEN - BAR_W;

  // Label columns
  int hour_label_x  = 0;
  int min_label_x   = mx + BAR_W + TICK_LEN;
  int min_label_w   = w - min_label_x;  // remaining px to right edge

  // Segments: equal-height slots, one per hour / one per 5 min
  int max_h    = show24 ? 24 : 12;
  int dh       = show24 ? s_hour : (s_hour % 12);
  // Fill: dh complete segments filled
  int hfill_px = (bar_h * dh) / max_h;
  // Minutes: 60 slots of equal height, s_minute slots filled
  int mfill_px = (bar_h * s_minute) / 60;

  // Draw bars
  draw_bar(ctx, hx, bar_y, BAR_W, bar_h, hfill_px, max_h, col_lit, col_bg);
  draw_outer_ticks(ctx, hx, bar_y, BAR_W, bar_h, max_h, true, TICK_LEN, col_lit);
  draw_bar(ctx, mx, bar_y, BAR_W, bar_h, mfill_px, 12, col_lit, col_bg);
  draw_outer_ticks(ctx, mx, bar_y, BAR_W, bar_h, 12, false, TICK_LEN, col_lit);

  GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  graphics_context_set_text_color(ctx, col_txt);

  // Hour labels: at top of each segment boundary (i = 1..max_h)
  // Position of segment top boundary i = bar_y + bar_h - (bar_h * i / max_h)
  // i.e. bottom = bar_y+bar_h (=0h), top = bar_y (=max_h)
  // Label every hour in 12h; every 2h in 24h
  {
    char buf[4];
    int step = show24 ? 2 : 1;
    for (int i = step; i <= max_h; i += step) {
      int tick_y = bar_y + bar_h - (bar_h * i) / max_h;
      // Clamp to drawable area
      int ly = tick_y - LABEL_HALF;
      if (ly < 0) ly = 0;
      if (ly + LABEL_H > bar_y + bar_h) continue;
      snprintf(buf, sizeof(buf), "%d", i);
      graphics_draw_text(ctx, buf, font,
        GRect(hour_label_x, ly, LABEL_W, LABEL_H),
        GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);
    }
  }

  // Minute labels: every 5 min at segment boundary
  // 12 segments of 5 min each. Boundary i (1..12) = i*5 minutes
  // tick_y for boundary i = bar_y + bar_h - (bar_h * i / 12)
  {
    char buf[4];
    for (int i = 1; i <= 12; i++) {
      int m = i * 5;
      int tick_y = bar_y + bar_h - (bar_h * i) / 12;
      int ly = tick_y - LABEL_HALF;
      if (ly < 0) ly = 0;
      if (ly + LABEL_H > bar_y + bar_h) continue;
      snprintf(buf, sizeof(buf), "%d", m);
      graphics_draw_text(ctx, buf, font,
        GRect(min_label_x, ly, min_label_w, LABEL_H),
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

  int max_h    = show24 ? 24 : 12;
  int dh       = show24 ? s_hour : (s_hour % 12);
  int hfill_px = (bar_h * dh) / max_h;
  int mfill_px = (bar_h * s_minute) / 60;

  draw_bar(ctx, hx, bar_top, bw, bar_h, hfill_px, max_h, col_lit, col_bg);
  draw_bar(ctx, mx, bar_top, bw, bar_h, mfill_px, 12,    col_lit, col_bg);

  // Ticks to circle edge
  graphics_context_set_stroke_color(ctx, col_lit);
  graphics_context_set_stroke_width(ctx, 1);
  for (int i = 1; i < max_h; i++) {
    int ty = bar_top + (bar_h * i) / max_h;
    int dy = ty - cy;
    int chord = (dy*dy < r*r) ? prv_isqrt(r*r - dy*dy) : 0;
    graphics_draw_line(ctx, GPoint(cx - chord, ty), GPoint(hx - 1, ty));
  }
  for (int i = 1; i < 12; i++) {
    int ty = bar_top + (bar_h * i) / 12;
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

#include <pebble.h>

// ============================================================
// Bar Graph 2 — main.c  v2.15 (slot 11px, bar top y=8; system fonts, switchable)
// Fixes:
//   1. Separator cut: only when ty > h_fill_top (fill strictly
//      above the line, not just at it) — prevents premature flip
//   2. Top and bottom border lines at bar_top and bar_bot
//      spanning bar width + nubs on both bars
// ============================================================

#define SETTINGS_KEY 1

#define V1_BAR_BOT  140
#define V1_BAR_TOP    8
#define V1_HX        38
#define V1_BAR_W     30
#define V1_MX        76
#define V1_DATE_Y   146
#define V1_SLOT_H    11
#define NUB_PX        8
#define GAP_PX        8

// Fonts: 1 = built-in Pebble fonts (no resources needed), 0 = custom Arial (FONT_ARIAL_11/18)
#define USE_SYSTEM_FONTS 1
#if USE_SYSTEM_FONTS
  #define LABEL_FONT_KEY  FONT_KEY_GOTHIC_14   // swap to FONT_KEY_GOTHIC_14_BOLD for bold digits
  #define DATE_FONT_KEY   FONT_KEY_GOTHIC_18
  #define LABEL_DY        1   // ESTIMATE for Gothic 14 — lifts digits so their center sits on the tick line; re-measure from a screenshot
#else
  #define LABEL_DY        1   // Arial 11 in a 16px box (was 3 in a 12px box)
#endif

#define ROUND_BAR_W    22
#define ROUND_BAR_GAP  10
#define ROUND_DATE_H   20
#define ROUND_DATE_GAP  4
#define ROUND_MARGIN   26

static int prv_isqrt(int n) {
  if (n <= 0) return 0;
  int x = n, y = (x + 1) / 2;
  while (y < x) { x = y; y = (x + n / x) / 2; }
  return x;
}

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

static Window  *s_window;
static Layer   *s_canvas_layer;
static GFont    s_font_label;
static GFont    s_font_date;
static bool     s_custom_fonts = false;
static int      s_hour   = 0;
static int      s_minute = 0;
static char     s_date_buf[16];

// Draw the top+bottom border lines for one bar (at bar_top and bar_bot)
// plus nubs extending outward.
static void draw_bar_borders(GContext *ctx,
                             int bx, int bw, int bar_top, int bar_bot,
                             bool left_nub, bool right_nub, int nub,
                             GColor col) {
  graphics_context_set_stroke_color(ctx, col);
  graphics_context_set_stroke_width(ctx, 1);
  // Bottom border (00 line)
  graphics_draw_line(ctx, GPoint(bx, bar_bot), GPoint(bx + bw - 1, bar_bot));
  if (left_nub)  graphics_draw_line(ctx, GPoint(bx - nub, bar_bot), GPoint(bx - 1, bar_bot));
  if (right_nub) graphics_draw_line(ctx, GPoint(bx + bw, bar_bot), GPoint(bx + bw + nub - 1, bar_bot));
  // Top border (12 line)
  graphics_draw_line(ctx, GPoint(bx, bar_top), GPoint(bx + bw - 1, bar_top));
  if (left_nub)  graphics_draw_line(ctx, GPoint(bx - nub, bar_top), GPoint(bx - 1, bar_top));
  if (right_nub) graphics_draw_line(ctx, GPoint(bx + bw, bar_top), GPoint(bx + bw + nub - 1, bar_top));
}

// ============================================================
// RECT DRAW
// ============================================================
#if !defined(PBL_ROUND)
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

  int hx      = (V1_HX      * w) / 144;
  int bw      = (V1_BAR_W   * w) / 144;
  int mx      = (V1_MX      * w) / 144;
  int bar_bot = (V1_BAR_BOT * h) / 168;
  int slot_h  = (V1_SLOT_H  * h) / 168;
  int date_y  = (V1_DATE_Y  * h) / 168;

  if (slot_h < 2) slot_h = 2;
  if (bw < 20) bw = 20;

  int bar_h   = slot_h * 12;
  int bar_top = bar_bot - bar_h;

  int max_h = show24 ? 24 : 12;
  int dh    = show24 ? s_hour : (s_hour % 12);
  int hfill = show24 ? (bar_h * dh / 24) : (slot_h * dh);
  int mfill = (slot_h * s_minute) / 5;

  int h_fill_top = bar_bot - hfill;  // top of hour fill (bar_bot when hfill=0)
  int m_fill_top = bar_bot - mfill;  // top of minute fill

  int h_label_w = hx - NUB_PX - GAP_PX;
  int m_label_x = mx + bw + NUB_PX + GAP_PX;
  int m_label_w = w - m_label_x;
  int lh = 16;   // tall enough for Gothic 14 (Arial 11 fits in less)

  // ---- 1. Tick lines + nubs (before fills) ----
  graphics_context_set_stroke_width(ctx, 1);
  graphics_context_set_stroke_color(ctx, col_lit);
  for (int i = 1; i < 12; i++) {
    int ty = bar_bot - slot_h * i;
    graphics_draw_line(ctx, GPoint(hx,           ty), GPoint(hx + bw - 1,           ty));
    graphics_draw_line(ctx, GPoint(mx,           ty), GPoint(mx + bw - 1,           ty));
    graphics_draw_line(ctx, GPoint(hx - NUB_PX, ty), GPoint(hx - 1,                ty));
    graphics_draw_line(ctx, GPoint(mx + bw,      ty), GPoint(mx + bw + NUB_PX - 1, ty));
  }

  // ---- 2. Top/bottom border lines ----
  draw_bar_borders(ctx, hx, bw, bar_top, bar_bot, true,  false, NUB_PX, col_lit);
  draw_bar_borders(ctx, mx, bw, bar_top, bar_bot, false, true,  NUB_PX, col_lit);

  // ---- 3. Bar fills (cover tick lines in filled region) ----
  if (hfill > 0) {
    graphics_context_set_fill_color(ctx, col_lit);
    graphics_fill_rect(ctx, GRect(hx, h_fill_top, bw, hfill), 0, GCornerNone);
  }
  if (mfill > 0) {
    graphics_context_set_fill_color(ctx, col_lit);
    graphics_fill_rect(ctx, GRect(mx, m_fill_top, bw, mfill), 0, GCornerNone);
  }

  // ---- 4. Separator cuts — only strictly inside filled region ----
  // "strictly inside" = ty > fill_top (fill has gone above the line)
  // ty == fill_top means the fill just reached the line from below — no cut yet
  graphics_context_set_fill_color(ctx, col_bg);
  for (int i = 1; i < 12; i++) {
    int ty = bar_bot - slot_h * i;
    if (hfill > 0 && ty > h_fill_top && ty < bar_bot)
      graphics_fill_rect(ctx, GRect(hx, ty, bw, 1), 0, GCornerNone);
    if (mfill > 0 && ty > m_fill_top && ty < bar_bot)
      graphics_fill_rect(ctx, GRect(mx, ty, bw, 1), 0, GCornerNone);
  }

  // 24h: half-slot cuts on hour bar (strictly inside fill)
  if (show24 && hfill > 0) {
    graphics_context_set_fill_color(ctx, col_bg);
    for (int i = 0; i < 12; i++) {
      int ty = bar_bot - slot_h * i - slot_h / 2;
      if (ty > bar_top && ty < bar_bot && ty > h_fill_top)
        graphics_fill_rect(ctx, GRect(hx, ty, bw, 1), 0, GCornerNone);
    }
  }

  // ---- 5. Labels ----
  graphics_context_set_text_color(ctx, col_txt);
  {
    char buf[4];
    int step = show24 ? 2 : 1;
    for (int i = 0; i <= max_h; i += step) {
      int ty = bar_bot - (show24 ? (bar_h * i / 24) : (slot_h * i));
      int ly = ty - lh / 2 - LABEL_DY;
      if (ly + lh < bar_top - lh || ly > bar_bot + lh) continue;
      snprintf(buf, sizeof(buf), "%02d", i);
      graphics_draw_text(ctx, buf, s_font_label,
        GRect(0, ly, h_label_w, lh),
        GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);
    }
  }
  {
    char buf[4];
    for (int i = 0; i <= 12; i++) {
      int ty = bar_bot - slot_h * i;
      int ly = ty - lh / 2 - LABEL_DY;
      if (ly + lh < bar_top - lh || ly > bar_bot + lh) continue;
      snprintf(buf, sizeof(buf), "%02d", i * 5);
      graphics_draw_text(ctx, buf, s_font_label,
        GRect(m_label_x, ly, m_label_w, lh),
        GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
    }
  }

  // ---- 6. Date ----
  graphics_context_set_text_color(ctx, col_txt);
  graphics_draw_text(ctx, s_date_buf, s_font_date,
    GRect(0, date_y, w, h - date_y),
    GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
}
#endif

// ============================================================
// ROUND DRAW
// ============================================================
#if defined(PBL_ROUND)
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
  int slot_h  = (h - ROUND_MARGIN * 2 - date_h - dgap) / 12;
  if (slot_h < 2) slot_h = 2;
  int bar_h   = slot_h * 12;
  int bar_top = (h - bar_h - date_h - dgap) / 2;
  int bar_bot = bar_top + bar_h;
  int hx      = cx - gap / 2 - bw;
  int mx      = cx + gap / 2;

  int dh      = show24 ? s_hour : (s_hour % 12);
  int hfill   = show24 ? (bar_h * dh / 24) : (slot_h * dh);
  int mfill   = (slot_h * s_minute) / 5;
  int h_fill_top = bar_bot - hfill;
  int m_fill_top = bar_bot - mfill;

  // Ticks + border lines first
  graphics_context_set_stroke_width(ctx, 1);
  graphics_context_set_stroke_color(ctx, col_lit);
  for (int i = 1; i < 12; i++) {
    int ty = bar_bot - slot_h * i;
    int dy = ty - cy;
    int chord = (dy*dy < r*r) ? prv_isqrt(r*r - dy*dy) : 0;
    graphics_draw_line(ctx, GPoint(hx,         ty), GPoint(hx + bw - 1, ty));
    graphics_draw_line(ctx, GPoint(mx,         ty), GPoint(mx + bw - 1, ty));
    graphics_draw_line(ctx, GPoint(cx - chord, ty), GPoint(hx - 1,      ty));
    graphics_draw_line(ctx, GPoint(mx + bw,    ty), GPoint(cx + chord,  ty));
  }
  // Border lines at top and bottom
  {
    int dy_bot = bar_bot - cy;
    int chord_bot = (dy_bot*dy_bot < r*r) ? prv_isqrt(r*r - dy_bot*dy_bot) : 0;
    int dy_top = bar_top - cy;
    int chord_top = (dy_top*dy_top < r*r) ? prv_isqrt(r*r - dy_top*dy_top) : 0;
    graphics_draw_line(ctx, GPoint(hx, bar_bot), GPoint(hx + bw - 1, bar_bot));
    graphics_draw_line(ctx, GPoint(mx, bar_bot), GPoint(mx + bw - 1, bar_bot));
    graphics_draw_line(ctx, GPoint(cx - chord_bot, bar_bot), GPoint(hx - 1, bar_bot));
    graphics_draw_line(ctx, GPoint(mx + bw, bar_bot), GPoint(cx + chord_bot, bar_bot));
    graphics_draw_line(ctx, GPoint(hx, bar_top), GPoint(hx + bw - 1, bar_top));
    graphics_draw_line(ctx, GPoint(mx, bar_top), GPoint(mx + bw - 1, bar_top));
    graphics_draw_line(ctx, GPoint(cx - chord_top, bar_top), GPoint(hx - 1, bar_top));
    graphics_draw_line(ctx, GPoint(mx + bw, bar_top), GPoint(cx + chord_top, bar_top));
  }

  // Fills
  if (hfill > 0) {
    graphics_context_set_fill_color(ctx, col_lit);
    graphics_fill_rect(ctx, GRect(hx, h_fill_top, bw, hfill), 0, GCornerNone);
  }
  if (mfill > 0) {
    graphics_context_set_fill_color(ctx, col_lit);
    graphics_fill_rect(ctx, GRect(mx, m_fill_top, bw, mfill), 0, GCornerNone);
  }

  // Separator cuts strictly inside fill
  graphics_context_set_fill_color(ctx, col_bg);
  for (int i = 1; i < 12; i++) {
    int ty = bar_bot - slot_h * i;
    if (hfill > 0 && ty > h_fill_top && ty < bar_bot)
      graphics_fill_rect(ctx, GRect(hx, ty, bw, 1), 0, GCornerNone);
    if (mfill > 0 && ty > m_fill_top && ty < bar_bot)
      graphics_fill_rect(ctx, GRect(mx, ty, bw, 1), 0, GCornerNone);
  }

  graphics_context_set_text_color(ctx, col_txt);
  graphics_draw_text(ctx, s_date_buf, s_font_date,
    GRect(0, bar_bot + dgap, w, date_h),
    GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
}
#endif

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
#if USE_SYSTEM_FONTS
  s_font_label   = fonts_get_system_font(LABEL_FONT_KEY);
  s_font_date    = fonts_get_system_font(DATE_FONT_KEY);
  s_custom_fonts = false;
#elif defined(RESOURCE_ID_FONT_ARIAL_11)
  s_font_label   = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ARIAL_11));
  s_font_date    = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ARIAL_18));
  s_custom_fonts = true;
#else
  s_font_label   = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  s_font_date    = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  s_custom_fonts = false;
#endif
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
  if (s_custom_fonts) {
    fonts_unload_custom_font(s_font_label);
    fonts_unload_custom_font(s_font_date);
  }
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

#include <pebble.h>

// ============================================================
// Bar Graph 2 — main.c  v2.21 (flint/emery locked; aplite/basalt/diorite date Gothic 24 Bold; round: wider bars + more label space)
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
#define V1_DATE_Y   143   // aplite: margins top 4 / bar-to-date 6 / bottom 4
#define V1_SLOT_H    11
#define NUB_PX        8
#define GAP_PX        8

// TEMP TEST: force a worst-case-width date ("Wed, Dec 30"). Set to 0 to use the real date.
#define DEBUG_FIXED_DATE 1

// Fonts: 1 = built-in Pebble fonts (no resources needed), 0 = custom Arial (FONT_ARIAL_11/18)
// NOTE: Leco only has real letters on newer firmware (flint, emery, gabbro). On aplite,
// basalt, chalk and diorite it renders letters as junk boxes, so those use Gothic (24 Bold on 144x168, 18 Bold on chalk).
// DATE_Y_ADJ shifts the date box so different fonts land the glyphs in the same place.
#define USE_SYSTEM_FONTS 1
#if USE_SYSTEM_FONTS
  #if defined(PBL_PLATFORM_EMERY) || defined(PBL_PLATFORM_GABBRO)
    // Large screens (emery 200x228 LOCKED, gabbro 260x260): one size up from the small platforms.
    #define LABEL_FONT_KEY  FONT_KEY_GOTHIC_18_BOLD
    #define DATE_FONT_KEY   FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM
    #define DATE_Y_ADJ       0
    #define LABEL_LH        22
    #define LABEL_DY         1
    #define ROUND_DATE_BOX  34   // vertical extent of the date below the bars (gabbro)
    #define EMERY_BAR_BOT  192
    #define EMERY_SLOT_H    15
    #define EMERY_DATE_Y   196
  #else
    // 144x168 (aplite, basalt, diorite, flint) and chalk (180x180)
    #define LABEL_FONT_KEY  FONT_KEY_GOTHIC_14_BOLD   // or FONT_KEY_GOTHIC_14
    #define LABEL_LH        16
    #define LABEL_DY         1   // measured in simulator: digit centers land exactly on tick lines
    #define ROUND_DATE_BOX  25   // vertical extent of the date below the bars (chalk)
    #if defined(PBL_PLATFORM_FLINT)
      // flint: LOCKED
      #define DATE_FONT_KEY FONT_KEY_LECO_20_BOLD_NUMBERS
      #define DATE_Y_ADJ     0
    #elif defined(PBL_ROUND)
      // chalk. ESTIMATE: Gothic glyphs sit ~3px higher in the box than Leco's.
      #define DATE_FONT_KEY FONT_KEY_GOTHIC_18_BOLD
      #define DATE_Y_ADJ     3
    #else
      // aplite / basalt / diorite: one size up (Gothic 24 Bold). ESTIMATE: box offset, 1px higher than before.
      #define DATE_FONT_KEY FONT_KEY_GOTHIC_24_BOLD
      #define DATE_Y_ADJ    -1
    #endif
  #endif
#else
  #define LABEL_LH          16
  #define LABEL_DY           1   // Arial 11 in a 16px box (was 3 in a 12px box)
  #define DATE_Y_ADJ         0
  #define ROUND_DATE_BOX    25
#endif

// Round layout: all sizes below are for a 180px-wide screen and scale with width.
#define ROUND_BAR_W_REF   28   // bar width
#define ROUND_C_REF        3   // half-gap between hour and minute label columns
#define ROUND_PAD_REF      5   // gap between label column and bar
#define ROUND_SHIFT_Y      2   // nudge the whole block down (px)


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
#if defined(PBL_PLATFORM_EMERY) && USE_SYSTEM_FONTS
  int bar_bot = EMERY_BAR_BOT;
  int slot_h  = EMERY_SLOT_H;
  int date_y  = EMERY_DATE_Y;
#else
  int bar_bot = (V1_BAR_BOT * h) / 168;
  int slot_h  = (V1_SLOT_H  * h) / 168;
  int date_y  = (V1_DATE_Y  * h) / 168;
#endif

  date_y += DATE_Y_ADJ;
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
  int lh = LABEL_LH;

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
    GRect(0, date_y, w, 40),   // taller than the remaining screen on purpose: Gothic 24 needs ~28px line height
    GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
}
#endif

// ============================================================
// ROUND DRAW — labels live in a center column between the bars
// ============================================================
#if defined(PBL_ROUND)
// One graduation line: bar spans + outer tick out to the circle edge (inset 2px from the rim).
static void prv_round_line(GContext *ctx, int ty, int cx, int cy, int r, int hx, int mx, int bw) {
  int dy = ty - cy;
  int chord = (dy*dy < r*r) ? prv_isqrt(r*r - dy*dy) : 0;
  int lx = cx - chord + 2;
  int rx = cx + chord - 2;
  graphics_draw_line(ctx, GPoint(hx, ty), GPoint(hx + bw - 1, ty));
  graphics_draw_line(ctx, GPoint(mx, ty), GPoint(mx + bw - 1, ty));
  if (lx < hx - 1)       graphics_draw_line(ctx, GPoint(lx, ty), GPoint(hx - 1, ty));
  if (rx > mx + bw)      graphics_draw_line(ctx, GPoint(mx + bw, ty), GPoint(rx, ty));
}

static void draw_round(GContext *ctx, int w, int h) {
  bool show24 = s_settings.Show24h;
  int cx = w / 2, cy = h / 2, r = w / 2;

  GColor col_bg  = s_settings.BackgroundColor;
  GColor col_lit = s_settings.BarLitColor;
  GColor col_txt = s_settings.DateTextColor;

  int lh = LABEL_LH;
  // Width of a two-digit label in the chosen font, measured at runtime
  GSize lsz = graphics_text_layout_get_content_size("00", s_font_label,
                GRect(0, 0, 100, 40), GTextOverflowModeWordWrap, GTextAlignmentLeft);
  int lab_w = lsz.w;

  int bw  = (ROUND_BAR_W_REF * w) / 180;
  int c   = (ROUND_C_REF     * w) / 180;
  int pad = (ROUND_PAD_REF   * w) / 180;
  if (c < 2) c = 2;
  int hx  = cx - c - lab_w - pad - bw;
  int mx  = cx + c + lab_w + pad;

  int slot_h = ((h * 2) / 3) / 12;
  if (slot_h < 2) slot_h = 2;
  int bar_h  = slot_h * 12;
  int dgap   = (4 * h) / 180;
  int total  = bar_h + dgap + ROUND_DATE_BOX;
  int bar_top = (h - total) / 2 + ROUND_SHIFT_Y;
  int bar_bot = bar_top + bar_h;
  int date_y  = bar_bot + dgap + DATE_Y_ADJ;

  int max_h  = show24 ? 24 : 12;
  int dh     = show24 ? s_hour : (s_hour % 12);
  int hfill  = show24 ? (bar_h * dh / 24) : (slot_h * dh);
  int mfill  = (slot_h * s_minute) / 5;
  int h_fill_top = bar_bot - hfill;
  int m_fill_top = bar_bot - mfill;

  // 1. Graduation lines (12 slots + both borders), before fills
  graphics_context_set_stroke_width(ctx, 1);
  graphics_context_set_stroke_color(ctx, col_lit);
  for (int i = 0; i <= 12; i++) {
    prv_round_line(ctx, bar_bot - slot_h * i, cx, cy, r, hx, mx, bw);
  }

  // 2. Fills
  if (hfill > 0) {
    graphics_context_set_fill_color(ctx, col_lit);
    graphics_fill_rect(ctx, GRect(hx, h_fill_top, bw, hfill), 0, GCornerNone);
  }
  if (mfill > 0) {
    graphics_context_set_fill_color(ctx, col_lit);
    graphics_fill_rect(ctx, GRect(mx, m_fill_top, bw, mfill), 0, GCornerNone);
  }

  // 3. Separator cuts strictly inside fill
  graphics_context_set_fill_color(ctx, col_bg);
  for (int i = 1; i < 12; i++) {
    int ty = bar_bot - slot_h * i;
    if (hfill > 0 && ty > h_fill_top && ty < bar_bot)
      graphics_fill_rect(ctx, GRect(hx, ty, bw, 1), 0, GCornerNone);
    if (mfill > 0 && ty > m_fill_top && ty < bar_bot)
      graphics_fill_rect(ctx, GRect(mx, ty, bw, 1), 0, GCornerNone);
  }
  if (show24 && hfill > 0) {
    for (int i = 0; i < 12; i++) {
      int ty = bar_bot - slot_h * i - slot_h / 2;
      if (ty > bar_top && ty < bar_bot && ty > h_fill_top)
        graphics_fill_rect(ctx, GRect(hx, ty, bw, 1), 0, GCornerNone);
    }
  }

  // 4. Center-column labels: hour right-aligned on the left of center, minute left-aligned on the right
  graphics_context_set_text_color(ctx, col_txt);
  {
    char buf[4];
    int step = show24 ? 2 : 1;
    for (int i = 0; i <= max_h; i += step) {
      int ty = bar_bot - (show24 ? (bar_h * i / 24) : (slot_h * i));
      int ly = ty - lh / 2 - LABEL_DY;
      snprintf(buf, sizeof(buf), "%02d", i);
      graphics_draw_text(ctx, buf, s_font_label,
        GRect(cx - c - lab_w - 4, ly, lab_w + 4, lh),
        GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);
    }
    for (int i = 0; i <= 12; i++) {
      int ty = bar_bot - slot_h * i;
      int ly = ty - lh / 2 - LABEL_DY;
      snprintf(buf, sizeof(buf), "%02d", i * 5);
      graphics_draw_text(ctx, buf, s_font_label,
        GRect(cx + c, ly, lab_w + 4, lh),
        GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
    }
  }

  // 5. Date
  graphics_draw_text(ctx, s_date_buf, s_font_date,
    GRect(0, date_y, w, ROUND_DATE_BOX),
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
#if DEBUG_FIXED_DATE
  snprintf(s_date_buf, sizeof(s_date_buf), "Wed, Dec 30");
#endif
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

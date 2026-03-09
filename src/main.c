#include <pebble.h>

// ============================================================
// CONSTANTS
// ============================================================
#define SETTINGS_KEY        1
#define DEFAULT_STEP_GOAL   10000

// Outer ring
#define RING_THICK          5     // px
#define RING_GAP            3     // px gap between ring and content

// Rect layout — bars hug opposite edges, labels between bar and edge.
// Left bar:  [ring_inset] [LABEL_W] [TICK_LEN] [BAR_W] ...
// Right bar: ... [BAR_W] [TICK_LEN] [LABEL_W] [ring_inset]
// The large gap between the two bars is intentional (it's most of the screen).
#define DATE_HEIGHT         22
#define BAR_MARGIN_TOP      4
#define BAR_MARGIN_BOTTOM   4
#define BAR_W               30    // matches v1 measured width
#define LABEL_W             26    // label column width
#define TICK_LEN            5     // tick line length outside bar

// Round layout — bars centered, labels+ticks extend to circle edge
#define ROUND_BAR_W         24
#define ROUND_BAR_GAP       10
#define ROUND_TICK_MARGIN   28
#define ROUND_LABEL_MARGIN  14
#define ROUND_DATE_H        18
#define ROUND_DATE_GAP      4

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
  int  StepGoal;
  bool ShowRing;
  bool InvertBW;
  bool Show24h;
} BarSettings;

static BarSettings s_settings;

static void prv_default_settings(void) {
  s_settings.BackgroundColor = GColorBlack;
  s_settings.BarLitColor    = GColorWhite;
  s_settings.BarDimColor    = GColorDarkGray;
  s_settings.DateTextColor  = GColorWhite;
  s_settings.RingBattColor  = GColorWhite;
  s_settings.RingStepsColor = GColorWhite;
  s_settings.RingDimColor   = GColorDarkGray;
  s_settings.StepGoal  = DEFAULT_STEP_GOAL;
  s_settings.ShowRing  = true;
  s_settings.InvertBW  = false;
  s_settings.Show24h   = false;
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

static int  s_hour    = 0;
static int  s_minute  = 0;
static int  s_battery = 100;
static int  s_steps   = 0;
static char s_date_buf[20];

// ============================================================
// INTEGER SQRT
// ============================================================
static int prv_isqrt(int n) {
  if (n <= 0) return 0;
  int x = n;
  int y = (x + 1) / 2;
  while (y < x) { x = y; y = (x + n / x) / 2; }
  return x;
}

// ============================================================
// EFFECTIVE COLORS
// ============================================================
static GColor eff_bg(void) {
#if defined(PBL_BW)
  return s_settings.InvertBW ? GColorWhite : GColorBlack;
#else
  return s_settings.BackgroundColor;
#endif
}
static GColor eff_lit(void) {
#if defined(PBL_BW)
  return s_settings.InvertBW ? GColorBlack : GColorWhite;
#else
  return s_settings.BarLitColor;
#endif
}
static GColor eff_dim(void) {
#if defined(PBL_BW)
  return s_settings.InvertBW ? GColorLightGray : GColorDarkGray;
#else
  return s_settings.BarDimColor;
#endif
}
static GColor eff_text(void) {
#if defined(PBL_BW)
  return s_settings.InvertBW ? GColorBlack : GColorWhite;
#else
  return s_settings.DateTextColor;
#endif
}
static GColor eff_ring_batt(void) {
#if defined(PBL_BW)
  return s_settings.InvertBW ? GColorBlack : GColorWhite;
#else
  return s_settings.RingBattColor;
#endif
}
static GColor eff_ring_steps(void) {
#if defined(PBL_BW)
  return s_settings.InvertBW ? GColorBlack : GColorWhite;
#else
  return s_settings.RingStepsColor;
#endif
}
static GColor eff_ring_dim(void) {
#if defined(PBL_BW)
  return s_settings.InvertBW ? GColorLightGray : GColorDarkGray;
#else
  return s_settings.RingDimColor;
#endif
}

// ============================================================
// OUTER RING — RECT
// ============================================================
#if !defined(PBL_ROUND)
static void draw_ring_rect(GContext *ctx, int w, int h) {
  int step_pct = (s_settings.StepGoal > 0)
    ? (s_steps * 100 / s_settings.StepGoal) : 0;
  if (step_pct > 100) step_pct = 100;

  int t   = RING_THICK;
  int gap = 5;
  int cx  = w / 2;
  int hw  = cx - gap;
  int total = hw + h + hw;

  graphics_context_set_fill_color(ctx, eff_bg());
  graphics_fill_rect(ctx, GRect(0,   0,   w, t), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(0,   h-t, w, t), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(0,   0,   t, h), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(w-t, 0,   t, h), 0, GCornerNone);

  graphics_context_set_fill_color(ctx, eff_ring_dim());
  graphics_fill_rect(ctx, GRect(cx+gap, h-t, hw, t), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(w-t,    0,   t,  h), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(cx+gap, 0,   hw, t), 0, GCornerNone);
  {
    int filled = total * s_battery / 100;
    graphics_context_set_fill_color(ctx, eff_ring_batt());
    if (filled > 0) {
      int seg = (filled < hw) ? filled : hw;
      graphics_fill_rect(ctx, GRect(cx+gap+hw-seg, h-t, seg, t), 0, GCornerNone);
      filled -= seg;
    }
    if (filled > 0) {
      int seg = (filled < h) ? filled : h;
      graphics_fill_rect(ctx, GRect(w-t, h-seg, t, seg), 0, GCornerNone);
      filled -= seg;
    }
    if (filled > 0) {
      int seg = (filled < hw) ? filled : hw;
      graphics_fill_rect(ctx, GRect(cx+gap, 0, seg, t), 0, GCornerNone);
    }
  }

  graphics_context_set_fill_color(ctx, eff_ring_dim());
  graphics_fill_rect(ctx, GRect(0,         h-t, hw, t), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(0,         0,   t,  h), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(cx-gap-hw, 0,   hw, t), 0, GCornerNone);
  if (step_pct > 0) {
    int filled = total * step_pct / 100;
    graphics_context_set_fill_color(ctx, eff_ring_steps());
    if (filled > 0) {
      int seg = (filled < hw) ? filled : hw;
      graphics_fill_rect(ctx, GRect(cx-gap-seg, h-t, seg, t), 0, GCornerNone);
      filled -= seg;
    }
    if (filled > 0) {
      int seg = (filled < h) ? filled : h;
      graphics_fill_rect(ctx, GRect(0, h-seg, t, seg), 0, GCornerNone);
      filled -= seg;
    }
    if (filled > 0) {
      int seg = (filled < hw) ? filled : hw;
      graphics_fill_rect(ctx, GRect(cx-gap-hw, 0, seg, t), 0, GCornerNone);
    }
  }
}
#endif

// ============================================================
// OUTER RING — ROUND
// ============================================================
#if defined(PBL_ROUND)
static void draw_ring_round(GContext *ctx, GRect bounds) {
  int step_pct = (s_settings.StepGoal > 0)
    ? (s_steps * 100 / s_settings.StepGoal) : 0;
  if (step_pct > 100) step_pct = 100;

  graphics_context_set_fill_color(ctx, eff_ring_dim());
  graphics_fill_radial(ctx, bounds, GOvalScaleModeFitCircle, RING_THICK,
                       DEG_TO_TRIGANGLE(3), DEG_TO_TRIGANGLE(177));
  if (s_battery > 0) {
    graphics_context_set_fill_color(ctx, eff_ring_batt());
    graphics_fill_radial(ctx, bounds, GOvalScaleModeFitCircle, RING_THICK,
                         DEG_TO_TRIGANGLE(177 - 174 * s_battery / 100),
                         DEG_TO_TRIGANGLE(177));
  }

  graphics_context_set_fill_color(ctx, eff_ring_dim());
  graphics_fill_radial(ctx, bounds, GOvalScaleModeFitCircle, RING_THICK,
                       DEG_TO_TRIGANGLE(183), DEG_TO_TRIGANGLE(357));
  if (step_pct > 0) {
    graphics_context_set_fill_color(ctx, eff_ring_steps());
    graphics_fill_radial(ctx, bounds, GOvalScaleModeFitCircle, RING_THICK,
                         DEG_TO_TRIGANGLE(183),
                         DEG_TO_TRIGANGLE(183 + 174 * step_pct / 100));
  }
}
#endif

// ============================================================
// DRAW BAR WITH SEPARATOR TICKS
// Draws dim track, lit fill, then bg-colored lines across the
// full bar width at each tick position — visible as separators
// through both the dim and lit regions.
// ============================================================
static void draw_bar_with_ticks(GContext *ctx,
                                int bar_x, int bar_top, int bar_w, int bar_h,
                                int lit_px,
                                int *tick_ys, int tick_count) {
  graphics_context_set_fill_color(ctx, eff_dim());
  graphics_fill_rect(ctx, GRect(bar_x, bar_top, bar_w, bar_h), 0, GCornerNone);

  if (lit_px > 0) {
    graphics_context_set_fill_color(ctx, eff_lit());
    graphics_fill_rect(ctx, GRect(bar_x, bar_top + bar_h - lit_px, bar_w, lit_px), 0, GCornerNone);
  }

  graphics_context_set_stroke_color(ctx, eff_bg());
  graphics_context_set_stroke_width(ctx, 1);
  for (int i = 0; i < tick_count; i++) {
    graphics_draw_line(ctx, GPoint(bar_x, tick_ys[i]), GPoint(bar_x + bar_w - 1, tick_ys[i]));
  }
}

// ============================================================
// DRAW — RECT PLATFORMS
// Layout matches v1: bars hug opposite screen edges.
// Left bar at left edge: [ring] [label] [tick] [BAR]
// Right bar at right edge:      [BAR] [tick] [label] [ring]
// Large open gap in the center between bars.
// ============================================================
#if !defined(PBL_ROUND)
static void draw_rect(GContext *ctx, GRect bounds) {
  int w = bounds.size.w;
  int h = bounds.size.h;

  bool show_ring = s_settings.ShowRing;
  int ri = show_ring ? (RING_THICK + RING_GAP) : 0;  // ring inset all sides

  // Vertical extents
  int date_y     = h - ri - DATE_HEIGHT;
  int bar_top    = ri + BAR_MARGIN_TOP;
  int bar_bottom = date_y - BAR_MARGIN_BOTTOM;
  int bar_h      = bar_bottom - bar_top;
  if (bar_h < 10) bar_h = 10;

  // Horizontal positions — left bar hugs left, right bar hugs right
  int lbar_x  = ri + LABEL_W + TICK_LEN;          // left bar left edge
  int ltick_x = ri + LABEL_W;                      // left tick right edge (butts bar)
  int lbl_l_x = ri;                                // left label left edge

  int rbar_x  = w - ri - LABEL_W - TICK_LEN - BAR_W; // right bar left edge
  int rtick_x = rbar_x + BAR_W;                       // right tick left edge
  int lbl_r_x = rtick_x + TICK_LEN;                   // right label left edge

  bool is_24h = s_settings.Show24h;
  int total_h = is_24h ? 24 : 12;

  // ---- Hour bar ----
  int hour_px;
  if (is_24h) {
    hour_px = (s_hour == 0) ? 0 : (bar_h * s_hour / 23);
  } else {
    int disp = s_hour % 12;
    if (disp == 0) disp = 12;
    hour_px = bar_h * disp / 12;
  }

  int hour_ticks[24];
  for (int hr = 1; hr <= total_h; hr++) {
    hour_ticks[hr-1] = is_24h
      ? (bar_bottom - bar_h * hr / 23)
      : (bar_bottom - bar_h * hr / 12);
  }

  draw_bar_with_ticks(ctx, lbar_x, bar_top, BAR_W, bar_h, hour_px, hour_ticks, total_h);

  // 24h AM/PM notch — slightly thicker to stand out
  if (is_24h) {
    int notch_y = bar_bottom - bar_h / 2;
    graphics_context_set_stroke_color(ctx, eff_bg());
    graphics_context_set_stroke_width(ctx, 2);
    graphics_draw_line(ctx, GPoint(lbar_x, notch_y), GPoint(lbar_x + BAR_W - 1, notch_y));
  }

  // Hour labels and ticks (left of bar)
  graphics_context_set_stroke_color(ctx, eff_text());
  graphics_context_set_stroke_width(ctx, 1);
  graphics_context_set_text_color(ctx, eff_text());
  for (int hr = 1; hr <= total_h; hr++) {
    int ty = hour_ticks[hr-1];
    graphics_draw_line(ctx, GPoint(ltick_x, ty), GPoint(lbar_x - 1, ty));
    if (!is_24h || (hr % 2 == 0)) {
      static char lbl[4];
      snprintf(lbl, sizeof(lbl), is_24h ? "%d" : "%02d", hr);
      graphics_draw_text(ctx, lbl,
        fonts_get_system_font(FONT_KEY_GOTHIC_14),
        GRect(lbl_l_x, ty - 8, LABEL_W, 16),
        GTextOverflowModeFill, GTextAlignmentRight, NULL);
    }
  }

  // ---- Minute bar ----
  int min_px = (s_minute == 0) ? 0 : (bar_h * s_minute / 59);

  int min_ticks[12];
  for (int i = 0; i < 12; i++) {
    int mn_src = ((i + 1) * 5 == 60) ? 59 : (i + 1) * 5;
    min_ticks[i] = bar_bottom - bar_h * mn_src / 59;
  }

  draw_bar_with_ticks(ctx, rbar_x, bar_top, BAR_W, bar_h, min_px, min_ticks, 12);

  // Minute labels and ticks (right of bar)
  graphics_context_set_stroke_color(ctx, eff_text());
  graphics_context_set_text_color(ctx, eff_text());
  for (int i = 0; i < 12; i++) {
    int mn = (i + 1) * 5;
    int ty = min_ticks[i];
    graphics_draw_line(ctx, GPoint(rtick_x, ty), GPoint(rtick_x + TICK_LEN - 1, ty));
    static char lbl[4];
    snprintf(lbl, sizeof(lbl), "%d", mn);
    graphics_draw_text(ctx, lbl,
      fonts_get_system_font(FONT_KEY_GOTHIC_14),
      GRect(lbl_r_x, ty - 8, LABEL_W, 16),
      GTextOverflowModeFill, GTextAlignmentLeft, NULL);
  }

  // ---- Date ----
  graphics_context_set_text_color(ctx, eff_text());
  graphics_draw_text(ctx, s_date_buf,
    fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
    GRect(0, date_y, w, DATE_HEIGHT + 4),
    GTextOverflowModeFill, GTextAlignmentCenter, NULL);

  if (show_ring) draw_ring_rect(ctx, w, h);
}
#endif

// ============================================================
// DRAW — ROUND PLATFORMS
// Two centered bars with ticks extending to circle boundary.
// Labels near circle edge. Date below bars.
// ============================================================
#if defined(PBL_ROUND)
static void draw_round(GContext *ctx, GRect bounds) {
  int w  = bounds.size.w;
  int h  = bounds.size.h;
  int cx = w / 2;
  int cy = h / 2;
  int r  = (w < h ? w : h) / 2;

  bool show_ring = s_settings.ShowRing;
  int cr = r - (show_ring ? (RING_THICK + RING_GAP) : 0);

  int usable_bottom = cy + cr - 6;
  int usable_top    = cy - cr + 6;
  int date_y        = usable_bottom - ROUND_DATE_H;
  int bar_top       = usable_top;
  int bar_bottom    = date_y - ROUND_DATE_GAP;
  int bar_h         = bar_bottom - bar_top;
  if (bar_h < 10) bar_h = 10;

  int lbar_x = cx - ROUND_BAR_GAP / 2 - ROUND_BAR_W;
  int rbar_x = cx + ROUND_BAR_GAP / 2;

  bool is_24h = s_settings.Show24h;
  int total_h = is_24h ? 24 : 12;

  // ---- Hour bar ----
  int hour_px;
  if (is_24h) {
    hour_px = (s_hour == 0) ? 0 : (bar_h * s_hour / 23);
  } else {
    int disp = s_hour % 12;
    if (disp == 0) disp = 12;
    hour_px = bar_h * disp / 12;
  }

  int hour_ticks[24];
  for (int hr = 1; hr <= total_h; hr++) {
    hour_ticks[hr-1] = is_24h
      ? (bar_bottom - bar_h * hr / 23)
      : (bar_bottom - bar_h * hr / 12);
  }

  draw_bar_with_ticks(ctx, lbar_x, bar_top, ROUND_BAR_W, bar_h, hour_px, hour_ticks, total_h);

  if (is_24h) {
    int notch_y = bar_bottom - bar_h / 2;
    graphics_context_set_stroke_color(ctx, eff_bg());
    graphics_context_set_stroke_width(ctx, 2);
    graphics_draw_line(ctx, GPoint(lbar_x, notch_y), GPoint(lbar_x + ROUND_BAR_W - 1, notch_y));
  }

  // ---- Minute bar ----
  int min_px = (s_minute == 0) ? 0 : (bar_h * s_minute / 59);

  int min_ticks[12];
  for (int i = 0; i < 12; i++) {
    int mn_src = ((i + 1) * 5 == 60) ? 59 : (i + 1) * 5;
    min_ticks[i] = bar_bottom - bar_h * mn_src / 59;
  }

  draw_bar_with_ticks(ctx, rbar_x, bar_top, ROUND_BAR_W, bar_h, min_px, min_ticks, 12);

  // ---- Ticks and labels toward circle edge ----
  graphics_context_set_stroke_width(ctx, 1);
  graphics_context_set_text_color(ctx, eff_text());

  for (int hr = 1; hr <= total_h; hr++) {
    int ty = hour_ticks[hr-1];
    int dy = ty - cy;
    int cr2 = cr * cr - dy * dy;
    if (cr2 < 0) continue;
    int hw = prv_isqrt(cr2);
    int tick_end = cx - hw + ROUND_TICK_MARGIN;
    if (tick_end < lbar_x - 1) {
      graphics_context_set_stroke_color(ctx, eff_text());
      graphics_draw_line(ctx, GPoint(tick_end, ty), GPoint(lbar_x - 1, ty));
    }
    if (!is_24h || (hr % 2 == 0)) {
      int lbl_cx = cx - hw + ROUND_LABEL_MARGIN;
      static char lbl[4];
      snprintf(lbl, sizeof(lbl), is_24h ? "%d" : "%02d", hr);
      graphics_draw_text(ctx, lbl,
        fonts_get_system_font(FONT_KEY_GOTHIC_14),
        GRect(lbl_cx - 13, ty - 8, 26, 16),
        GTextOverflowModeFill, GTextAlignmentCenter, NULL);
    }
  }

  for (int i = 0; i < 12; i++) {
    int ty = min_ticks[i];
    int dy = ty - cy;
    int cr2 = cr * cr - dy * dy;
    if (cr2 < 0) continue;
    int hw = prv_isqrt(cr2);
    int tick_end = cx + hw - ROUND_TICK_MARGIN;
    if (tick_end > rbar_x + ROUND_BAR_W) {
      graphics_context_set_stroke_color(ctx, eff_text());
      graphics_draw_line(ctx, GPoint(rbar_x + ROUND_BAR_W, ty), GPoint(tick_end, ty));
    }
    int mn = (i + 1) * 5;
    int lbl_cx = cx + hw - ROUND_LABEL_MARGIN;
    static char lbl[4];
    snprintf(lbl, sizeof(lbl), "%d", mn);
    graphics_draw_text(ctx, lbl,
      fonts_get_system_font(FONT_KEY_GOTHIC_14),
      GRect(lbl_cx - 13, ty - 8, 26, 16),
      GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  }

  // ---- Date ----
  graphics_context_set_text_color(ctx, eff_text());
  graphics_draw_text(ctx, s_date_buf,
    fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
    GRect(0, date_y, w, ROUND_DATE_H + 4),
    GTextOverflowModeFill, GTextAlignmentCenter, NULL);

  if (show_ring) draw_ring_round(ctx, bounds);
}
#endif

// ============================================================
// MAIN DRAW CALLBACK
// ============================================================
static void draw_layer(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_unobstructed_bounds(layer);
  graphics_context_set_fill_color(ctx, eff_bg());
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  graphics_context_set_stroke_width(ctx, 1);
#if defined(PBL_ROUND)
  draw_round(ctx, bounds);
#else
  draw_rect(ctx, bounds);
#endif
}

// ============================================================
// EVENT HANDLERS
// ============================================================
static void tick_handler(struct tm *t, TimeUnits units_changed) {
  s_hour   = t->tm_hour;
  s_minute = t->tm_min;
  strftime(s_date_buf, sizeof(s_date_buf), "%a %b %e", t);
  layer_mark_dirty(s_canvas_layer);
}

static void battery_handler(BatteryChargeState state) {
  s_battery = state.charge_percent;
  layer_mark_dirty(s_canvas_layer);
}

#if defined(PBL_HEALTH)
static void update_steps(void) {
  HealthServiceAccessibilityMask mask = health_service_metric_accessible(
    HealthMetricStepCount, time_start_of_today(), time(NULL));
  s_steps = (mask & HealthServiceAccessibilityMaskAvailable)
    ? (int)health_service_sum_today(HealthMetricStepCount) : 0;
  layer_mark_dirty(s_canvas_layer);
}
static void health_handler(HealthEventType event, void *context) {
  if (event == HealthEventMovementUpdate) update_steps();
}
#endif

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
  window_set_window_handlers(s_window, (WindowHandlers){
    .load   = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);

  time_t now = time(NULL);
  tick_handler(localtime(&now), MINUTE_UNIT | DAY_UNIT);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(battery_handler);
  battery_handler(battery_state_service_peek());
#if defined(PBL_HEALTH)
  health_service_events_subscribe(health_handler, NULL);
  update_steps();
#endif
  app_message_register_inbox_received(inbox_received);
  app_message_open(256, 64);
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
#if defined(PBL_HEALTH)
  health_service_events_unsubscribe();
#endif
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

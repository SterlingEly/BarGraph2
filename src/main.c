#include <pebble.h>

// ============================================================
// CONSTANTS
// ============================================================
#define SETTINGS_KEY        1
#define DEFAULT_STEP_GOAL   10000
#define RING_THICK          5     // outer ring thickness px
#define RING_GAP            3     // gap between ring and bars
#define DATE_HEIGHT         22    // px reserved at bottom for date (rect only)
#define BAR_MARGIN_TOP      6     // px from top edge to bar top (rect)
#define BAR_MARGIN_BOTTOM   4     // px from date strip to bar bottom (rect)
#define BAR_WIDTH           20    // px width of each bar (rect)
#define BAR_GAP             12    // px gap between the two bars (rect)
#define TICK_LEN            5     // px length of tick marks (rect)
#define LABEL_W             26    // px width of label column (rect)

// Round-specific
// Bars are radial spokes emanating from a central hub.
// Hours on bottom semicircle, minutes on top.
// Labels ride the outer edge of the spoke field.
#define SPOKE_INNER_R       28    // px from center where spoke starts
#define SPOKE_OUTER_INSET   30    // px short of screen edge where spoke ends (inside label ring)
#define SPOKE_LABEL_INSET   14    // px from edge for label center

// ============================================================
// SETTINGS
// ============================================================
typedef struct {
  GColor BackgroundColor;
  GColor BarLitColor;      // filled portion of both bars
  GColor BarDimColor;      // empty track of both bars
  GColor DateTextColor;
  GColor RingBattColor;    // ring lit: battery
  GColor RingStepsColor;   // ring lit: steps
  GColor RingDimColor;     // ring empty track
  int  StepGoal;
  bool ShowRing;
  bool InvertBW;           // B&W only
  bool Show24h;            // show AM/PM notch on rect hour bar
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
// HELPERS — effective colors (B&W invert handled here)
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
// DRAW — RECT PLATFORMS
// ============================================================
#if !defined(PBL_ROUND)
static void draw_rect(GContext *ctx, GRect bounds) {
  int w = bounds.size.w;
  int h = bounds.size.h;

  bool show_ring = s_settings.ShowRing;
  int ring_inset = show_ring ? (RING_THICK + RING_GAP) : 0;

  int date_y     = h - DATE_HEIGHT;
  int bar_top    = ring_inset + BAR_MARGIN_TOP;
  int bar_bottom = date_y - BAR_MARGIN_BOTTOM;
  int bar_h      = bar_bottom - bar_top;
  if (bar_h < 10) bar_h = 10;

  int assembly_w = LABEL_W + TICK_LEN + BAR_WIDTH + BAR_GAP + BAR_WIDTH + TICK_LEN + LABEL_W;
  int asm_x      = (w - assembly_w) / 2;
  int lbl_l_x    = asm_x;
  int ltick_x    = lbl_l_x + LABEL_W;
  int lbar_x     = ltick_x + TICK_LEN;
  int rbar_x     = lbar_x + BAR_WIDTH + BAR_GAP;
  int rtick_x    = rbar_x + BAR_WIDTH;
  int lbl_r_x    = rtick_x + TICK_LEN;

  // ---- Hour bar ----
  int disp_hour = s_hour % 12;
  if (disp_hour == 0) disp_hour = 12;
  int hour_px = bar_h * disp_hour / 12;

  // Empty track (dim color shows full bar ghost)
  graphics_context_set_fill_color(ctx, eff_dim());
  graphics_fill_rect(ctx, GRect(lbar_x, bar_top, BAR_WIDTH, bar_h), 0, GCornerNone);

  // Filled portion (grows upward)
  if (hour_px > 0) {
    graphics_context_set_fill_color(ctx, eff_lit());
    graphics_fill_rect(ctx, GRect(lbar_x, bar_bottom - hour_px, BAR_WIDTH, hour_px), 0, GCornerNone);
  }

  // Optional AM/PM notch: 1px bg-colored line at midpoint of bar
  if (s_settings.Show24h) {
    int notch_y = bar_bottom - bar_h / 2;
    graphics_context_set_stroke_color(ctx, eff_bg());
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_line(ctx, GPoint(lbar_x, notch_y), GPoint(lbar_x + BAR_WIDTH - 1, notch_y));
  }

  // Ticks and labels — hours left side
  graphics_context_set_stroke_color(ctx, eff_text());
  graphics_context_set_stroke_width(ctx, 1);
  graphics_context_set_text_color(ctx, eff_text());
  for (int hr = 1; hr <= 12; hr++) {
    int ty = bar_bottom - bar_h * hr / 12;
    graphics_draw_line(ctx, GPoint(ltick_x, ty), GPoint(lbar_x - 1, ty));
    static char lbl[4];
    snprintf(lbl, sizeof(lbl), "%02d", hr);
    graphics_draw_text(ctx, lbl,
      fonts_get_system_font(FONT_KEY_GOTHIC_14),
      GRect(lbl_l_x, ty - 8, LABEL_W, 16),
      GTextOverflowModeFill, GTextAlignmentRight, NULL);
  }

  // ---- Minute bar ----
  int min_px = (s_minute == 0) ? 0 : (bar_h * s_minute / 59);

  graphics_context_set_fill_color(ctx, eff_dim());
  graphics_fill_rect(ctx, GRect(rbar_x, bar_top, BAR_WIDTH, bar_h), 0, GCornerNone);

  if (min_px > 0) {
    graphics_context_set_fill_color(ctx, eff_lit());
    graphics_fill_rect(ctx, GRect(rbar_x, bar_bottom - min_px, BAR_WIDTH, min_px), 0, GCornerNone);
  }

  // Ticks and labels — minutes right side, every 5
  graphics_context_set_stroke_color(ctx, eff_text());
  graphics_context_set_text_color(ctx, eff_text());
  for (int mn = 5; mn <= 60; mn += 5) {
    int mn_src = (mn == 60) ? 59 : mn;
    int ty = bar_bottom - bar_h * mn_src / 59;
    graphics_draw_line(ctx, GPoint(rbar_x + BAR_WIDTH, ty), GPoint(rtick_x + TICK_LEN - 1, ty));
    static char lbl[4];
    snprintf(lbl, sizeof(lbl), "%d", mn);
    graphics_draw_text(ctx, lbl,
      fonts_get_system_font(FONT_KEY_GOTHIC_14),
      GRect(lbl_r_x, ty - 8, LABEL_W, 16),
      GTextOverflowModeFill, GTextAlignmentLeft, NULL);
  }

  // ---- Date text ----
  graphics_context_set_text_color(ctx, eff_text());
  graphics_draw_text(ctx, s_date_buf,
    fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
    GRect(0, date_y, w, DATE_HEIGHT + 4),
    GTextOverflowModeFill, GTextAlignmentCenter, NULL);

  // ---- Outer ring ----
  if (show_ring) {
    int step_pct = (s_settings.StepGoal > 0)
      ? (s_steps * 100 / s_settings.StepGoal) : 0;
    if (step_pct > 100) step_pct = 100;

    int t   = RING_THICK;
    int gap = 5;
    int cx  = w / 2;
    int hw  = cx - gap;
    int total = hw + h + hw;

    // Clear edge strips first
    graphics_context_set_fill_color(ctx, eff_bg());
    graphics_fill_rect(ctx, GRect(0,   0,   w, t), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(0,   h-t, w, t), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(0,   0,   t, h), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(w-t, 0,   t, h), 0, GCornerNone);

    // Battery empty track (right half, counterclockwise from bottom-center)
    graphics_context_set_fill_color(ctx, eff_ring_dim());
    graphics_fill_rect(ctx, GRect(cx+gap, h-t, hw, t), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(w-t,    0,   t,  h), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(cx+gap, 0,   hw, t), 0, GCornerNone);

    // Battery fill
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

    // Steps empty track (left half)
    graphics_context_set_fill_color(ctx, eff_ring_dim());
    graphics_fill_rect(ctx, GRect(0,         h-t, hw, t), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(0,         0,   t,  h), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(cx-gap-hw, 0,   hw, t), 0, GCornerNone);

    // Steps fill
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
}
#endif // !PBL_ROUND

// ============================================================
// DRAW — ROUND PLATFORMS
// ============================================================
// Hours: bottom semicircle (180°–360°), 12 spokes at 15° pitch (9° wide, 6° gap).
// Minutes: top semicircle (0°–180°), 60 spokes at 3° pitch (2° wide, 1° gap).
// All spokes emanate from a central hub, stopping short of a label ring at the edge.
// Date text lives in the center hub area.
#if defined(PBL_ROUND)
static void draw_round(GContext *ctx, GRect bounds) {
  int w  = bounds.size.w;
  int h  = bounds.size.h;
  int cx = w / 2;
  int cy = h / 2;
  int r  = (w < h ? w : h) / 2;

  bool show_ring  = s_settings.ShowRing;
  int  ring_inset = show_ring ? (RING_THICK + RING_GAP) : 0;
  int  spoke_outer = r - ring_inset - SPOKE_OUTER_INSET;
  int  spoke_inner = SPOKE_INNER_R;
  int  label_r     = r - ring_inset - SPOKE_LABEL_INSET;
  int  spoke_thick = spoke_outer - spoke_inner;

  GRect spoke_rect = GRect(cx - spoke_outer, cy - spoke_outer,
                           spoke_outer * 2,  spoke_outer * 2);

  // ---- Minutes — top semicircle 0°–180° ----
  // 60 spokes, 2° wide, 1° gap, 3° pitch. Spoke i at [1+3i, 1+3i+2).
  // Draw empty track first, then overlay filled.
  for (int i = 0; i < 60; i++) {
    int a_start = 1 + 3 * i;
    graphics_context_set_fill_color(ctx, (i < s_minute) ? eff_lit() : eff_dim());
    graphics_fill_radial(ctx, spoke_rect, GOvalScaleModeFitCircle, spoke_thick,
                         DEG_TO_TRIGANGLE(a_start), DEG_TO_TRIGANGLE(a_start + 2));
  }

  // ---- Hours — bottom semicircle 180°–360° ----
  // 12 spokes, 9° wide, 6° gap, 15° pitch. Spoke i at [183+15i, 183+15i+9).
  int disp_hour = s_hour % 12;
  if (disp_hour == 0) disp_hour = 12;

  for (int i = 0; i < 12; i++) {
    int a_start = 183 + 15 * i;
    graphics_context_set_fill_color(ctx, (i < disp_hour) ? eff_lit() : eff_dim());
    graphics_fill_radial(ctx, spoke_rect, GOvalScaleModeFitCircle, spoke_thick,
                         DEG_TO_TRIGANGLE(a_start), DEG_TO_TRIGANGLE(a_start + 9));
  }

  // ---- Hub — cleans up spoke roots ----
  graphics_context_set_fill_color(ctx, eff_bg());
  graphics_fill_circle(ctx, GPoint(cx, cy), spoke_inner);

  // ---- Minute labels: 5, 10... 60 around top edge ----
  graphics_context_set_text_color(ctx, eff_text());
  for (int mn = 5; mn <= 60; mn += 5) {
    // Midpoint angle of the spoke for this minute
    // Spoke mn-1 (0-indexed) is at 1 + 3*(mn-1) + 1 = 3*mn - 1 degrees
    int a_deg = 3 * mn - 1;
    if (mn == 60) a_deg = 178;  // clamp last label
    int32_t a = DEG_TO_TRIGANGLE(a_deg);
    int lx = cx + label_r * sin_lookup(a) / TRIG_MAX_RATIO;
    int ly = cy - label_r * cos_lookup(a) / TRIG_MAX_RATIO;
    static char lbl[4];
    snprintf(lbl, sizeof(lbl), "%d", mn);
    graphics_draw_text(ctx, lbl,
      fonts_get_system_font(FONT_KEY_GOTHIC_14),
      GRect(lx - 13, ly - 8, 26, 16),
      GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  }

  // ---- Hour labels: 1–12 around bottom edge ----
  for (int hr = 1; hr <= 12; hr++) {
    // Midpoint of spoke hr-1 (0-indexed): 183 + 15*(hr-1) + 4
    int a_deg = 183 + 15 * (hr - 1) + 4;
    int32_t a = DEG_TO_TRIGANGLE(a_deg);
    int lx = cx + label_r * sin_lookup(a) / TRIG_MAX_RATIO;
    int ly = cy - label_r * cos_lookup(a) / TRIG_MAX_RATIO;
    static char lbl[4];
    snprintf(lbl, sizeof(lbl), "%d", hr);
    graphics_draw_text(ctx, lbl,
      fonts_get_system_font(FONT_KEY_GOTHIC_14),
      GRect(lx - 13, ly - 8, 26, 16),
      GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  }

  // ---- Date: centered in hub ----
  graphics_context_set_text_color(ctx, eff_text());
  graphics_draw_text(ctx, s_date_buf,
    fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
    GRect(cx - 52, cy - 10, 104, 22),
    GTextOverflowModeFill, GTextAlignmentCenter, NULL);

  // ---- Outer ring: battery right, steps left ----
  if (show_ring) {
    int step_pct = (s_settings.StepGoal > 0)
      ? (s_steps * 100 / s_settings.StepGoal) : 0;
    if (step_pct > 100) step_pct = 100;

    // Battery: right semicircle (3°–177°), fills from bottom-right up
    graphics_context_set_fill_color(ctx, eff_ring_dim());
    graphics_fill_radial(ctx, bounds, GOvalScaleModeFitCircle, RING_THICK,
                         DEG_TO_TRIGANGLE(3), DEG_TO_TRIGANGLE(177));
    if (s_battery > 0) {
      graphics_context_set_fill_color(ctx, eff_ring_batt());
      graphics_fill_radial(ctx, bounds, GOvalScaleModeFitCircle, RING_THICK,
                           DEG_TO_TRIGANGLE(177 - 174 * s_battery / 100),
                           DEG_TO_TRIGANGLE(177));
    }

    // Steps: left semicircle (183°–357°), fills from bottom-left up
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
}
#endif // PBL_ROUND

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
  strftime(s_date_buf, sizeof(s_date_buf), "%a, %b %e", t);
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

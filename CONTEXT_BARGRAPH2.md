# Bar Graph 2 — Project Context

> **AI collaborators: read [`PROJECT_CONTEXT.md`](https://github.com/SterlingEly/Radium2/blob/master/PROJECT_CONTEXT.md) first** for the authoritative platform table, shared build rules, and role split. (Shared context lives in the Radium2 repo.)

---

## Purpose

Bar Graph 2 is a Pebble watchface displaying time as two vertical bar graphs. Left bar = hours (fills upward as the hour progresses), right bar = minutes. The original Bar Graph v1 was designed by Sterling Ely and implemented by Cameron MacFarland (distantcam) in 2013–2014. Bar Graph 2 is a from-scratch rebuild by Sterling Ely + Claude (2026), targeting all Pebble platforms with color, round-screen support, and a settings page — while exactly replicating the v1 edge-hugging layout aesthetic.

---

## Current Status

- **NOT YET SUBMITTED to app store**
- aplite/diorite/flint layout locked at v2.12
- GitHub repo: https://github.com/SterlingEly/BarGraph2 (branch: `master`)

### Development Plan
aplite/diorite/flint ✅ → emery (200×228) → basalt (color) → chalk/gabbro (round)

---

## Human / AI Role Split

**Sterling:** Design direction, v1 layout fidelity decisions, device testing, store submission.
**Claude:** C code, JavaScript config page, GitHub commits, documentation.

---

## Repository Structure

```
SterlingEly/BarGraph2 (master)
├── CONTEXT_BARGRAPH2.md   ← this file
└── src/
    ├── main.c             ← entire watchface (flat path, not src/c/)
    └── pkjs/
        ├── config.js
        └── index.js
```

---

## Build / Deployment Rules

- Source path: `src/main.c` (flat — not `src/c/main.c`)
- SETTINGS_KEY: 1
- See [PROJECT_CONTEXT.md](https://github.com/SterlingEly/Radium2/blob/master/PROJECT_CONTEXT.md) for shared build rules and GitHub MCP rules

---

## Architecture

### Core Design Principle (KEY)
**The bars hug opposite screen edges — NOT the center.** This is the defining v1 aesthetic. Labels and tick lines sit between the bar and the screen edge, with a large open gap in the middle.

```
Left bar:  [ring_inset] [LABEL_W=26] [TICK_LEN=5] [BAR_W=30] ··· (gap) ···
Right bar: ··· (gap) ··· [BAR_W=30] [TICK_LEN=5] [LABEL_W=26] [ring_inset]
```

### Rect Layout Constants (aplite-locked)
```c
#define DATE_HEIGHT       22
#define BAR_MARGIN_TOP    4
#define BAR_MARGIN_BOTTOM 4
#define BAR_W             30    // matches v1 measured width exactly
#define LABEL_W           26
#define TICK_LEN          5
#define RING_THICK        5
#define RING_GAP          3
// hx=38 (hour bar x), bw=30, mx=76 (minute bar x)
// bar_bot=132, slot_h=10px, 12 slots = 120px tall
```

### Round Layout Constants
```c
#define ROUND_BAR_W        24
#define ROUND_BAR_GAP      10
#define ROUND_TICK_MARGIN  28
#define ROUND_LABEL_MARGIN 14
#define ROUND_DATE_H       18
#define ROUND_DATE_GAP     4
```

### Bars Fill From Bottom
- Hour bar (left): bottom = :00, top = 12:00 or 24:00
- Minute bar (right): bottom = :00, top = :59

### Tick / Separator Lines
`draw_bar_with_ticks()` draws dim track → lit fill → bg-colored horizontal lines at tick positions. Separators are visible through both dim and lit regions simultaneously.
- Hour ticks: one per hour (12 or 24)
- Minute ticks: every 5 minutes (12 ticks: :05, :10, ..., :55)
- 24h mode: slightly thicker (2px) notch at AM/PM midpoint

### Labels
- Hour: left of bar (right-aligned); every hour 12h, every even hour 24h; font `GOTHIC_14`
- Minute: right of bar (left-aligned); every 5 minutes; font `GOTHIC_14`
- Leading zeroes on all labels (`%02d`)

### Date Line
`strftime(buf, "%a %b %e", t)` → e.g. "Thu Mar 12"; font `GOTHIC_18_BOLD`; `%e` suppresses leading zero on day.

### Round — Ticks to Circle Edge
Uses `prv_isqrt()` to compute horizontal chord at each tick's Y position, so tick lines extend cleanly to the circle boundary.

---

## Critical Constants / Message Keys

### Settings Struct (v2.0)
```c
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
} BarSettings;  // SETTINGS_KEY=1
```

### MessageKeys (11 total)
`BackgroundColor`(0) through `Show24h`(10)

### Fonts
- Bar labels: custom `FONT_ARIAL_10` — must be added via CloudPebble "Another Font" UI
- Date: custom `FONT_ARIAL_18` — same
- Both are registered from the same TTF at different sizes

---

## Known Bugs / Known Traps

- Bars must hug edges — centering them breaks the v1 aesthetic
- Custom Arial fonts must be added via CloudPebble UI, not appinfo.json
- Leading zeroes on all labels (`%02d`)

---

## Current TODO

1. Implement emery (200×228) layout
2. Implement basalt (color, 144×168)
3. Implement chalk/gabbro (round) layout
4. Config page polish
5. Store preparation (description, screenshots, banner)

---

## Verification Plan

Before store submission:
1. Verify bar positions match v1 screenshots on aplite (144×168)
2. Verify bars fill correctly in 12h and 24h mode
3. Verify separator lines visible in both lit and dim regions
4. Verify date format (no leading zero on day via `%e`)
5. Verify ring fills correctly on rect and round

---

## Source of Truth / External Links

| Resource | URL |
|----------|-----|
| GitHub repo | https://github.com/SterlingEly/BarGraph2 |
| v1 store (reference) | https://apps.repebble.com/en_US/application/5305a587b704cb4e7d0000e5 |

---

## Last Updated

2026-07-01 — aplite layout locked. Next: emery layout.

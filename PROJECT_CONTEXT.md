# Bar Graph 2 — Project Context

> **AI collaborators and technical contributors: read this file before touching any code.**
> It is the authoritative technical reference for this repository.

---

## Purpose

Bar Graph 2 is a Pebble watchface displaying time as two vertical bar graphs. Left bar = hours (fills upward, bottom = 00, top = 12 or 24). Right bar = minutes (bottom = 00, top = 60).

The original Bar Graph v1 was designed by Sterling Ely and implemented by Cameron MacFarland (distantcam) in 2013–2014. Bar Graph 2 is a from-scratch rebuild by Sterling Ely + Claude (2026), targeting all modern Pebble platforms with color, round-screen support, and a settings page — while faithfully replicating the v1 layout aesthetic and geometry.

**Core design tenet:** The v1 pixel geometry on 144×168 aplite is the canonical reference. All other platforms scale from it proportionally. Bars are symmetric and near-center, with labeled tick lines and nubs extending outward into the gutters on each side.

---

## Current Status

> **Live state — update this section as work progresses.**

- **NOT YET SUBMITTED to app store**
- `aplite / diorite / flint` (144×168, B&W) layout **locked at v2.12** ✅
- All other platforms pending
- Branch: `master`

### Development plan (in order)

| Step | Platform | Resolution | State |
|------|----------|------------|-------|
| 1 | aplite / diorite / flint | 144×168, B&W | ✅ Locked at v2.12 |
| 2 | emery | 200×228, color, hi-res | ⬜ Not started |
| 3 | basalt | 144×168, color | ⬜ Not started |
| 4 | chalk / gabbro | 180×180 / 260×260, round | ⬜ Not started |
| 5 | Config page | — | ⬜ Needs end-to-end verification |
| 6 | Store submission | — | ⬜ Not started |

---

## Human / AI Role Split

| Role | Responsibility |
|------|----------------|
| **Sterling Ely** | Design direction, v1 layout fidelity decisions, pixel-level review, device testing, store submission |
| **AI collaborator (Claude)** | C implementation, JS config page, GitHub commits, documentation |

Sterling makes all product and design decisions. AI proposes and implements; Sterling approves.

---

## Repository Structure

```
SterlingEly/BarGraph2 (master)
├── PROJECT_CONTEXT.md     ← authoritative technical reference (this file)
├── CONTEXT_BARGRAPH2.md   ← legacy stub; redirects here
├── README.md              ← human-facing overview
├── appinfo.json           ← message keys as appKeys{} object
└── src/
    ├── main.c             ← entire watchface C source
    └── pkjs/
        ├── config.js      ← self-contained HTML config page (no pebble-clay)
        └── index.js       ← PebbleKit JS: settings relay + localStorage persistence
```

**Critical path rule:** Source must live at `src/main.c` (flat). Do not use `src/c/main.c`. CloudPebble import fails if two `main.c` files exist at different paths.

---

## Build / Deployment Rules

> **Stable — these rules do not change between sessions.**

### CloudPebble

- `appinfo.json` must use `"appKeys": { "Key": N }` (key/value object). Do NOT use `"messageKeys": [...]` (array). The array form does not generate `MESSAGE_KEY_*` constants and causes undeclared identifier errors at compile time.
- Do NOT add a `"resources"` / `"media"` block to `appinfo.json` — causes "Unsupported published resource type" build error.
- Custom fonts must be added via the CloudPebble UI ("Another Font" button on the font resource), NOT via `appinfo.json` or GitHub import.
- Menu icons must be added via CloudPebble UI, not GitHub.
- Tilde (`~`) in resource filenames breaks CloudPebble GitHub import.
- CloudPebble re-import fails if duplicate source files exist at different paths (e.g. `src/main.c` and `src/c/main.c` both present).

### GitHub MCP

- `push_files` silently empties file content — **never use it**. Use `create_or_update_file` with full inline content.
- Always fetch a fresh SHA via `get_file_contents` before any `create_or_update_file` call — SHAs from prior sessions are stale after any intervening commit.
- `get_file_contents` times out on files above ~15KB.
- "Deleting" a file via MCP produces a zero-byte blob. Actual deletion requires the GitHub web UI trash icon.
- Release tags cannot be created via MCP — manual web UI step only.

### JS config

- No `pebble-clay` dependency. `config.js` generates a self-contained `data:text/html` URI.
- Settings persisted via `localStorage` with the `BG2_` key prefix.
- `index.js` parses the JSON response in `webviewclosed` directly (no Clay intermediary).

---

## Architecture

> **Stable — reflects v2.12 locked implementation.**

### V1 Reference Geometry (canonical — 144×168 aplite)

All rect layout derives from these v1 values, scaled to the current screen:

```
V1_HX      = 38    // hour bar left x
V1_BAR_W   = 30    // bar width (both bars, matching v1 exactly)
V1_MX      = 76    // minute bar left x  →  inter-bar gap = 76 - 38 - 30 = 8px
V1_BAR_TOP = 12    // bar area top y
V1_BAR_BOT = 132   // bar area bottom y
V1_SLOT_H  = 10    // px per slot on aplite (1 hour = 1 slot)
V1_DATE_Y  = 138   // date text top y
V1_BAR_H   = 120   // 12 slots × 10px
```

Scaling formula: `value = (V1_VALUE * screen_dim) / reference_dim`
Example: `hx = (38 * w) / 144`

**Bar positioning:** Both bars sit near screen center with symmetric 38px gutters on each side. This is not edge-hugging; it is the exact v1 geometry which places bars centrally, not at the edges.

### Tick Line Rendering (draw order matters)

Per slot boundary `ty = bar_bot - slot_h * i` (i = 1 to 11):

1. **Lit-color line across full bar width** — drawn first, visible on unfilled (black) background
2. **Nub lines** — 8px lit-color lines outside each bar edge (left nub for hour bar, right nub for minute bar; no line in inter-bar gap)
3. **Bar fill rect** — drawn over tick lines; covers lit line in filled region
4. **Bg-color 1px separator cut** — applied only when `ty > h_fill_top` (strictly inside fill). When `ty == h_fill_top`, the line stays lit — fill has not yet fully passed it.
5. **Labels** — drawn last, on top of everything

**Border lines:** Lit-color lines at `bar_top` (12/60 boundary) and `bar_bot` (00 boundary), with nubs.

### Fill Math

```c
hfill = show24 ? (bar_h * dh / 24) : (slot_h * dh);   // hour fill in px
mfill = (slot_h * s_minute) / 5;                        // minute fill in px
```

### Round Layout

`draw_round()` uses `prv_isqrt()` to compute the horizontal chord at each tick's Y, extending lines cleanly to the circle boundary. This path has a basic implementation but has not been tested or tuned. A full layout pass is needed.

### Date Format

`strftime(buf, "%a, %b %e", t)` → e.g. `"Tue, Apr  7"` (`%e` = space-padded day).

---

## Critical Constants / Message Keys

### appinfo.json appKeys (11 total)

```json
"appKeys": {
  "BackgroundColor": 0,
  "BarLitColor":     1,
  "BarDimColor":     2,
  "DateTextColor":   3,
  "RingBattColor":   4,
  "RingStepsColor":  5,
  "RingDimColor":    6,
  "StepGoal":        7,
  "ShowRing":        8,
  "InvertBW":        9,
  "Show24h":         10
}
```

### Settings Struct

```c
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
} BarSettings;  // SETTINGS_KEY = 1
```

### Font Resources

Registered in CloudPebble UI from `arial.ttf` (sourced from distantcam/Watchface-BarGraph). Add via "Another Font" button on the resource, not via appinfo.json.

| CloudPebble Identifier | Size | Use |
|------------------------|------|-----|
| `FONT_ARIAL_10` | 10px | Bar labels (hours + minutes) |
| `FONT_ARIAL_18` | 18px | Date line |

Code uses `#ifdef RESOURCE_ID_FONT_ARIAL_10` to fall back to system fonts if Arial is not registered. This allows building without the font registered, but the fallback (GOTHIC_14 / GOTHIC_18_BOLD) is visually different.

### Layout Constants (v2.12)

```c
#define V1_BAR_BOT  132
#define V1_BAR_TOP   12
#define V1_HX        38
#define V1_BAR_W     30
#define V1_MX        76
#define V1_DATE_Y   138
#define V1_SLOT_H    10
#define NUB_PX        8    // outer tick nub length (px, not scaled)
#define GAP_PX        8    // gap between nub end and label (px, not scaled)
```

---

## Known Bugs / Known Traps

- **Separator cut off-by-one** — the separator bg cut must use `ty > h_fill_top` (strictly greater than). Using `>=` causes the separator to appear one slot early, making the transition look wrong at the fill boundary. This is a subtle but visually significant bug.
- **`src/c/main.c` stray file** — during early development, a zero-byte `src/c/main.c` was left in the repo. CloudPebble sees both files and refuses to build. If this recurs, delete via GitHub web UI (MCP cannot truly delete files).
- **`push_files` silently empties files** — see Build / Deployment Rules.
- **`messageKeys` array form** — does not generate `MESSAGE_KEY_*` constants. Must use `appKeys` object form. This caused a build failure early in development.
- **Font resources not in appinfo.json** — if `RESOURCE_ID_FONT_ARIAL_10` is undeclared, the `#ifdef` falls back gracefully, but the visual result looks wrong. Register both fonts in CloudPebble UI.
- **Bars must not fill full screen height** — v1 bars are 120px tall (12 slots × 10px), bottom at y=132, not y=168. Filling to the screen edge is incorrect and breaks the v1 aesthetic.
- **CloudPebble re-import fails** if duplicate source files exist at different paths. Clean up stray files via GitHub web UI before re-importing.

---

## Current TODO

> **Live state — update as work progresses.**

1. ⬜ Emery (200×228) — test scaling math in simulator; verify geometry and font sizes at hi-res
2. ⬜ Basalt (144×168, color) — same geometry as aplite; add color defaults and verify
3. ⬜ Round layout (chalk 180×180, gabbro 260×260) — `draw_round()` needs full layout pass and simulator testing
4. ⬜ Config page — verify all color pickers, toggles, and step goal slider work end-to-end
5. ⬜ Dim track — v2 has `BarDimColor` in settings but it is not drawn; decide: implement as option, or keep v1-faithful no-dim default
6. ⬜ Battery/step ring — `ShowRing` is reserved but defaults to false; no ring is drawn; decide whether to implement before store release
7. ⬜ Store submission — description, screenshots, banner image

---

## Verification Plan

Before declaring each platform locked:

1. Screenshot at a known time and verify bar fill heights match expected slot count
2. Verify separator lines are visible in both filled and unfilled bar regions
3. Verify separator line stays lit at the exact fill boundary (does not flip one slot early)
4. Verify top (12/60) and bottom (00) border lines are present with nubs
5. Verify labels: leading zeroes (`%02d`), correct font, centered on tick lines, not clipped
6. Verify date format: `"Tue, Apr  7"` style (space-padded day, no comma after day number is wrong)

---

## Related Projects

Other Pebble watchface repositories by Sterling Ely:

| Repository | URL | Relationship |
|------------|-----|--------------|
| **Radium 2** | https://github.com/SterlingEly/Radium2 | Sibling watchface; radial time display; most feature-complete; released on Rebble store. Bar Graph 2 shares CloudPebble build patterns and JS config architecture with Radium 2. |
| **TallBoy** | https://github.com/SterlingEly/TallBoy | Sibling watchface; oversized vector-drawn digits; all-platform; advanced health, weather, and solar data. |
| **Monogram** | https://github.com/SterlingEly/Monogram | Sibling watchface; custom monogram-style digit assets; early development; targets gabbro (Pebble Round 2). |
| **Pixel Sampler** | https://github.com/SterlingEly/PixelSampler | Developer reference app; browser for Pebble system fonts, colors, and platform capabilities; released on Rebble store. |

**v1 original source (reference only):**

| Resource | URL |
|----------|-----|
| distantcam/Watchface-BarGraph | https://github.com/distantcam/Watchface-BarGraph |

Bar Graph 2 is forked from distantcam/Watchface-BarGraph. The v1 source was the basis for reverse-engineering the exact pixel geometry used in v2.

---

## Source of Truth / External Links

| Resource | URL |
|----------|-----|
| This repository | https://github.com/SterlingEly/BarGraph2 |
| v1 original source | https://github.com/distantcam/Watchface-BarGraph |
| v1 Repebble store | https://apps.repebble.com/en_US/application/5305a587b704cb4e7d0000e5 |
| v1 Rebble store | https://apps.rebble.io/en_US/application/5305a587b704cb4e7d0000e5 |
| CloudPebble | https://cloudpebble.repebble.com |
| Arial font source | https://github.com/distantcam/Watchface-BarGraph/blob/master/resources/fonts/arial.ttf |
| Open-Meteo API (weather, if added) | https://open-meteo.com |

---

## Unresolved Questions

- **Emery font sizing** — `FONT_ARIAL_10` and `FONT_ARIAL_18` may be too small at 200×228. Larger variants may be needed. Verify in simulator.
- **Dim track** — v1 had no dim track (unfilled = black background). v2 has `BarDimColor` in settings but it is not drawn. Decision pending: implement as a configurable option, or preserve v1-faithful no-dim default?
- **Battery/step ring** — settings keys are reserved, drawing code is stubbed, but `ShowRing` defaults to false. Decision pending: implement before store release, or release without ring and add later?
- **Round layout** — `draw_round()` exists but is untested. Is the centered-bar approach correct for chalk/gabbro, or does the round layout need a different geometry?

---

## Last Updated

2026-07-03 — Standardized to repository documentation standard. Aplite/diorite/flint locked at v2.12. Next: emery layout pass.

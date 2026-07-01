# Bar Graph 2 — Project Context

> **AI collaborators: read this file before touching any code.**
> It is the authoritative session seed for Bar Graph 2.

---

## Purpose

Bar Graph 2 is a Pebble watchface displaying time as two vertical bar graphs. Left bar = hours (fills upward as the hour progresses, bottom = :00, top = 12:00 or 24:00). Right bar = minutes (bottom = :00, top = :59).

The original Bar Graph v1 was designed by Sterling Ely and implemented by Cameron MacFarland (distantcam) in 2013–2014. Bar Graph 2 is a from-scratch rebuild by Sterling Ely + Claude (2026), targeting all modern Pebble platforms with color, round-screen support, and a settings page — while exactly replicating the v1 layout aesthetic.

**Core design tenet:** The bars are positioned as in v1 — anchored near the center of the screen, symmetric, with labeled tick lines extending outward into the gutters. The exact v1 pixel geometry on 144×168 is the canonical reference; all other platforms scale from it.

---

## Current Status

- **NOT YET SUBMITTED to app store**
- `aplite / diorite / flint` layout **locked at v2.12** ✅
- GitHub repo: https://github.com/SterlingEly/BarGraph2 (branch: `master`)
- HEAD commit: v2.12 — aplite layout locked

### Development Plan (in order)
1. ✅ aplite / diorite / flint (144×168, B&W)
2. ⬜ emery (200×228, color, hi-res)
3. ⬜ basalt (144×168, color)
4. ⬜ chalk / gabbro (round, 180×180 / 260×260)
5. ⬜ Config page polish
6. ⬜ Store submission

---

## Human / AI Role Split

| Role | Responsibility |
|------|----------------|
| **Sterling Ely** | Design direction, v1 layout fidelity decisions, pixel-level review, device testing, store submission |
| **AI collaborator (Claude)** | C implementation, JS config page, GitHub commits, documentation |

Sterling makes all product/design calls. AI proposes and implements; Sterling approves.

---

## Repository Structure

```
SterlingEly/BarGraph2 (master)
├── PROJECT_CONTEXT.md     ← AI session seed (this file)
├── CONTEXT_BARGRAPH2.md   ← legacy; superseded by this file
├── README.md
├── appinfo.json           ← message keys as appKeys{} object (NOT messageKeys array)
└── src/
    ├── main.c             ← entire watchface C source (flat path — not src/c/main.c)
    └── pkjs/
        ├── config.js      ← self-contained HTML config page (no pebble-clay dependency)
        └── index.js       ← PebbleKit JS: settings relay + localStorage persistence
```

---

## Build / Deployment Rules

**CloudPebble-specific (critical):**
- Source must be at `src/main.c` — NOT `src/c/main.c`. CloudPebble import fails if two main.c files exist at different paths.
- `appinfo.json` must use `appKeys: { "Key": N }` (key/value object) — NOT `messageKeys: [...]` (array). The array form does not generate `MESSAGE_KEY_*` constants.
- Do NOT add a `resources/media` block to `appinfo.json` — causes "Unsupported published resource type" build error.
- Custom fonts must be added via CloudPebble UI ("Another Font" button), NOT via appinfo.json or GitHub import.
- Menu icons must be added via CloudPebble UI, not GitHub.
- Tilde (`~`) in resource filenames breaks CloudPebble GitHub import.

**GitHub MCP rules:**
- `push_files` silently empties files — NEVER USE IT. Use `create_or_update_file` with inline content only.
- Always fetch current SHA via `get_file_contents` before any update (prior session SHAs are stale).
- `get_file_contents` times out on files above ~15KB — use bash + GitHub API instead.
- Cannot create release tags via MCP — manual web UI step only.
- "Deleting" a file via MCP produces a zero-byte blob — real deletion requires GitHub web UI.

**JS config:**
- No `pebble-clay` dependency — config.js generates a self-contained `data:text/html` URI.
- Settings persisted via `localStorage` with `BG2_` prefix.

---

## Architecture

### V1 Reference Geometry (canonical — 144×168 aplite)

All rect layout is derived by scaling these v1 pixel values to the current screen size:

```
V1_HX      = 38    // hour bar left x
V1_BAR_W   = 30    // bar width (both bars)
V1_MX      = 76    // minute bar left x  (gap between bars = 76-38-30 = 8px)
V1_BAR_TOP = 12    // bar area top y
V1_BAR_BOT = 132   // bar area bottom y
V1_SLOT_H  = 10    // px per slot (1 hour = 1 slot = 10px on aplite)
V1_DATE_Y  = 138   // date text top y
V1_BAR_H   = 120   // total bar height = 12 slots × 10px
V1_GUTTER  = 38    // symmetric gutter width on each side
```

Scaling formula: `scaled = (V1_VALUE * screen_dim) / reference_dim`
e.g. `hx = (38 * w) / 144`

### Tick Line Rendering (draw order matters)

Per tick position `ty = bar_bot - slot_h * i`:

1. **Lit-color line across full bar width** — visible on unfilled (black) background
2. **Bar fill rect** — covers tick line in filled region
3. **Bg-color 1px separator cut** — BUT ONLY when `ty > h_fill_top` (strictly inside fill). When `ty == h_fill_top`, fill has just barely reached the line — no cut, line stays lit color.
4. **Labels drawn last** — on top of everything

Plus outer **nubs**: 8px lit-color line outside each bar edge (not in inter-bar gap), then 8px gap, then labels.

**Border lines:** lit-color lines at `bar_top` (the 12/60 line) and `bar_bot` (the 00 line) with nubs.

### Fill Math

```c
hfill = show24 ? (bar_h * dh / 24) : (slot_h * dh);   // hour fill in px
mfill = (slot_h * s_minute) / 5;                        // minute fill in px
```

### Round Layout

Uses `prv_isqrt()` to compute chord width at each tick's Y to extend lines cleanly to the circle boundary. Parameters in `ROUND_*` constants (not yet fully tuned — round layout is still pending).

### Date Format

`strftime(buf, "%a, %b %e", t)` → e.g. `"Tue, Apr  7"` (`%e` = space-padded day, no leading zero).

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

### Font Resources (registered in CloudPebble UI)

| Identifier | Size | Use |
|------------|------|-----|
| `FONT_ARIAL_10` | 10px | Bar labels (hours + minutes) |
| `FONT_ARIAL_18` | 18px | Date line |

Both registered from the same `arial.ttf` file (sourced from distantcam/Watchface-BarGraph) using "Another Font" in CloudPebble. Code uses `#ifdef RESOURCE_ID_FONT_ARIAL_10` to fall back to system fonts if not registered.

### Layout Constants (active in v2.12)

```c
#define V1_BAR_BOT  132
#define V1_BAR_TOP   12
#define V1_HX        38
#define V1_BAR_W     30
#define V1_MX        76
#define V1_DATE_Y   138
#define V1_SLOT_H    10
#define NUB_PX        8    // outer tick nub length
#define GAP_PX        8    // gap between nub end and label
```

---

## Known Bugs / Known Traps

- **`src/c/main.c` stray file** — at some point during development a zero-byte file was left at `src/c/main.c`. CloudPebble sees both `src/main.c` and `src/c/main.c` and fails to build. If this recurs, delete via GitHub web UI (MCP cannot truly delete files).
- **Separator cut off-by-one** — separator must use `ty > h_fill_top` (strictly greater), not `>=`. At exactly `ty == h_fill_top`, the fill hasn't fully covered the line yet; cutting it makes the transition appear one slot early.
- **`push_files` silently empties content** — see Build Rules above.
- **Font resources not in appinfo.json** — must be added via CloudPebble UI. If `RESOURCE_ID_FONT_ARIAL_10` is undeclared at build time, the `#ifdef` falls back to GOTHIC_14 automatically.
- **`messageKeys` array form** — does not generate `MESSAGE_KEY_*` constants. Must use `appKeys` object form in appinfo.json.
- **CloudPebble re-import fails** if duplicate source files exist at different paths.
- **Bars must not extend full screen height** — v1 bars are 120px (12 slots × 10px), not full screen height. The bottom is at y=132, not y=168.

---

## Current TODO

1. ⬜ Test emery simulator (200×228) — verify scaling math produces correct geometry
2. ⬜ Basalt (144×168 color) — same geometry as aplite, add color defaults
3. ⬜ Round layout (chalk 180×180, gabbro 260×260) — `draw_round()` needs full layout pass
4. ⬜ Config page — verify all color pickers and toggles work end-to-end
5. ⬜ Optional: dim track (BarDimColor) as visible grey on B&W and color watches
6. ⬜ Optional: battery/step ring (ShowRing) — currently disabled (`ShowRing = false` default)
7. ⬜ Store submission: description, screenshots, banner image

---

## Verification Plan

Before each platform is declared locked:
1. Screenshot at a known time (e.g. 10:17) and compare bar fill heights to expected slot count
2. Verify separator lines visible in both filled and unfilled bar regions
3. Verify separator line stays lit color at the exact fill boundary (doesn't flip one slot early)
4. Verify top (12/60) and bottom (00) border lines present with nubs
5. Verify labels: leading zeroes, correct font, centered on tick lines, not clipped
6. Verify date format: `"Tue, Apr  7"` style (space-padded day, Arial 18)

---

## Source of Truth / External Links

| Resource | URL |
|----------|-----|
| GitHub repo | https://github.com/SterlingEly/BarGraph2 |
| v1 original (reference) | https://github.com/distantcam/Watchface-BarGraph |
| v1 app store | https://apps.repebble.com/en_US/application/5305a587b704cb4e7d0000e5 |
| Rebble app store | https://apps.rebble.io/en_US/application/5305a587b704cb4e7d0000e5 |
| CloudPebble | https://cloudpebble.repebble.com |
| Arial font source | https://github.com/distantcam/Watchface-BarGraph/blob/master/resources/fonts/arial.ttf |

---

## Unresolved Questions

- Round layout (chalk/gabbro): `draw_round()` in main.c has a basic implementation but has not been tested or tuned. Full layout pass needed.
- Emery scaling: font sizes (FONT_ARIAL_10 for labels, FONT_ARIAL_18 for date) may need larger variants on the 200×228 hi-res screen.
- Dim track: v1 had no dim track (unfilled bar = black). v2 has `BarDimColor` in settings but it is not currently drawn. Decision pending: add as option, or keep v1-faithful default of no dim track?
- Battery/step ring: settings keys reserved, drawing code stubbed, but `ShowRing` defaults to false and no ring is drawn. Pending decision on whether to implement for v2 store release.

---

## Last Updated

2026-07-01 — aplite/diorite/flint layout locked at v2.12. Next: emery layout pass.

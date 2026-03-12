# BAR GRAPH 2 — CONTEXT SEED FOR NEW THREAD
*Everything a fresh Claude session needs to resume Bar Graph 2 development*

---

## 1. WHAT IS THIS PROJECT?

**Bar Graph 2** is a Pebble watchface displaying time as two vertical bar graphs — left bar = hours (fills up toward current hour), right bar = minutes. The original Bar Graph (v1) was designed by Sterling Ely, implemented by Cameron MacFarland (distantcam) in 2013–2014. Bar Graph 2 is a from-scratch rebuild by Sterling Ely + Claude (2026) for all modern Pebble platforms, replicating v1's exact layout/aesthetic while adding color, round-screen support, and a settings page.

**Sterling's role:** Design/concept lead, pixel archaeologist for v1 layout fidelity.
**Claude's role:** Technical implementation partner.

---

## 2. HISTORY & ATTRIBUTION

| Project | Year | Designer | Developer |
|---------|------|----------|-----------|
| Bar Graph v1 | 2013–2014 | Sterling Ely | Cameron MacFarland (distantcam) |
| Bar Graph 2 | 2026 | Sterling Ely | Sterling Ely + Claude |

**Original Bar Graph v1 app store:** https://apps.repebble.com/en_US/application/5305a587b704cb4e7d0000e5

---

## 3. CURRENT STATUS

- **NOT YET in the app store** (as of March 2026)
- GitHub repo: https://github.com/SterlingEly/BarGraph2 (branch: `master`)
- Working state but not submitted

---

## 4. REPO STRUCTURE

```
SterlingEly/BarGraph2 (master)
├── appinfo.json (or package.json — check current repo)
└── src/
    ├── main.c       ← SHA: 886dee8dccf9e160ba8eec293b69273d3ffa7b69
    └── pkjs/
        ├── config.js
        └── index.js
```

---

## 5. CURRENT VERSION SPEC

- Version: **2.0** (versionCode 2, SETTINGS_KEY: 1)
- Capabilities: configurable, health
- 11 messageKeys/appKeys: BackgroundColor(0) through Show24h(10)

### Settings struct
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
} BarSettings;
```

### Defaults
- Background: black; BarLit: white; BarDim: DarkGray; DateText: white
- RingBatt/Steps: white; RingDim: DarkGray
- StepGoal: 10000; ShowRing: true; InvertBW: false; Show24h: false

---

## 6. LAYOUT ARCHITECTURE

### KEY DESIGN INSIGHT (discovered by pixel-measuring v1 screenshots)
**The bars hug opposite screen edges — NOT centered.** This is the core v1 aesthetic. Large open gap in the middle.

```
Left:  [ring_inset] [LABEL_W=26] [TICK_LEN=5] [BAR_W=30] ...(gap)...
Right: ...(gap)... [BAR_W=30] [TICK_LEN=5] [LABEL_W=26] [ring_inset]
```

### Rect layout constants
```c
#define DATE_HEIGHT         22
#define BAR_MARGIN_TOP      4
#define BAR_MARGIN_BOTTOM   4
#define BAR_W               30    // matches v1 measured width
#define LABEL_W             26
#define TICK_LEN            5
#define RING_THICK          5
#define RING_GAP            3
```

### Round layout constants
```c
#define ROUND_BAR_W         24
#define ROUND_BAR_GAP       10
#define ROUND_TICK_MARGIN   28
#define ROUND_LABEL_MARGIN  14
#define ROUND_DATE_H        18
#define ROUND_DATE_GAP      4
```

### Bars fill BOTTOM-UP
- Hour bar (left): bottom = :00, top = 12:00 or 24:00
- Minute bar (right): bottom = :00, top = :59

### draw_bar_with_ticks()
Draws dim track, then lit fill, then bg-color horizontal lines (tick separators cut across full bar width through both regions).

### Ticks
- Hour: 1 per hour (12 or 24)
- Minute: 1 per 5 min (12 ticks: :05 through :59)
- 24h mode: thick (2px) notch at 12:00 AM/PM midpoint

### Labels
- Hours: right-aligned, left of bar, GOTHIC_14
  - 12h: every hour
  - 24h: every even hour
- Minutes: left-aligned, right of bar, GOTHIC_14, every 5 min
- Date: strftime("%a %b %e"), GOTHIC_18_BOLD, centered bottom

### Round: ticks extend to circle boundary
- Uses `prv_isqrt()` to compute chord width at each tick Y
- Ticks extend from bar edge to circle edge
- Labels near circle edge

---

## 7. OUTER RING

Same design as Radium 2:
- Battery: right half, fills from bottom
- Steps: left half, fills from bottom
- ShowRing toggle removes ring and inset

---

## 8. CONFIG PAGE

Derived from Radium 2. Color pickers for BarLit, BarDim, Background, DateText, ring colors. ShowRing toggle, InvertBW, Show24h, StepGoal slider. localStorage persistence.

---

## 9. CLOUDPEBBLE / BUILD RULES (CRITICAL)

1. No resources/media block in appinfo.json — causes build errors
2. Menu icons via CloudPebble UI only
3. No tilde (~) in resource filenames
4. CloudPebble imports from GitHub master

---

## 10. V1 VS V2

| Feature | v1 | v2 |
|---------|----|----|
| Platforms | aplite only | All 7 |
| Colors | B&W | Full color + B&W |
| Round screen | No | Yes |
| Config page | No | Yes |
| Outer ring | No | Optional |
| 24h mode | No | Optional |
| Bar position | Edge-hugging | Same (preserved) |
| Bar width | 30px | 30px |

---

## 11. OPEN ITEMS / NEXT STEPS

1. **Test on device/emulator** — rect low-res (144x168) and hi-res (200x228 emery)
2. **Test round layout** on chalk/diorite emulators
3. **Verify 24h mode** correctness
4. **Config page polish**
5. **Store submission** — description, screenshots, banner
6. **README/attribution** update

---

## 12. DEV ENVIRONMENT

- **CloudPebble:** https://cloudpebble.repebble.com
- **GitHub MCP connector:** Live on Mac desktop
- **Rebble Developer Portal:** https://dev.rebble.io

---

## 13. QUICK REFERENCE

```
Repo:         https://github.com/SterlingEly/BarGraph2
Branch:       master
Store status: NOT SUBMITTED
Version:      2.0 (versionCode 2)
SETTINGS_KEY: 1
main.c SHA:   886dee8dccf9e160ba8eec293b69273d3ffa7b69
```

---

*Bar Graph 2 is working. Next: test on hardware/emulator and prepare for store submission.*

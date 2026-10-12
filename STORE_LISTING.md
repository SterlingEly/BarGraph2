# Bar Graph 2 — Store Listing (DRAFT, updated for v2.23 — Arlo, 2026-10-12)

## Name
Bar Graph 2

## Short Description
Time as a bar chart. Hours left, minutes right — with tick marks, labels, and a full color model: 32 presets and per-element colors.

## Full Description

The premise is simple: your watch face is a bar chart.

The left bar fills to the current hour. The right bar fills to the current minute. Tick marks cross through the fill at each interval — visible as separators through the lit and dim regions both — so the scale is always readable no matter where the fill line sits. Labels along the outer edges keep everything explicit, and the date sits quietly below the bars.

It's a watchface that looks like data visualization, because it is.

Bar Graph 2 is a complete rebuild of the original Bar Graph watchface, designed in 2013 and published in 2014, faithfully preserving the original pixel geometry — bars near the center, symmetric gutters, labeled ticks — while adding a full color model for modern Pebbles. Choose from 32 presets in four rows (Dark, Dark two-tone, Light, Color), or drill down in settings and set every element yourself: bar fill and dim colors per bar, tick lines, digits, date, and background. On black & white models, an optional gray track shows the unfilled portion of each bar, and a B&W invert mode flips the whole face.

---
FEATURES:
- Vertical bar graph display — hours on the left, minutes on the right
- Tick separators visible through both the filled and unfilled regions of each bar
- Numeric labels along the outer edges; date displayed below the bars
- 12h and 24h display modes
- Full color model: 10 independent color slots (background, date, hour/minute digits, hour/minute tick lines, hour/minute lit and dim fills)
- 32 color presets in four rows: Dark, Dark two-tone, Light, Color — two-tone presets give the hour and minute bars different colors
- Hierarchical settings page: pick a preset, or open the palette picker and customize element by element
- Unfilled track option on B&W models: draws the empty portion of each bar in gray (off = the original 2013 blank-track look)
- B&W invert mode for Pebble Classic, Pebble 2, and Pebble 2 Duo

PLATFORMS:
Pebble Classic/Steel · Pebble Time/Steel · Pebble Time Round · Pebble 2 · Pebble Time 2 · Pebble 2 Duo · Pebble Round 2

---
2013: Original Bar Graph design by Sterling Ely
2013–2014: Implementation by Cameron MacFarland (distantcam)
2026: Bar Graph 2 rebuilt by Sterling Ely & Claude (Anthropic)

Feedback is appreciated: please hit me up at @sterlingely on Twitter/X with any bugs or feature requests!

---

## Release Notes

### v2.23
The color update:
- Full color model: 10 independent color slots — set the lit and dim fill of each bar separately, plus tick lines, digits, date, and background
- 32 color presets in four rows (Dark, Dark two-tone, Light, Color); two-tone presets give hours and minutes different colors
- New hierarchical settings page: presets up top, palette picker below for per-element control; settings now stored on the phone and survive reinstalls
- Unfilled track option on B&W models: the empty part of each bar can show in gray instead of disappearing into the background
- Date nudged for better spacing against the bars

### v2.12
- 144×168 B&W layout locked to the original 2013 geometry: separator logic, top/bottom borders
- Earlier builds rewrote the face to match the original exactly (no ring, no dim track) and fixed label rendering

---

## Drafting notes (not part of the listing — for Sterling/Claude)

- **Ring removed:** the previous STORE_LISTING.md described an outer battery/step ring. The current `main.c` contains no ring, battery, or steps code (README likewise calls the ring "reserved for future release"). This draft drops the ring claim. If the ring lands before submission, say the word and I'll add it back.
- **Version label open:** `appinfo.json` still says `versionLabel: 2.0`; the internal labels are v2.12 / v2.23. Release Notes above use the internal labels, following the codex's versioning rule (internal vs public labels are different source roles). Settle the public launch number before submission.
- **Platform tuning:** all 7 platforms are listed, matching `appinfo.json` targets and the Radium 2 format. Per PROJECT_CONTEXT, emery/basalt/round layouts were still pending tuning as of v2.12 — confirm they're submission-ready before this listing goes live.
- Modeled on `SterlingEly/Radium2` STORE_LISTING.md structure: Name / Short Description / Full Description with FEATURES + PLATFORMS blocks / credits timeline / feedback line / Release Notes.

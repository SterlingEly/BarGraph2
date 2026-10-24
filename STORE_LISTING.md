# Bar Graph 2 — Store Listing

> **Not yet published.** While Bar Graph 2 is unpublished, this file *is* the listing (current through v2.27). Once it is published, edits to this file become update drafts for the next release. Open pre-release items are under "Notes" at the bottom.

## Name
Bar Graph 2

## Short Description
Time as a bar chart. Hours left, minutes right — with tick marks, labels, and a full color model: 32 presets and per-element colors.

## Full Description

The premise is simple: your watch face is a bar chart.

The left bar fills to the current hour. The right bar fills to the current minute. Tick marks cross through the fill at each interval — visible as separators through the lit and dim regions both — so the scale is always readable no matter where the fill line sits. Labels along the outer edges keep everything explicit, and the date sits quietly below the bars.

It's a watchface that looks like data visualization, because it is.

Bar Graph 2 is a complete rebuild of the original Bar Graph watchface, designed by Sterling Ely in 2013 and first published in 2014. It keeps the original look — bars near the center, symmetric gutters, labeled ticks — and expands it for every Pebble platform, including the newest round and high-resolution models, with a full color model for modern Pebbles. Choose from 32 presets in four rows (Dark, Dark two-tone, Light, Color), or drill down in settings and set every element yourself: bar fill and dim colors per bar, tick lines, digits, date, and background. On black & white models, an optional gray track shows the unfilled portion of each bar, and a B&W invert mode flips the whole face.

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
- Round top tick setting
- B&W invert mode for Pebble Classic, Pebble 2, and Pebble 2 Duo
- Layouts tuned for each platform, including Pebble Time 2 and Pebble Round 2

PLATFORMS:
Pebble Classic/Steel · Pebble Time/Steel · Pebble Time Round · Pebble 2 · Pebble Time 2 · Pebble 2 Duo · Pebble Round 2

---
2013: Original Bar Graph design by Sterling Ely
2013–2014: Implementation by Cameron MacFarland (distantcam)
2026: Bar Graph 2 rebuilt and expanded by Sterling Ely & Claude (Anthropic)

Feedback is appreciated: please hit me up at @sterlingely on Twitter/X with any bugs or feature requests!

---

## Release Notes

### v2.27
- Digits are all-bold again for readability
- Horizontal layout reworked around a consistent unit grid for cleaner spacing on every platform
- New setting: round top tick

### v2.24
- The hour bar now reads full at 12 o'clock

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

## Notes (not part of the listing — for Sterling/Claude)

- **Template:** new listings and future update drafts start from the live Radium 2 listing on repebble (https://apps.repebble.com/radium-2_69a6531826cc4f0009c65926): tagline, description, features, platforms, credits, feedback line. It is the best listing so far, not a perfect one. The repo copy at `SterlingEly/Radium2` STORE_LISTING.md mirrors it.
- **Status of this file:** unpublished, so no "DRAFT" label (Sterling, 2026-10-24). After publication this file holds update drafts only.
- **Framing (decided 2026-10-24):** follows the Radium 2 pattern. Bar Graph was designed by Sterling years ago, implemented by others, and has now been fully rebuilt and expanded by Sterling & Claude. The credits timeline carries the attribution.
- **No ring:** `main.c` has no ring, battery, or steps code, and Sterling leans toward no ring (2026-10-12). The listing makes no ring claim.
- **Version label:** `appinfo.json` says `versionLabel: 2.0`, the planned public number. Release Notes use internal labels (v2.23, v2.24, v2.27), following the Codex versioning rule. Collapse them into a single public launch note at submission.
- **Platforms:** all 7 platform layouts were approved on device at v2.22 (2026-10-12); v2.24 and v2.27 changed the hour-bar and horizontal layout since. Re-confirm on device before submitting.
- **Not yet submitted** to the Rebble store; no store link exists.
- Release notes for v2.24 and v2.27 are drafted from the repo changelog and PROJECT_CONTEXT; adjust wording to taste.

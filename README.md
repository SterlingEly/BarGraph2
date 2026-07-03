# Bar Graph 2

A Pebble watchface displaying time as two vertical bar graphs — hours on the left, minutes on the right — with labeled tick marks and an optional battery/step ring.

**Current status:** In development. Aplite/diorite/flint layout locked at v2.12. Emery, basalt, and round platforms pending. Not yet submitted to the app store.

> **AI collaborators and technical contributors:** Read [`PROJECT_CONTEXT.md`](PROJECT_CONTEXT.md) before making any code changes. It contains the authoritative architecture reference, build rules, known traps, and current TODO.

---

## The idea

The premise is simple: your watch face is a bar chart.

The left bar fills to the current hour. The right bar fills to the current minute. Tick marks cross through the fill at each interval — visible as separators through both the filled and unfilled regions — so the scale is always readable no matter where the fill line sits. Labels along the outer edges keep everything explicit.

It's a watchface that looks like data visualization, because it is.

---

## Features

- **Hour bar (left) and minute bar (right)** with labeled tick marks
- **Tick separators** visible through both filled and unfilled bar regions
- **12h and 24h** display modes
- **Outer ring** — battery level (right half) and step count (left half) — reserved for future release
- **Configurable colors** for bars, labels, and ring segments
- **B&W invert** option for aplite/diorite/flint
- **All 7 platforms** — Aplite, Basalt, Chalk, Diorite, Emery, Flint, Gabbro

---

## Platforms

| Platform | Watch | Resolution | Color |
|----------|-------|------------|-------|
| Aplite   | Pebble Classic / Steel | 144×168 | B&W |
| Basalt   | Pebble Time / Steel | 144×168 | Color |
| Chalk    | Pebble Time Round | 180×180 | Color, round |
| Diorite  | Pebble 2 SE | 144×168 | B&W |
| Emery    | Pebble Time 2 | 200×228 | Color, hi-res |
| Flint    | Pebble 2 | 144×168 | B&W |
| Gabbro   | Pebble Round 2 | 260×260 | Color, round |

---

## History & credits

**2013 — Original design (Sterling Ely)**
The bar graph watchface concept was designed by Sterling Ely for the original Pebble.

**April–August 2013 — Implementation (Cameron MacFarland / distantcam)**
Cameron MacFarland built the original implementation.
- Early version: [distantcam/pebble](https://github.com/distantcam/pebble) (April 2013)
- Full release: [distantcam/Watchface-BarGraph](https://github.com/distantcam/Watchface-BarGraph) (August 2013 – March 2014)

**March 5, 2014 — v1.0 app store release**
Published to the Pebble App Store.
Appstore: [apps.repebble.com](https://apps.repebble.com/en_US/application/5305a587b704cb4e7d0000e5) · [apps.rebble.io](https://apps.rebble.io/en_US/application/5305a587b704cb4e7d0000e5)

**March 2026 — Bar Graph 2 (Sterling Ely & Claude)**
A full rebuild for all modern Pebble platforms, forked from [distantcam/Watchface-BarGraph](https://github.com/distantcam/Watchface-BarGraph). Sterling Ely led design and direction; Claude (Anthropic) handled technical implementation. Adds color support, configurable ring, 12h/24h mode, tick separators through bar fill, and round platform support.

---

## Building

Built with the Pebble SDK via CloudPebble. Requires `arial.ttf` registered as a font resource in CloudPebble (see `PROJECT_CONTEXT.md`).

```
pebble build
pebble install --emulator basalt
```

Source files:
- `src/main.c` — all drawing, event handling, settings persistence
- `src/pkjs/config.js` — config page HTML/JS (self-contained, no pebble-clay)
- `src/pkjs/index.js` — PebbleKit JS: settings relay and localStorage persistence
- `appinfo.json` — message keys, target platforms

See [`PROJECT_CONTEXT.md`](PROJECT_CONTEXT.md) for full build rules, CloudPebble-specific requirements, and known traps.

---

## Related projects

| Repository | Summary |
|------------|---------|
| [Radium 2](https://github.com/SterlingEly/Radium2) | Radial time display; released on Rebble store |
| [TallBoy](https://github.com/SterlingEly/TallBoy) | Oversized vector-drawn digits; all platforms |
| [Monogram](https://github.com/SterlingEly/Monogram) | Custom monogram-style digit watchface; early development |
| [Pixel Sampler](https://github.com/SterlingEly/PixelSampler) | Developer reference app for Pebble fonts, colors, and platform capabilities |

---

## License

MIT

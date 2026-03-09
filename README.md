# Bar Graph 2

A Pebble watchface displaying time as two vertical bar graphs — hours on the left, minutes on the right — with labeled tick marks and an optional battery/step ring.

---

## The idea

The premise is simple: your watch face is a bar chart.

The left bar fills to the current hour. The right bar fills to the current minute. Tick marks cross through the fill at each interval — visible as separators through both the lit and dim regions — so the scale is always readable no matter where the fill line sits. Labels along the outer edges keep everything explicit.

It's a watchface that looks like data visualization, because it is.

---

## Features

- **Hour bar (left) and minute bar (right)** with labeled tick marks
- **Tick separators** visible through both lit and dim bar regions
- **12h and 24h** display modes
- **Outer ring** — battery level (right half) and step count (left half)
- **Configurable colors** for bars, labels, and ring segments
- **B&W invert** option for aplite/diorite
- **All platforms** — Aplite, Basalt, Chalk, Diorite, Emery, Flint

---

## Platforms

| Platform | Watch | Resolution |
|----------|-------|------------|
| Aplite   | Pebble Classic / Steel | 144×168 (B&W) |
| Basalt   | Pebble Time / Steel | 144×168 (color) |
| Chalk    | Pebble Time Round | 180×180 (color) |
| Diorite  | Pebble 2 | 144×168 (B&W) |
| Emery    | Pebble Time 2 | 200×228 (color) |
| Flint    | Pebble 2 Duo | 144×168 (color) |

---

## History & credits

**2013 — Original design (Sterling Ely)**
The bar graph watchface concept was designed by Sterling Ely for the original Pebble in 2013.

**April–August 2013 — Implementation (Cameron MacFarland / distantcam)**
Cameron MacFarland built the original implementation.
- Early version: [distantcam/pebble](https://github.com/distantcam/pebble) (April 2013)
- Full release: [distantcam/Watchface-BarGraph](https://github.com/distantcam/Watchface-BarGraph) (August 2013 – March 2014)

**March 5, 2014 — v1.0 app store release**
Published to the Pebble App Store.
Appstore: [apps.repebble.com](https://apps.repebble.com/en_US/application/5305a587b704cb4e7d0000e5) · [apps.rebble.io](https://apps.rebble.io/en_US/application/5305a587b704cb4e7d0000e5)

**March 2026 — Bar Graph 2 (Sterling Ely & Claude)**
A full rebuild for all modern Pebble platforms, forked from [distantcam/Watchface-BarGraph](https://github.com/distantcam/Watchface-BarGraph). Sterling Ely led design and direction; Claude (Anthropic) handled technical implementation. Adds color support, configurable ring, 12h/24h mode, tick separators through bar fill, and round platform support.
GitHub: [SterlingEly/BarGraph2](https://github.com/SterlingEly/BarGraph2)

---

## Building

Built with the Pebble SDK (CloudPebble or local SDK). No external dependencies.

```
pebble build
pebble install --emulator basalt
```

Source files:
- `src/main.c` — all drawing, event handling, settings persistence
- `src/pkjs/config.js` — config page HTML/JS
- `src/pkjs/index.js` — PebbleKit JS: settings relay
- `appinfo.json` — message keys, target platforms

---

## License

MIT

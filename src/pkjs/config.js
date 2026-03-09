// ============================================================
// Bar Graph 2 — config.js  (Pebble Clay)
// ============================================================

module.exports = [
  {
    "type": "heading",
    "defaultValue": "Bar Graph 2"
  },

  // ── BARS ──────────────────────────────────────────────────
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Bars",
        "size": 4
      },
      {
        "type": "color",
        "messageKey": "BarLitColor",
        "label": "Filled bar color",
        "defaultValue": "#FFFFFF",
        "sunlight": true,
        "capabilities": ["COLOR"]
      },
      {
        "type": "color",
        "messageKey": "BarDimColor",
        "label": "Empty track color",
        "defaultValue": "#555555",
        "sunlight": true,
        "capabilities": ["COLOR"]
      },
      {
        "type": "color",
        "messageKey": "DateTextColor",
        "label": "Text & tick color",
        "defaultValue": "#FFFFFF",
        "sunlight": true,
        "capabilities": ["COLOR"]
      },
      {
        "type": "color",
        "messageKey": "BackgroundColor",
        "label": "Background color",
        "defaultValue": "#000000",
        "sunlight": true,
        "capabilities": ["COLOR"]
      }
    ]
  },

  // ── HOURS ─────────────────────────────────────────────────
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Hour display",
        "size": 4
      },
      {
        "type": "toggle",
        "messageKey": "Show24h",
        "label": "24-hour mode",
        "description": "Bar fills 0–23h. A center notch marks noon / midnight.",
        "defaultValue": false
      }
    ]
  },

  // ── RING ──────────────────────────────────────────────────
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Battery & Steps Ring",
        "size": 4
      },
      {
        "type": "toggle",
        "messageKey": "ShowRing",
        "label": "Show ring",
        "defaultValue": true
      },
      {
        "type": "color",
        "messageKey": "RingBattColor",
        "label": "Battery fill color",
        "defaultValue": "#FFFFFF",
        "sunlight": true,
        "capabilities": ["COLOR"]
      },
      {
        "type": "color",
        "messageKey": "RingStepsColor",
        "label": "Steps fill color",
        "defaultValue": "#FFFFFF",
        "sunlight": true,
        "capabilities": ["COLOR"]
      },
      {
        "type": "color",
        "messageKey": "RingDimColor",
        "label": "Ring track color",
        "defaultValue": "#555555",
        "sunlight": true,
        "capabilities": ["COLOR"]
      },
      {
        "type": "slider",
        "messageKey": "StepGoal",
        "label": "Daily step goal",
        "defaultValue": 10000,
        "min": 1000,
        "max": 30000,
        "step": 500
      }
    ]
  },

  // ── B&W ───────────────────────────────────────────────────
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Black & White watches",
        "size": 4
      },
      {
        "type": "toggle",
        "messageKey": "InvertBW",
        "label": "Invert (white background)",
        "defaultValue": false,
        "capabilities": ["NOT_COLOR"]
      }
    ]
  },

  {
    "type": "submit",
    "defaultValue": "Save"
  }
];

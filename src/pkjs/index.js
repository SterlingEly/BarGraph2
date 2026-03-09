// ============================================================
// Bar Graph 2 — index.js (PebbleKit JS)
// Handles Clay config + settings persistence via localStorage.
// ============================================================

var Clay = require('pebble-clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig);

// Hex string (#RRGGBB) → signed 32-bit int (for GColorFromHEX on watch)
function hexToInt(hex) {
  var n = parseInt(hex.replace('#', ''), 16);
  // GColorFromHEX expects the value as-is; no sign extension needed
  return n;
}

// Defaults mirror prv_default_settings() in main.c
var DEFAULTS = {
  BackgroundColor: '#000000',
  BarLitColor:     '#FFFFFF',
  BarDimColor:     '#555555',
  DateTextColor:   '#FFFFFF',
  RingBattColor:   '#FFFFFF',
  RingStepsColor:  '#FFFFFF',
  RingDimColor:    '#555555',
  StepGoal:        10000,
  ShowRing:        true,
  InvertBW:        false,
  Show24h:         false
};

function loadSettings() {
  var settings = {};
  for (var key in DEFAULTS) {
    var stored = localStorage.getItem('BG2_' + key);
    if (stored === null) {
      settings[key] = DEFAULTS[key];
    } else if (stored === 'true') {
      settings[key] = true;
    } else if (stored === 'false') {
      settings[key] = false;
    } else {
      var num = Number(stored);
      settings[key] = (!isNaN(num) && stored.indexOf('#') === -1) ? num : stored;
    }
  }
  return settings;
}

function saveSettings(settings) {
  for (var key in settings) {
    localStorage.setItem('BG2_' + key, settings[key]);
  }
}

function sendSettings(settings) {
  var dict = {
    BackgroundColor: hexToInt(settings.BackgroundColor),
    BarLitColor:     hexToInt(settings.BarLitColor),
    BarDimColor:     hexToInt(settings.BarDimColor),
    DateTextColor:   hexToInt(settings.DateTextColor),
    RingBattColor:   hexToInt(settings.RingBattColor),
    RingStepsColor:  hexToInt(settings.RingStepsColor),
    RingDimColor:    hexToInt(settings.RingDimColor),
    StepGoal:        parseInt(settings.StepGoal, 10),
    ShowRing:        settings.ShowRing  ? 1 : 0,
    InvertBW:        settings.InvertBW  ? 1 : 0,
    Show24h:         settings.Show24h   ? 1 : 0
  };

  Pebble.sendAppMessage(dict,
    function()  { console.log('BG2: settings sent'); },
    function(e) { console.log('BG2: send error: ' + JSON.stringify(e)); }
  );
}

Pebble.addEventListener('ready', function() {
  console.log('BG2: PebbleKit JS ready');
  sendSettings(loadSettings());
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (!e || !e.response) return;
  var settings = clay.getSettings(e.response);
  saveSettings(settings);
  sendSettings(settings);
});

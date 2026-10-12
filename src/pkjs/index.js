var buildUrl = require('./config').buildUrl;

var STORE = 'bg2_settings';
var COLOR_KEYS = ['BackgroundColor', 'DateColor', 'HourDigitColor', 'MinuteDigitColor',
  'HourLineColor', 'MinuteLineColor', 'HourLitColor', 'MinuteLitColor',
  'HourDimColor', 'MinuteDimColor'];
var BOOL_KEYS = ['InvertBW', 'ShowTrack', 'Show24h'];

function getPlatform() {
  try {
    var p = Pebble.getActiveWatchInfo().platform;
    return (p === 'aplite' || p === 'diorite' || p === 'flint') ? 'bw' : 'color';
  } catch (e) {
    return 'color';
  }
}

function load() {
  try { return JSON.parse(localStorage.getItem(STORE) || '{}'); } catch (e) { return {}; }
}

function toMessage(s) {
  var m = {}, i;
  for (i = 0; i < COLOR_KEYS.length; i++) {
    var c = s[COLOR_KEYS[i]];
    if (typeof c === 'string' && /^#[0-9a-fA-F]{6}$/.test(c)) m[COLOR_KEYS[i]] = parseInt(c.slice(1), 16);
  }
  for (i = 0; i < BOOL_KEYS.length; i++) {
    if (s.hasOwnProperty(BOOL_KEYS[i])) m[BOOL_KEYS[i]] = s[BOOL_KEYS[i]] ? 1 : 0;
  }
  return m;
}

function send(s) {
  var m = toMessage(s);
  Pebble.sendAppMessage(m, function () {}, function (e) {
    console.log('Bar Graph 2: send failed ' + JSON.stringify(e));
  });
}

Pebble.addEventListener('ready', function () {
  var s = load();
  if (Object.keys(s).length) send(s);
});

Pebble.addEventListener('showConfiguration', function () {
  Pebble.openURL(buildUrl(getPlatform(), load()));
});

Pebble.addEventListener('webviewclosed', function (e) {
  if (!e || !e.response) return;
  var s;
  try { s = JSON.parse(decodeURIComponent(e.response)); } catch (err) { return; }
  try { localStorage.setItem(STORE, JSON.stringify(s)); } catch (err2) {}
  send(s);
});

// ============================================================
// Bar Graph 2 — config.js
// Self-contained HTML config page (no pebble-clay dependency).
// Returns a data URI that Pebble opens as the settings page.
// ============================================================

module.exports = function(currentSettings) {
  var s = currentSettings || {};

  function v(key, def) {
    return (s[key] !== undefined) ? s[key] : def;
  }

  function colorRow(key, label, val) {
    return '<div class="row"><label>' + label + '</label>'
      + '<input type="color" data-key="' + key + '" value="' + val + '"></div>';
  }

  function toggleRow(key, label, val) {
    return '<div class="row"><label>' + label + '</label>'
      + '<label class="toggle"><input type="checkbox" data-key="' + key + '"'
      + (val ? ' checked' : '') + '><span class="slider-toggle"></span></label></div>';
  }

  function rangeRow(key, label, val, min, max, step) {
    return '<div class="row"><label>' + label + '</label>'
      + '<div style="display:flex;align-items:center;gap:8px;">'
      + '<input type="range" data-key="' + key + '" min="' + min + '" max="' + max
      + '" step="' + step + '" value="' + val + '">'
      + '<span class="range-val">' + val + '</span></div></div>';
  }

  var html = '<!DOCTYPE html><html><head><meta charset="utf-8">'
    + '<meta name="viewport" content="width=device-width,initial-scale=1">'
    + '<title>Bar Graph 2</title>'
    + '<style>'
    + 'body{font-family:sans-serif;background:#1a1a1a;color:#eee;margin:0;padding:16px;box-sizing:border-box;}'
    + 'h1{font-size:1.2em;margin:0 0 16px;color:#fff;}'
    + 'h2{font-size:0.9em;color:#aaa;margin:20px 0 8px;text-transform:uppercase;letter-spacing:0.05em;}'
    + '.row{display:flex;justify-content:space-between;align-items:center;padding:10px 0;border-bottom:1px solid #333;}'
    + '.row:last-child{border-bottom:none;}'
    + 'label{font-size:0.95em;}'
    + 'input[type=color]{width:44px;height:32px;border:none;border-radius:4px;cursor:pointer;background:none;}'
    + 'input[type=range]{width:140px;}'
    + '.toggle{position:relative;width:44px;height:24px;}'
    + '.toggle input{opacity:0;width:0;height:0;}'
    + '.slider-toggle{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;background:#555;border-radius:24px;transition:.2s;}'
    + '.slider-toggle:before{position:absolute;content:"";height:18px;width:18px;left:3px;bottom:3px;background:#fff;border-radius:50%;transition:.2s;}'
    + 'input:checked+.slider-toggle{background:#2196F3;}'
    + 'input:checked+.slider-toggle:before{transform:translateX(20px);}'
    + '.section{background:#252525;border-radius:8px;padding:0 14px;margin-bottom:16px;}'
    + '.range-val{font-size:0.85em;color:#aaa;min-width:48px;text-align:right;}'
    + 'button{width:100%;padding:14px;background:#2196F3;color:#fff;border:none;border-radius:8px;font-size:1em;cursor:pointer;margin-top:8px;}'
    + 'button:active{background:#1976D2;}'
    + '</style></head><body>'
    + '<h1>Bar Graph 2</h1>'

    + '<h2>Bars</h2><div class="section">'
    + colorRow('BarLitColor',    'Filled bar color',   v('BarLitColor',    '#FFFFFF'))
    + colorRow('BarDimColor',    'Empty track color',  v('BarDimColor',    '#555555'))
    + colorRow('DateTextColor',  'Text & tick color',  v('DateTextColor',  '#FFFFFF'))
    + colorRow('BackgroundColor','Background color',   v('BackgroundColor','#000000'))
    + '</div>'

    + '<h2>Hour Display</h2><div class="section">'
    + toggleRow('Show24h', '24-hour mode', v('Show24h', false))
    + '</div>'

    + '<h2>Battery &amp; Steps Ring</h2><div class="section">'
    + toggleRow('ShowRing',      'Show ring',          v('ShowRing', true))
    + colorRow('RingBattColor',  'Battery fill color', v('RingBattColor',  '#FFFFFF'))
    + colorRow('RingStepsColor', 'Steps fill color',   v('RingStepsColor', '#FFFFFF'))
    + colorRow('RingDimColor',   'Ring track color',   v('RingDimColor',   '#555555'))
    + rangeRow('StepGoal', 'Daily step goal', v('StepGoal', 10000), 1000, 30000, 500)
    + '</div>'

    + '<h2>Black &amp; White Watches</h2><div class="section">'
    + toggleRow('InvertBW', 'Invert (white background)', v('InvertBW', false))
    + '</div>'

    + '<button onclick="save()">Save</button>'

    + '<script>'
    + 'function save(){'
    + '  var r={};'
    + '  document.querySelectorAll("input[data-key]").forEach(function(el){'
    + '    var k=el.dataset.key;'
    + '    if(el.type==="color") r[k]=el.value;'
    + '    else if(el.type==="checkbox") r[k]=el.checked;'
    + '    else if(el.type==="range") r[k]=parseInt(el.value,10);'
    + '  });'
    + '  var loc=window.location.href;'
    + '  var ret=loc.substring(0,loc.indexOf("?"));'
    + '  document.location=ret+"?return_to=pebblejs%3A%2F%2Fback#"+encodeURIComponent(JSON.stringify(r));'
    + '}'
    + 'document.querySelectorAll("input[type=range]").forEach(function(el){'
    + '  var d=el.parentNode.querySelector(".range-val");'
    + '  if(d) el.addEventListener("input",function(){d.textContent=el.value;});'
    + '});'
    + '</script>'
    + '</body></html>';

  return 'data:text/html,' + encodeURIComponent(html);
};

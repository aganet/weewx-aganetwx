/* AganetWX client: charts, moon phase, and the in-page language switcher. */
(function () {
  "use strict";

  var DARK = document.documentElement.getAttribute("data-theme") === "dark";
  var AX = DARK ? "#9fb0c0" : "#666";        // axis labels
  var GRID = DARK ? "#2c3c4d" : "#e6e6e6";   // split lines
  var TITLE = DARK ? "#8ab4f8" : "#0a5ca8";  // chart titles

  // Language dictionaries for the switcher (code -> {englishKey: translation}).
  // Server-rendered pages carry the default; the switcher swaps client-side.
  var DICTS = window.AGANETWX_I18N_ALL || {};
  var LANG = window.AGANETWX_LANG || document.documentElement.lang || "en";
  var decoder = document.createElement("textarea");
  function decode(s) { decoder.innerHTML = s; return decoder.value; }
  // Translate an English key into the active language (fallback: the key).
  function tr(key) {
    var d = DICTS[LANG] || {};
    return decode(d[key] || key);
  }
  // Chart-series short keys mapped to their English label, then translated.
  var CHART_KEYS = {
    temp: "Temperature", dewpoint: "Dew Point", appTemp: "Apparent",
    heatindex: "Heat Index", windchill: "Wind Chill", humidity: "Humidity",
    pressure: "Barometer", windSpeed: "Wind Speed", windGust: "Gust",
    windvec: "Wind Direction", windvector: "Wind Vector", windrose: "Wind Rose",
    calm: "calm", rain: "Rain", rainRate: "Rain Rate", UV: "UV Index",
    radiation: "Solar Radiation", ET: "Evapotranspiration", cloudbase: "Cloud Base"
  };
  // Single-language build (no switcher): fall back to the page's own labels.
  var PAGE_LABELS = window.AGANETWX_I18N || {};
  function t(k) {
    if (DICTS[LANG]) return tr(CHART_KEYS[k] || k);
    return PAGE_LABELS[k] ? decode(PAGE_LABELS[k]) : (CHART_KEYS[k] || k);
  }

  var COLORS = {
    temp: "#e8623d", dewpoint: "#0b76d0", appTemp: "#8e44ad", heatindex: "#c0392b", windchill: "#2980b9",
    humidity: "#27ae60", pressure: "#7f8c8d", windSpeed: "#16a085", windGust: "#0a5ca8",
    windvec: "#34495e", rain: "#2e86de", rainRate: "#2e86de", UV: "#9b59b6", radiation: "#f39c12",
    ET: "#1abc9c", cloudbase: "#95a5a6"
  };

  // Unit labels from the configured system (metric fallbacks if not injected).
  var UNITS = window.AGANETWX_UNITS || {};
  // Decode entities (page is html_entities-encoded) so axes show °C not &#176;C.
  function u(key, dflt) { return UNITS[key] ? decode(UNITS[key]) : dflt; }

  // ECharts plumbing
  var charts = {};
  window.__aganetwxCharts = charts;
  function chart(id) {
    var el = document.getElementById(id);
    if (!el) return null;
    if (!charts[id]) charts[id] = echarts.init(el);
    return charts[id];
  }
  function nonEmpty(arr) { return Array.isArray(arr) && arr.length > 0; }

  var PERIOD = window.AGANETWX_PERIOD || "day";
  function pad(n) { return (n < 10 ? "0" : "") + n; }
  var COMPASS = ["N","NNE","NE","ENE","E","ESE","SE","SSE","S","SSW","SW","WSW","W","WNW","NW","NNW"];
  function compass(deg) { return COMPASS[Math.round(((deg % 360) / 22.5)) % 16]; }
  // Chart times are labeled in the STATION's timezone (IANA name injected as
  // AGANETWX_TZ), so they're correct for any viewer. Intl handles offset + DST;
  // an empty/invalid zone falls back to the viewer's browser timezone.
  var TZ = window.AGANETWX_TZ || undefined;
  function tzParts(ts) {
    var p = {};
    try {
      new Intl.DateTimeFormat("en-GB", { timeZone: TZ, hour12: false,
        year: "2-digit", month: "2-digit", day: "2-digit", hour: "2-digit", minute: "2-digit" })
        .formatToParts(new Date(ts)).forEach(function (x) { p[x.type] = x.value; });
    } catch (e) {
      var d = new Date(ts);
      p = { hour: pad(d.getHours()), minute: pad(d.getMinutes()),
            day: pad(d.getDate()), month: pad(d.getMonth() + 1),
            year: pad(d.getFullYear() % 100) };
    }
    return p;
  }
  // Axis tick label by period: intraday shows time, longer spans show a date.
  function axisTime(ts) {
    var p = tzParts(ts);
    if (PERIOD === "day" || PERIOD === "yesterday") return p.hour + ":" + p.minute;
    if (PERIOD === "year" || PERIOD === "lastyear") return p.month + "/" + p.year;
    return p.day + "/" + p.month;
  }
  // For day/yesterday, pin the axis to the exact day bounds (set from the JSON)
  // so ECharts does not pad past midnight into the next day.
  var axisMin = null, axisMax = null;
  function timeAxis() {
    var ax = { type: "time",
               axisLabel: { fontSize: 10, color: AX, formatter: axisTime },
               splitLine: { lineStyle: { color: GRID } } };
    if (axisMin != null) {
      ax.min = axisMin; ax.max = axisMax;
      ax.interval = 4 * 3600 * 1000;   // 4-hour ticks across the fixed day
    }
    return ax;
  }

  // Degree and percent sit tight on the number; other units keep a space.
  function unitSuffix(unit) {
    if (!unit) return "";
    return (unit === "%" || unit.charAt(0) === "°") ? unit : " " + unit;
  }
  function baseOpt(title, unit) {
    var suffix = unitSuffix(unit);
    return {
      title: { text: title, left: 8, top: 4, textStyle: { fontSize: 13, color: TITLE, fontWeight: 600 } },
      grid: { left: 64, right: 16, top: 42, bottom: 28 },
      tooltip: { trigger: "axis", valueFormatter: function (v) { return v + suffix; } },
      legend: { top: 4, right: 8, textStyle: { fontSize: 11, color: AX } },
      xAxis: timeAxis(),
      yAxis: { type: "value", scale: true,
               axisLabel: { fontSize: 10, color: AX,
                            formatter: function (v) { return v + suffix; } },
               splitLine: { lineStyle: { color: GRID } } }
    };
  }
  function line(name, color, data, area) {
    return { name: name, type: "line", showSymbol: false, smooth: true,
             lineStyle: { width: 1.8, color: color }, itemStyle: { color: color },
             areaStyle: area ? { opacity: 0.12, color: color } : undefined, data: data };
  }
  function bar(name, color, data) {
    return { name: name, type: "bar", itemStyle: { color: color }, data: data };
  }
  // Draw a chart with its unit on the y-axis and tooltip. Hidden if no data.
  function draw(id, title, unit, series) {
    var c = chart(id);
    if (!c) return;
    var has = series.some(function (s) { return nonEmpty(s.data); });
    if (!has) { c.getDom().style.display = "none"; return; }
    c.setOption(Object.assign(baseOpt(title, unit), { series: series }), true);
  }

  function render(d) {
    if (!d) return;
    // Pin the time axis to the day's midnight bounds when provided (UTC epochs
    // of the station's local midnight, computed server-side).
    axisMin = (typeof d.start === "number") ? d.start : null;
    axisMax = (typeof d.end === "number") ? d.end : null;

    // Temperature + derived
    var ts = [];
    if (nonEmpty(d.outTemp))   ts.push(line(t("temp"), COLORS.temp, d.outTemp, true));
    if (nonEmpty(d.dewpoint))  ts.push(line(t("dewpoint"), COLORS.dewpoint, d.dewpoint));
    if (nonEmpty(d.appTemp))   ts.push(line(t("appTemp"), COLORS.appTemp, d.appTemp));
    if (nonEmpty(d.heatindex)) ts.push(line(t("heatindex"), COLORS.heatindex, d.heatindex));
    if (nonEmpty(d.windchill)) ts.push(line(t("windchill"), COLORS.windchill, d.windchill));
    draw("chart-temp", t("temp"), u("temp", "°C"), ts);

    draw("chart-humidity", t("humidity"), "%",
         [line(t("humidity"), COLORS.humidity, d.outHumidity, true)]);

    draw("chart-pressure", t("pressure"), u("pressure", "hPa"),
         [line(t("pressure"), COLORS.pressure, d.barometer)]);

    draw("chart-windspeed", t("windSpeed"), u("wind", "km/h"),
         [line(t("windSpeed"), COLORS.windSpeed, d.windSpeed, true),
          line(t("windGust"), COLORS.windGust, d.windGust)]);

    // Wind direction scatter (bearing 0..360). Exact degree shown on hover.
    var wv = chart("chart-windvec");
    if (wv) {
      if (nonEmpty(d.windDir)) {
        wv.setOption({
          title: { text: t("windvec") + " (°)", left: 8, top: 4, textStyle: { fontSize: 13, color: TITLE, fontWeight: 600 } },
          grid: { left: 52, right: 16, top: 42, bottom: 28 },
          tooltip: { trigger: "item", formatter: function (p) {
            var deg = Math.round(p.value[1]);
            return axisTime(p.value[0]) + "<br/><b>" + deg + "°</b> " + compass(deg);
          } },
          xAxis: timeAxis(),
          yAxis: { type: "value", min: 0, max: 360, interval: 90,
                   axisLabel: { fontSize: 10, color: AX, formatter: function (v) {
                     return ({0:"N",90:"E",180:"S",270:"W",360:"N"})[v] || v; } },
                   splitLine: { lineStyle: { color: GRID } } },
          series: [{ name: t("windvec"), type: "scatter", symbolSize: 5,
                     itemStyle: { color: COLORS.windvec }, data: d.windDir }]
        }, true);
      } else { wv.getDom().style.display = "none"; }
    }

    // Wind vector: arrow per point, y = speed, rotated to direction, thinned to avoid overlap.
    var wvec = chart("chart-windvector");
    if (wvec) {
      if (nonEmpty(d.windSpeed) && nonEmpty(d.windDir)) {
        var dir = {};
        for (var j = 0; j < d.windDir.length; j++) dir[d.windDir[j][0]] = d.windDir[j][1];
        var step = Math.max(1, Math.ceil(d.windSpeed.length / 40));
        var maxSpd = 0, pts = [];
        for (var i = 0; i < d.windSpeed.length; i += step) {
          var ts = d.windSpeed[i][0], spd = d.windSpeed[i][1], deg = dir[ts];
          if (spd == null || deg == null) continue;
          if (spd > maxSpd) maxSpd = spd;
          pts.push([ts, spd, deg]);
        }
        wvec.setOption({
          title: { text: t("windvector") + " (" + u("wind", "km/h") + ")", left: 8, top: 4, textStyle: { fontSize: 13, color: TITLE, fontWeight: 600 } },
          grid: { left: 64, right: 16, top: 42, bottom: 28 },
          tooltip: { trigger: "item", formatter: function (p) {
            var spd = p.value[1], deg = Math.round(p.value[2]);
            return axisTime(p.value[0]) + "<br/><b>" + spd + " " + u("wind", "km/h") + "</b> " + deg + "° " + compass(deg);
          } },
          xAxis: timeAxis(),
          yAxis: { type: "value", scale: true,
                   axisLabel: { fontSize: 10, color: AX, formatter: function (v) { return v + " " + u("wind", "km/h"); } },
                   splitLine: { lineStyle: { color: GRID } } },
          visualMap: { show: false, min: 0, max: maxSpd || 1, dimension: 1,
                       inRange: { color: ["#16a085", "#f39c12", "#e8623d"] } },
          series: [{
            name: t("windvector"), type: "custom", data: pts,
            renderItem: function (params, api) {
              var x = api.coord([api.value(0), api.value(1)]);
              var deg = api.value(2);
              // Bearing is where wind comes FROM; arrow points TO (deg+180). SVG y is down.
              var a = (deg + 180) * Math.PI / 180;
              var len = 11;
              var dx = Math.sin(a) * len, dy = -Math.cos(a) * len;
              return { type: "group", children: [
                { type: "line", shape: { x1: x[0] - dx, y1: x[1] - dy, x2: x[0] + dx, y2: x[1] + dy },
                  style: { stroke: api.visual("color"), lineWidth: 1.6 } },
                { type: "circle", shape: { cx: x[0] + dx, cy: x[1] + dy, r: 2 },
                  style: { fill: api.visual("color") } }
              ] };
            }
          }]
        }, true);
      } else { wvec.getDom().style.display = "none"; }
    }

    windRose(d);

    draw("chart-rain", t("rain"), u("rain", "mm"), [bar(t("rain"), COLORS.rain, d.rain)]);
    draw("chart-rainrate", t("rainRate"), u("rainRate", "mm/h"), [line(t("rainRate"), COLORS.rainRate, d.rainRate, true)]);
    draw("chart-uv", t("UV"), "", [line(t("UV"), COLORS.UV, d.UV, true)]);
    draw("chart-radiation", t("radiation"), u("radiation", "W/m²"), [line(t("radiation"), COLORS.radiation, d.radiation, true)]);
    draw("chart-et", t("ET"), u("ET", "mm"), [bar(t("ET"), COLORS.ET, d.ET)]);
    draw("chart-cloudbase", t("cloudbase"), u("cloudbase", "m"), [line(t("cloudbase"), COLORS.cloudbase, d.cloudbase)]);
  }

  // Wind rose: direction frequency (16 sectors) stacked by speed band, as % of all obs; calm shown as a centre %.
  var ROSE_BANDS = [
    { max: 5,   color: "#a6d96a", label: "0-5" },
    { max: 15,  color: "#66bd63", label: "5-15" },
    { max: 25,  color: "#fdae61", label: "15-25" },
    { max: 40,  color: "#f46d43", label: "25-40" },
    { max: Infinity, color: "#d73027", label: "40+" }
  ];
  var ROSE_SECTORS = ["N","NNE","NE","ENE","E","ESE","SE","SSE","S","SSW","SW","WSW","W","WNW","NW","NNW"];
  function windRose(d) {
    var c = chart("chart-windrose");
    if (!c) return;
    if (!nonEmpty(d.windDir) || !nonEmpty(d.windSpeed)) { c.getDom().style.display = "none"; return; }

    var spd = {};
    for (var j = 0; j < d.windSpeed.length; j++) spd[d.windSpeed[j][0]] = d.windSpeed[j][1];
    // counts[band][sector]
    var counts = ROSE_BANDS.map(function () { return ROSE_SECTORS.map(function () { return 0; }); });
    var total = 0, calm = 0;
    for (var i = 0; i < d.windDir.length; i++) {
      var ts = d.windDir[i][0], deg = d.windDir[i][1], s = spd[ts];
      if (deg == null || s == null) continue;
      total++;
      if (s <= 0.5) { calm++; continue; }
      var sec = Math.round((deg % 360) / 22.5) % 16;
      var b = 0;
      while (b < ROSE_BANDS.length - 1 && s > ROSE_BANDS[b].max) b++;
      counts[b][sec]++;
    }
    if (!total) { c.getDom().style.display = "none"; return; }

    var series = ROSE_BANDS.map(function (band, bi) {
      return {
        type: "bar", coordinateSystem: "polar", name: band.label + " " + u("wind", "km/h"),
        stack: "s", itemStyle: { color: band.color },
        data: counts[bi].map(function (n) { return +(100 * n / total).toFixed(1); })
      };
    });
    var calmPct = total ? Math.round(100 * calm / total) : 0;

    c.setOption({
      title: { text: t("windrose") + " (%)", left: 8, top: 4,
               textStyle: { fontSize: 13, color: TITLE, fontWeight: 600 },
               subtext: calmPct + "% " + t("calm"), subtextStyle: { fontSize: 10, color: AX } },
      legend: { bottom: 4, textStyle: { fontSize: 11, color: AX }, itemWidth: 14, itemHeight: 9 },
      tooltip: { trigger: "item", formatter: function (p) {
        return ROSE_SECTORS[p.dataIndex] + "<br/>" + p.seriesName + ": <b>" + p.value + "%</b>";
      } },
      polar: { radius: ["6%", "80%"], center: ["50%", "52%"] },
      angleAxis: { type: "category", data: ROSE_SECTORS, startAngle: 90,
                   axisLabel: { fontSize: 11, color: AX },
                   axisLine: { lineStyle: { color: GRID } },
                   splitLine: { show: true, lineStyle: { color: GRID } } },
      radiusAxis: { axisLabel: { fontSize: 10, color: AX, formatter: "{value}%" },
                    splitLine: { lineStyle: { color: GRID } } },
      series: series
    }, true);
  }

  window.addEventListener("resize", function () {
    Object.keys(charts).forEach(function (k) {
      if (charts[k].getDom().style.display !== "none") charts[k].resize();
    });
  });

  // Moon phase from data-fullness (0..100): lit/dark halves plus a terminator ellipse; data-waning flips sides.
  function paintOneMoon(m, idx) {
    var f = Math.max(0, Math.min(100, parseFloat(m.getAttribute("data-fullness")) || 0));
    var waning = m.getAttribute("data-waning") === "1";
    var lit = "#eef1f6", shadow = "#141922";   // bright moonlight, dark limb
    var k = f / 100;
    var rx = 50 * Math.abs(1 - 2 * k);
    var leftLit = waning, rightLit = !waning, ellLit = (k >= 0.5);
    // Edge cases: full moon = both halves lit; new moon = both dark.
    if (f >= 99.5) { leftLit = rightLit = true; }
    else if (f <= 0.5) { leftLit = rightLit = false; }
    function col(b) { return b ? lit : shadow; }
    var cid = "moonClip" + idx;
    var svg = '<svg viewBox="0 0 100 100" width="100%" height="100%" style="display:block">' +
      '<defs>' +
        '<clipPath id="' + cid + '"><circle cx="50" cy="50" r="50"/></clipPath>' +
        // Spherical shading: bright near the light source, softly darker at the limb.
        '<radialGradient id="sh' + idx + '" cx="36%" cy="32%" r="78%">' +
          '<stop offset="0%" stop-color="#ffffff" stop-opacity="0.45"/>' +
          '<stop offset="60%" stop-color="#ffffff" stop-opacity="0"/>' +
          '<stop offset="100%" stop-color="#000000" stop-opacity="0.30"/>' +
        '</radialGradient>' +
      '</defs>' +
      '<g clip-path="url(#' + cid + ')">' +
        '<rect x="0"  y="0" width="50" height="100" fill="' + col(leftLit) + '"/>' +
        '<rect x="50" y="0" width="50" height="100" fill="' + col(rightLit) + '"/>';
    if (f > 0.5 && f < 99.5) {
      svg += '<ellipse cx="50" cy="50" rx="' + rx + '" ry="50" fill="' + col(ellLit) + '"/>';
    }
    svg += '<circle cx="50" cy="50" r="50" fill="url(#sh' + idx + ')"/>';
    svg += '</g></svg>';
    m.innerHTML = svg;
    m.style.overflow = "hidden";
    m.style.boxShadow = "inset 0 0 12px rgba(0,0,0,.35)";
  }
  Array.prototype.forEach.call(document.querySelectorAll(".moon"), paintOneMoon);

  // Load this page period JSON, then render. Keep the data so a language
  // switch can redraw the charts with the new labels.
  var chartData = null;
  if (document.getElementById("chart-temp")) {
    fetch("data/" + PERIOD + ".json", { cache: "no-store" })
      .then(function (r) { return r.json(); })
      .then(function (d) { chartData = d; render(d); })
      .catch(function (e) { console.error("AganetWX: failed to load", PERIOD, e); });
  }

  // In-page language switcher. Swaps every [data-i18n] label and redraws the
  // charts, with no page reload. Active language is remembered in localStorage;
  // first visit uses the browser language if we ship it.
  var LS_KEY = "aganetwx_lang";
  function available() { return Object.keys(DICTS); }
  function applyLang(code) {
    var d = DICTS[code];
    if (!d) return;
    LANG = code;
    document.documentElement.lang = code;
    document.body.setAttribute("data-lang", code);
    Array.prototype.forEach.call(document.querySelectorAll(".i18n"), function (el) {
      var key = el.getAttribute("data-i18n");
      if (key && d[key] != null) el.textContent = decode(d[key]);
    });
    if (chartData) render(chartData);
    var sel = document.getElementById("lang-select");
    if (sel && sel.value !== code) sel.value = code;
    try { localStorage.setItem(LS_KEY, code); } catch (e) {}
  }
  function initLang() {
    var langs = available();
    if (langs.length < 2) return;   // switcher disabled
    var choice = null;
    try { choice = localStorage.getItem(LS_KEY); } catch (e) {}
    if (!choice || langs.indexOf(choice) < 0) {
      var nav = navigator.languages || [navigator.language || ""];
      for (var i = 0; i < nav.length && !choice; i++) {
        var c = String(nav[i]).slice(0, 2).toLowerCase();
        if (langs.indexOf(c) >= 0) choice = c;
      }
    }
    if (!choice || langs.indexOf(choice) < 0) choice = LANG;
    var sel = document.getElementById("lang-select");
    if (sel) sel.addEventListener("change", function () { applyLang(sel.value); });
    // Only repaint if the chosen language differs from the server-rendered one.
    if (choice !== LANG) applyLang(choice);
    else if (sel) sel.value = choice;
  }
  initLang();
})();

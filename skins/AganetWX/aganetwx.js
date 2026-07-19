/* AganetWX client: charts, moon phase, and the in-page language switcher. */
(function () {
  "use strict";

  // Theme-derived chart colors. Recomputed when the theme switcher changes mode.
  var DARK, AX, GRID, TITLE;
  function readTheme() {
    DARK = document.documentElement.getAttribute("data-theme") === "dark";
    AX = DARK ? "#9fb0c0" : "#666";        // axis labels
    GRID = DARK ? "#2c3c4d" : "#e6e6e6";   // split lines
    TITLE = DARK ? "#8ab4f8" : "#0a5ca8";  // chart titles
  }
  readTheme();

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
  var redraw = {};
  function chart(id) {
    var el = document.getElementById(id);
    if (!el) return null;
    if (!charts[id]) charts[id] = echarts.init(el);
    return charts[id];
  }

  // Lazy building: each chart's build closure is registered by DOM id and only
  // run when its element first nears the viewport, so a page full of charts
  // does not initialise them all at once on load. Charts already visible build
  // immediately. Falls back to building everything if IntersectionObserver is
  // unavailable.
  var builders = {};
  var built = {};
  var observer = null;
  function runBuilder(id) {
    if (built[id] || !builders[id]) return;
    built[id] = true;
    builders[id]();
  }
  function defer(id, build) {
    builders[id] = build;
    var el = document.getElementById(id);
    if (!el || !("IntersectionObserver" in window)) { runBuilder(id); return; }
    if (!observer) {
      observer = new IntersectionObserver(function (entries) {
        entries.forEach(function (e) {
          if (e.isIntersecting) { observer.unobserve(e.target); runBuilder(e.target.id); }
        });
      }, { rootMargin: "200px 0px" });   // start just before it scrolls in
    }
    observer.observe(el);
  }
  // Re-tint live charts for a light/dark switch by merging only the theme-derived
  // colors (axis labels, gridlines, titles, legend), leaving series data and the
  // rest of each option untouched. Much lighter than rebuilding every option.
  function retint() {
    readTheme();
    var patch = {
      title:  { textStyle: { color: TITLE } },
      legend: { textStyle: { color: AX } },
      xAxis:  { axisLabel: { color: AX }, splitLine: { lineStyle: { color: GRID } } },
      yAxis:  { axisLabel: { color: AX }, splitLine: { lineStyle: { color: GRID } } },
      radiusAxis: { axisLabel: { color: AX } },
      angleAxis:  { axisLabel: { color: AX } }
    };
    for (var id in charts) {
      if (charts[id]) charts[id].setOption(patch, false);   // merge, keep everything else
    }
  }
  function nonEmpty(arr) { return Array.isArray(arr) && arr.length > 0; }

  var PERIOD = window.AGANETWX_PERIOD || "day";
  function pad(n) { return (n < 10 ? "0" : "") + n; }
  var COMPASS = ["N","NNE","NE","ENE","E","ESE","SE","SSE","S","SSW","SW","WSW","W","WNW","NW","NNW"];
  function compass(deg) { return COMPASS[Math.round(((deg % 360) / 22.5)) % 16]; }
  // Chart times must read in the STATION's timezone (AGANETWX_TZ) for every
  // visitor, not the viewer's. The charts run with useUTC (see setOption), so
  // ECharts renders timestamps as UTC with no browser-offset applied; we shift
  // each real UTC epoch by the station's offset first, so "UTC" then reads as
  // the station's wall-clock. One mechanism: shift here, useUTC there.
  var TZ = window.AGANETWX_TZ || undefined;
  // Station UTC offset (ms) at instant ts, via Intl. Handles DST per-timestamp.
  function tzOffsetMs(ts) {
    if (!TZ) return 0;
    try {
      var p = {};
      new Intl.DateTimeFormat("en-US", { timeZone: TZ, hour12: false,
        year: "numeric", month: "2-digit", day: "2-digit",
        hour: "2-digit", minute: "2-digit", second: "2-digit" })
        .formatToParts(new Date(ts)).forEach(function (x) { p[x.type] = x.value; });
      var asUTC = Date.UTC(+p.year, +p.month - 1, +p.day,
                           +(p.hour === "24" ? "0" : p.hour), +p.minute, +p.second);
      return asUTC - ts;
    } catch (e) { return 0; }
  }
  // Shift a real epoch so its UTC representation is the station wall-clock.
  function toStationClock(ts) { return ts + tzOffsetMs(ts); }
  // Parts of a pre-shifted (pseudo-UTC) timestamp, read in UTC.
  function tzParts(ts) {
    var d = new Date(ts);
    return { hour: pad(d.getUTCHours()), minute: pad(d.getUTCMinutes()),
             day: pad(d.getUTCDate()), month: pad(d.getUTCMonth() + 1),
             year: pad(d.getUTCFullYear() % 100) };
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
  function baseOpt(title, unit, width) {
    var suffix = unitSuffix(unit);
    // On narrow (mobile) widths the multi-series legend collides with the title,
    // so drop it onto its own centered row below the title and lower the grid.
    var narrow = width && width < 520;
    var legend = narrow
      ? { top: 26, left: "center", textStyle: { fontSize: 10, color: AX }, itemWidth: 12, itemHeight: 8, itemGap: 8 }
      : { top: 4, right: 8, textStyle: { fontSize: 11, color: AX } };
    return {
      useUTC: true,
      title: { text: title, left: 8, top: 4, textStyle: { fontSize: 13, color: TITLE, fontWeight: 600 } },
      grid: { left: 64, right: 16, top: narrow ? 66 : 42, bottom: 28 },
      tooltip: { trigger: "axis", valueFormatter: function (v) { return v + suffix; } },
      legend: legend,
      xAxis: timeAxis(),
      yAxis: { type: "value", scale: true,
               axisLabel: { fontSize: 10, color: AX,
                            formatter: function (v) { return v + suffix; } },
               splitLine: { lineStyle: { color: GRID } } }
    };
  }
  // Shift each [ts_ms, value] point into the station-clock (pseudo-UTC) domain.
  function shiftSeries(data) {
    if (!Array.isArray(data)) return data;
    return data.map(function (pt) {
      return Array.isArray(pt) ? [toStationClock(pt[0])].concat(pt.slice(1)) : pt;
    });
  }
  function line(name, color, data, area) {
    return { name: name, type: "line", showSymbol: false, smooth: true,
             lineStyle: { width: 1.8, color: color }, itemStyle: { color: color },
             areaStyle: area ? { opacity: 0.12, color: color } : undefined,
             data: shiftSeries(data) };
  }
  function bar(name, color, data) {
    return { name: name, type: "bar", itemStyle: { color: color }, data: shiftSeries(data) };
  }
  // Draw a chart with its unit on the y-axis and tooltip. Hidden if no data.
  function draw(id, title, unit, series) {
    var c = chart(id);
    if (!c) return;
    var has = series.some(function (s) { return nonEmpty(s.data); });
    if (!has) { c.getDom().style.display = "none"; return; }
    var w = c.getDom().clientWidth;
    c.setOption(Object.assign(baseOpt(title, unit, w), { series: series }), true);
    redraw[id] = function () {
      c.setOption(baseOpt(title, unit, c.getDom().clientWidth), false);
    };
  }

  function render(d) {
    if (!d) return;
    // Pin the time axis to the day's midnight bounds when provided (UTC epochs
    // of the station's local midnight, computed server-side). Shift into the
    // station-clock domain so they line up with the shifted data points.
    axisMin = (typeof d.start === "number") ? toStationClock(d.start) : null;
    axisMax = (typeof d.end === "number") ? toStationClock(d.end) : null;

    // On a re-render (e.g. language switch), rebuild charts that are already
    // built with the new closure; charts not yet seen stay deferred. defer()
    // below re-registers each builder, so just clear the built-flags of the
    // ones already drawn and re-run them after registration.
    var wasBuilt = Object.keys(built);
    built = {};

    // Temperature + derived
    defer("chart-temp", function () {
      var ts = [];
      var custom = window.AGANETWX_TEMP_SERIES;
      if (custom && custom.length) {
        // Config-chosen series (Extras.temp_chart_series), in order. Built-in
        // temperatures come from d.<obs>; extra sensors from d.extra.<obs>,
        // labelled from AGANETWX_EXTRA_LABELS. A small palette keeps extra
        // lines distinct.
        // Built-in temperature obs -> its COLORS key and i18n label key. The
        // outTemp obs is keyed "temp" in both maps, so look it up here rather
        // than by the obs name (COLORS["outTemp"] does not exist).
        var BUILTIN_TEMP = {
          outTemp:   "temp",
          dewpoint:  "dewpoint",
          appTemp:   "appTemp",
          heatindex: "heatindex",
          windchill: "windchill"
        };
        var labels = window.AGANETWX_EXTRA_LABELS || {};
        var extraColors = ["#8e44ad", "#16a085", "#d35400", "#2c3e50", "#c0392b"];
        var ei = 0;
        custom.forEach(function (name) {
          var key = BUILTIN_TEMP[name];
          if (key && nonEmpty(d[name])) {
            ts.push(line(t(key), COLORS[key], d[name], name === "outTemp"));
          } else if (d.extra && nonEmpty(d.extra[name])) {
            var lbl = decode(labels[name] || name);
            ts.push(line(lbl, extraColors[ei++ % extraColors.length], d.extra[name]));
          }
        });
      } else {
        if (nonEmpty(d.outTemp))   ts.push(line(t("temp"), COLORS.temp, d.outTemp, true));
        if (nonEmpty(d.dewpoint))  ts.push(line(t("dewpoint"), COLORS.dewpoint, d.dewpoint));
        if (nonEmpty(d.appTemp))   ts.push(line(t("appTemp"), COLORS.appTemp, d.appTemp));
        if (nonEmpty(d.heatindex)) ts.push(line(t("heatindex"), COLORS.heatindex, d.heatindex));
        if (nonEmpty(d.windchill)) ts.push(line(t("windchill"), COLORS.windchill, d.windchill));
      }
      draw("chart-temp", t("temp"), u("temp", "°C"), ts);
    });

    defer("chart-humidity", function () {
      draw("chart-humidity", t("humidity"), "%",
           [line(t("humidity"), COLORS.humidity, d.outHumidity, true)]);
    });

    defer("chart-pressure", function () {
      draw("chart-pressure", t("pressure"), u("pressure", "hPa"),
           [line(t("pressure"), COLORS.pressure, d.barometer)]);
    });

    defer("chart-windspeed", function () {
      draw("chart-windspeed", t("windSpeed"), u("wind", "km/h"),
           [line(t("windSpeed"), COLORS.windSpeed, d.windSpeed, true),
            line(t("windGust"), COLORS.windGust, d.windGust)]);
    });

    // Wind direction scatter (bearing 0..360). Exact degree shown on hover.
    defer("chart-windvec", function () {
    var wv = chart("chart-windvec");
    if (wv) {
      if (nonEmpty(d.windDir)) {
        wv.setOption({
          useUTC: true,
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
                     itemStyle: { color: COLORS.windvec }, data: shiftSeries(d.windDir) }]
        }, true);
      } else { wv.getDom().style.display = "none"; }
    }
    });

    // Wind vector: arrow per point, y = speed, rotated to direction, thinned to avoid overlap.
    defer("chart-windvector", function () {
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
          pts.push([toStationClock(ts), spd, deg]);
        }
        wvec.setOption({
          useUTC: true,
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
    });

    defer("chart-windrose", function () { windRose(d); });

    defer("chart-rain", function () {
      draw("chart-rain", t("rain"), u("rain", "mm"), [bar(t("rain"), COLORS.rain, d.rain)]);
    });
    defer("chart-rainrate", function () {
      draw("chart-rainrate", t("rainRate"), u("rainRate", "mm/h"), [line(t("rainRate"), COLORS.rainRate, d.rainRate, true)]);
    });
    defer("chart-uv", function () {
      draw("chart-uv", t("UV"), "", [line(t("UV"), COLORS.UV, d.UV, true)]);
    });
    defer("chart-radiation", function () {
      draw("chart-radiation", t("radiation"), u("radiation", "W/m²"), [line(t("radiation"), COLORS.radiation, d.radiation, true)]);
    });
    defer("chart-et", function () {
      draw("chart-et", t("ET"), u("ET", "mm"), [bar(t("ET"), COLORS.ET, d.ET)]);
    });
    defer("chart-cloudbase", function () {
      draw("chart-cloudbase", t("cloudbase"), u("cloudbase", "m"), [line(t("cloudbase"), COLORS.cloudbase, d.cloudbase)]);
    });

    // Re-run any chart that was already built before this render (a re-render
    // from a language switch), so it redraws with the new labels immediately.
    wasBuilt.forEach(runBuilder);
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
      if (charts[k].getDom().style.display === "none") return;
      if (redraw[k]) redraw[k]();   // re-place legend/grid for the new width
      charts[k].resize();
    });
  });

  // Realistic moon from data-fullness (0..100). A textured lit disc (warm
  // moonlight + a few maria) is drawn full, then the un-lit part is covered by
  // a shadow whose curved edge (the terminator) is a half-circle + an ellipse.
  // The lit side is on the right when waxing, on the left when waning, matching
  // the real sky. Reads clearly at every phase.
  function paintOneMoon(m, idx) {
    var f = Math.max(0, Math.min(100, parseFloat(m.getAttribute("data-fullness")) || 0));
    var waning = m.getAttribute("data-waning") === "1";
    var k = f / 100;
    var R = 50, C = 50;
    var cid = "mc" + idx, tex = "mt" + idx, sh = "ms" + idx;

    // Terminator ellipse half-width (0 at quarter, ->R near new/full).
    var rx = R * Math.abs(1 - 2 * k);
    // Which vertical half is lit: waxing lights the RIGHT, waning the LEFT.
    var litRight = !waning;
    // The terminator ellipse takes the LIT colour for a gibbous moon (k>0.5,
    // lit past the meridian) and the DARK colour for a crescent (k<0.5).
    var ellipseLit = (k >= 0.5);

    var svg =
      '<svg viewBox="0 0 100 100" width="100%" height="100%" style="display:block">' +
      '<defs>' +
        '<clipPath id="' + cid + '"><circle cx="50" cy="50" r="50"/></clipPath>' +
        // Warm moonlight sphere: bright near top-left, gently darker at the limb.
        '<radialGradient id="' + tex + '" cx="38%" cy="34%" r="72%">' +
          '<stop offset="0%" stop-color="#fdfdf5"/>' +
          '<stop offset="70%" stop-color="#e9ebf0"/>' +
          '<stop offset="100%" stop-color="#c7ccd6"/>' +
        '</radialGradient>' +
        // Soft edge for the terminator so it is not a hard line.
        '<filter id="' + sh + '" x="-20%" y="-20%" width="140%" height="140%">' +
          '<feGaussianBlur stdDeviation="1.4"/>' +
        '</filter>' +
      '</defs>' +
      '<g clip-path="url(#' + cid + ')">' +
        // Lit disc + subtle maria (the "man in the moon" dark patches).
        '<circle cx="50" cy="50" r="50" fill="url(#' + tex + ')"/>' +
        '<g fill="#b9bfca" opacity="0.55">' +
          '<ellipse cx="38" cy="34" rx="11" ry="9"/>' +
          '<ellipse cx="58" cy="40" rx="8" ry="7"/>' +
          '<ellipse cx="46" cy="58" rx="13" ry="10"/>' +
          '<circle cx="66" cy="63" r="4"/>' +
          '<circle cx="30" cy="55" r="3"/>' +
        '</g>' +
        // A couple of tiny bright-rimmed craters for texture.
        '<g fill="#d8dce3" opacity="0.5">' +
          '<circle cx="70" cy="30" r="3.5"/><circle cx="26" cy="40" r="2.5"/>' +
        '</g>';
    // Overlay the dark (un-lit) part. Full moon: nothing. New moon: all dark.
    // Otherwise: the dark vertical half, plus the terminator ellipse painted
    // dark for a crescent (the lit region is a sliver) so it reads correctly.
    var dark = "#151a24";
    if (f <= 0.5) {
      svg += '<circle cx="50" cy="50" r="50" fill="' + dark + '"/>';
    } else if (f < 99.5) {
      // Dark half is the side opposite the lit half.
      if (litRight) {
        svg += '<rect x="0" y="0" width="50" height="100" fill="' + dark + '"/>';
      } else {
        svg += '<rect x="50" y="0" width="50" height="100" fill="' + dark + '"/>';
      }
      // Terminator ellipse: dark when crescent (extends shadow across meridian),
      // lit when gibbous (extends light across meridian, so paint it lit).
      var ellFill = ellipseLit ? "url(#" + tex + ")" : dark;
      svg += '<ellipse cx="50" cy="50" rx="' + rx + '" ry="50" fill="' + ellFill + '" filter="url(#' + sh + ')"/>';
    }
    svg += '</g></svg>';
    m.innerHTML = svg;
    m.style.overflow = "hidden";
    m.style.borderRadius = "50%";
    // Inner spherical shading (inside the clipped disc) plus a soft dark halo
    // around it for contrast against the light card. box-shadow's outer part is
    // clipped by overflow:hidden, so the outer halo goes via drop-shadow, which
    // renders after the clip and hugs the circle.
    m.style.boxShadow = "inset 0 0 10px rgba(0,0,0,.45)";
    m.style.filter =
      "drop-shadow(0 2px 5px rgba(0,0,0,.5)) drop-shadow(0 0 9px rgba(0,0,0,.35))";
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

  // Theme switcher: Modern / Classic / Dark presets. The active preset is
  // applied before paint by the inline head script; here we sync the dropdown
  // and switch live (mode + layout), persisting the choice, and re-tint charts.
  function initTheme() {
    var sel = document.getElementById("theme-select");
    if (!sel) return;
    var PRESETS = {
      modern:  { mode: "light", layout: "modern"  },
      classic: { mode: "light", layout: "classic" },
      dark:    { mode: "dark",  layout: "modern"  }
    };
    var cur = document.documentElement.getAttribute("data-preset") || "modern";
    if (!PRESETS[cur]) cur = "modern";
    sel.value = cur;
    sel.addEventListener("change", function () {
      var p = PRESETS[sel.value] ? sel.value : "modern";
      var d = document.documentElement;
      d.setAttribute("data-theme", PRESETS[p].mode);
      d.setAttribute("data-preset", p);
      d.setAttribute("data-layout", PRESETS[p].layout);
      document.body.classList.remove("layout-modern", "layout-classic");
      document.body.classList.add("layout-" + PRESETS[p].layout);
      try { localStorage.setItem("aganetwx_theme", p); } catch (e) {}
      retint();   // merge new theme colors into the live charts, no full rebuild
    });
  }
  initTheme();

  // Live webcams: reload each image on an interval with a cache-busting query
  // so the browser always fetches the current frame. Each cam hides itself if
  // its image fails to load (camera offline). One shared lightbox serves all.
  function initWebcams() {
    var imgs = document.querySelectorAll(".webcam-img");
    if (!imgs.length) return;
    var box = document.getElementById("webcam-lightbox");
    var big = box ? box.querySelector(".webcam-lightbox-img") : null;
    var closeBtn = box ? box.querySelector(".webcam-lightbox-close") : null;
    var lastFocus = null;

    function openBox(fromImg) {
      if (!box || !big) return;
      lastFocus = fromImg;
      big.src = fromImg.currentSrc || fromImg.src;   // the frame currently shown
      box.hidden = false;
      document.body.style.overflow = "hidden";
      if (closeBtn) closeBtn.focus();
    }
    function closeBox() {
      if (!box) return;
      box.hidden = true;
      document.body.style.overflow = "";
      if (lastFocus) lastFocus.focus();
    }
    if (box) {
      if (closeBtn) closeBtn.addEventListener("click", closeBox);
      box.addEventListener("click", function (e) { if (e.target === box) closeBox(); });
      document.addEventListener("keydown", function (e) {
        if (e.key === "Escape" && !box.hidden) closeBox();
      });
    }

    Array.prototype.forEach.call(imgs, function (img) {
      initOneCam(img, openBox);
    });
  }

  function initOneCam(img, openBox) {
    var src = img.getAttribute("data-src");
    var every = parseInt(img.getAttribute("data-refresh"), 10) || 0;
    // Countdown and wrap are scoped to this cam's own card.
    var card = img.closest(".webcam-box") || img.parentNode;
    var countEl = card ? card.querySelector(".webcam-count") : null;
    var wrap = img.closest(".webcam-wrap") || card;
    var loadedOnce = false;
    var failCount = 0;
    function bust() {
      img.src = src + (src.indexOf("?") >= 0 ? "&" : "?") + "_t=" + (new Date()).getTime();
    }
    // A failed frame is transient: once any frame has loaded the cam stays
    // visible (a later failure keeps the last good frame, never blanks it).
    // Before the first success we tolerate a few misses so a single hiccup
    // does not hide it, then hide only if it truly never loads. Either way we
    // keep retrying so it self-heals when the camera returns.
    img.addEventListener("load", function () {
      loadedOnce = true; failCount = 0; if (wrap) wrap.style.display = "";
    });
    img.addEventListener("error", function () {
      if (!loadedOnce && ++failCount >= 3 && wrap) wrap.style.display = "none";
      setTimeout(bust, 5000);
    });
    // Reload the frame only when it actually changed: a lightweight HEAD request
    // reads Last-Modified/ETag and we swap the image only if it differs. Saves
    // re-downloading identical frames. If HEAD is unavailable (CORS/older
    // server), fall back to reloading every tick.
    var lastTag = null;
    var headOk = true;
    function refreshIfNewer() {
      if (!headOk) { bust(); return; }
      fetch(src, { method: "HEAD", cache: "no-store" }).then(function (r) {
        if (!r.ok) throw 0;
        var tag = r.headers.get("Last-Modified") || r.headers.get("ETag");
        if (tag === null) { headOk = false; bust(); return; }  // header not exposed
        if (tag !== lastTag) { lastTag = tag; bust(); }
      }).catch(function () { headOk = false; bust(); });  // CORS/network: just reload
    }
    // Let the plain src="..." already in the HTML load as the first frame (it is
    // the freshest one right after generation); only cache-bust on later ticks.
    if (every > 0) {
      var left = every;
      // Stop auto-refreshing after this many seconds so a forgotten/idle tab
      // does not poll forever. A manual page reload starts a fresh session.
      var stopAfter = (parseInt(img.getAttribute("data-refresh-stop"), 10) || 3600);
      var elapsed = 0;
      var timer = setInterval(function () {
        elapsed += 1;
        if (elapsed >= stopAfter) {
          clearInterval(timer);
          if (countEl) {
            var badge = countEl.parentNode;   // the .webcam-live pill
            if (badge) badge.textContent = t("Paused");
          }
          return;
        }
        left -= 1;
        if (left <= 0) { refreshIfNewer(); left = every; }
        if (countEl) countEl.textContent = left;
      }, 1000);
    }

    // Click-to-enlarge (lightbox), when the image is not a click-through link.
    if (img.classList.contains("webcam-zoomable") && openBox) {
      img.addEventListener("click", function () { openBox(img); });
      img.addEventListener("keydown", function (e) {
        if (e.key === "Enter" || e.key === " ") { e.preventDefault(); openBox(img); }
      });
    }
  }
  initWebcams();

  // Stale-data banner: compare the latest observation's epoch (absolute, so it
  // is timezone-independent) to the visitor's clock, and warn if the station
  // has not reported for longer than the configured limit. Works even on a
  // frozen page, since the check runs in the browser each load/tick.
  function initStale() {
    var banner = document.getElementById("stale-banner");
    var last = window.AGANETWX_LAST || 0;
    var limit = window.AGANETWX_STALE_AFTER || 0;
    if (!banner || !last || !limit) return;
    var ageEl = document.getElementById("stale-age");
    function fmtAge(secs) {
      var d = Math.floor(secs / 86400), h = Math.floor((secs % 86400) / 3600),
          m = Math.floor((secs % 3600) / 60);
      if (d) return d + "d " + h + "h";
      if (h) return h + "h " + m + "m";
      return m + "m";
    }
    function check() {
      var age = Math.floor(Date.now() / 1000) - last;
      if (age >= limit) {
        if (ageEl) ageEl.textContent = " (" + fmtAge(age) + ")";
        banner.hidden = false;
      } else {
        banner.hidden = true;
      }
    }
    check();
    setInterval(check, 60000);
  }
  initStale();

  // Temperature-reactive hero: tint the hero from cold (blue) to hot (red) by
  // interpolating hue across the configured range. Unit-independent (reads the
  // raw Celsius value emitted server-side). Only runs when hero-dynamic is set.
  function initHeroColor() {
    var el = document.querySelector(".hero-dynamic");
    if (!el) return;
    var t = parseFloat(el.getAttribute("data-temp-c"));
    if (isNaN(t)) return;
    // Temperature-keyed color stops (degC -> RGB): bright ice-cyan only at true
    // freezing, a solid blue through the comfortable range, then amber, orange,
    // and deep red as it gets hot. No green. Stops are absolute so a given
    // temperature always maps to the same color.
    var stops = [
      [-10, [150, 222, 246]], [3, [120, 200, 235]], [8, [41, 120, 190]],
      [20, [30, 96, 170]], [25, [42, 104, 170]], [28, [150, 170, 175]],
      [31, [236, 196, 110]], [35, [240, 158, 66]], [40, [224, 96, 48]],
      [45, [176, 28, 40]], [50, [150, 20, 34]]
    ];
    var c = stops[stops.length - 1][1];
    for (var i = 0; i < stops.length - 1; i++) {
      var a = stops[i], b = stops[i + 1];
      if (t <= b[0]) {
        var u = Math.max(0, Math.min(1, (t - a[0]) / ((b[0] - a[0]) || 1)));
        c = a[1].map(function (ca, j) { return Math.round(ca + (b[1][j] - ca) * u); });
        break;
      }
    }
    var d = c.map(function (x) { return Math.round(x * 0.82); });
    var rgb = function (v) { return "rgb(" + v[0] + "," + v[1] + "," + v[2] + ")"; };
    el.style.background = "linear-gradient(160deg, " + rgb(c) + " 0%, " + rgb(d) + " 100%)";
    // Dark text on a light (cold) background, white on a dark one, for contrast.
    var lum = 0.299 * c[0] + 0.587 * c[1] + 0.114 * c[2];
    el.classList.toggle("hero-ink-dark", lum > 150);
  }
  initHeroColor();

  // Compare page: overlay a chosen metric across every year, for a chosen month
  // (day-by-day). Reads data/compare.json (built server-side). x-axis is the
  // day of the month (1..31), so no timezone handling is needed.
  function initCompare() {
    var host = document.getElementById("cmpx-chart");
    if (!host) return;
    var monthSel = document.getElementById("cmpx-month");
    var metricSel = document.getElementById("cmpx-metric");
    var empty = document.getElementById("cmpx-empty");
    var yearList = document.getElementById("cmpx-year-list");
    var btnAll = document.getElementById("cmpx-all");
    var btnNone = document.getElementById("cmpx-none");
    var unitFor = {
      temp: u("temp", "°C"), rain: u("rain", "mm"), wind: u("wind", "km/h"),
      humidity: "%", pressure: u("pressure", "hPa"), uv: "", radiation: u("radiation", "W/m²")
    };
    var chart = null, DB = null;
    var selected = {};       // year -> shown?
    var mode = "month";      // "month" (days across years) | "year" (months)
    var monthField = document.getElementById("cmpx-month-field");
    var modeMonthBtn = document.getElementById("cmpx-mode-month");
    var modeYearBtn = document.getElementById("cmpx-mode-year");
    var MONTHS_SHORT = ["Jan", "Feb", "Mar", "Apr", "May", "Jun",
                        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"];

    // Default the month dropdown to the current month.
    var thisMonth = ("0" + (new Date().getMonth() + 1)).slice(-2);
    if (monthSel) monthSel.value = thisMonth;

    // Each year gets its own distinct, fixed hue so lines are easy to tell
    // apart and the colour matches the swatch in the year list. The colour is
    // keyed to the year's position in the full (newest-first) list, so toggling
    // years on and off never recolours the others. The newest year (index 0)
    // still stands out via a heavier line width in draw().
    // A colourblind-friendly categorical palette; cycles if there are more
    // years than colours.
    var YEAR_PALETTE = [
      "#e6194b", "#3cb44b", "#4363d8", "#f58231", "#911eb4", "#42a5c8",
      "#bcbd22", "#f032e6", "#008080", "#9a6324", "#808000", "#3d5afe"
    ];
    var yearColour = {};   // year -> fixed hex, filled by buildYearBoxes
    function colourFor(yr) {
      return yearColour[yr] || YEAR_PALETTE[0];
    }

    function shownYears() {
      return DB.years.filter(function (y) { return selected[y]; });
    }

    function draw() {
      if (!DB || !DB.years || !DB.years.length) { host.style.display = "none"; if (empty) empty.hidden = false; return; }
      var metric = metricSel.value;
      var yearData;   // year -> array of values
      if (mode === "year") {
        yearData = DB.data[metric + "_year"] || {};
      } else {
        yearData = (DB.data[metric] || {})[monthSel.value] || {};
      }
      // Newest first so rank 0 = most recent.
      var years = shownYears().slice().sort().reverse();
      var series = [];
      years.forEach(function (yr, rank) {
        var arr = yearData[yr];
        if (!arr) return;
        var pts = [];
        for (var i = 0; i < arr.length; i++) if (arr[i] != null) pts.push([i + 1, arr[i]]);
        if (!pts.length) return;
        var newest = (rank === 0);
        series.push({
          name: yr, type: "line", showSymbol: mode === "year", symbolSize: 4,
          smooth: true, connectNulls: true, z: newest ? 10 : 1,
          lineStyle: { width: newest ? 3 : 1.8, color: colourFor(yr) },
          itemStyle: { color: colourFor(yr) }, data: pts
        });
      });
      if (!series.length) { host.style.display = "none"; if (empty) empty.hidden = false; return; }
      host.style.display = ""; if (empty) empty.hidden = true;
      if (!chart) chart = echarts.init(host);
      var un = unitFor[metric] || "";
      var suffix = un ? (un.charAt(0) === "°" || un === "%" ? un : " " + un) : "";
      var xAxis = (mode === "year")
        ? { type: "category", boundaryGap: false,
            data: MONTHS_SHORT.map(function (m) { return decode(tr(m) || m); }),
            axisLabel: { fontSize: 10, color: AX }, splitLine: { lineStyle: { color: GRID } } }
        : { type: "value", min: 1, max: 31, minInterval: 1,
            axisLabel: { fontSize: 10, color: AX }, splitLine: { lineStyle: { color: GRID } } };
      // In year mode the point x is 1..12; map to category index 0..11.
      if (mode === "year") {
        series.forEach(function (s) { s.data = s.data.map(function (p) { return [p[0] - 1, p[1]]; }); });
      }
      chart.setOption({
        tooltip: { trigger: "axis", valueFormatter: function (v) { return v + suffix; } },
        legend: { show: false },
        grid: { left: 56, right: 16, top: 12, bottom: 30 },
        xAxis: xAxis,
        yAxis: { type: "value", scale: true,
                 axisLabel: { fontSize: 10, color: AX, formatter: function (v) { return v + suffix; } },
                 splitLine: { lineStyle: { color: GRID } } },
        series: series
      }, true);
    }

    function setMode(m) {
      mode = m;
      if (modeMonthBtn) modeMonthBtn.classList.toggle("cmpx-mode-on", m === "month");
      if (modeYearBtn) modeYearBtn.classList.toggle("cmpx-mode-on", m === "year");
      if (monthField) monthField.style.display = (m === "year") ? "none" : "";
      draw();
    }

    function buildYearBoxes() {
      yearList.innerHTML = "";
      yearColour = {};
      DB.years.slice().sort().reverse().forEach(function (yr, i) {
        selected[yr] = true;
        yearColour[yr] = YEAR_PALETTE[i % YEAR_PALETTE.length];
        var id = "cmpx-y-" + yr;
        var lab = document.createElement("label");
        lab.className = "cmpx-year";
        lab.innerHTML = '<input type="checkbox" id="' + id + '" checked>' +
          '<span class="cmpx-swatch" style="background:' + yearColour[yr] + '"></span>' + yr;
        lab.querySelector("input").addEventListener("change", function (e) {
          selected[yr] = e.target.checked; draw();
        });
        yearList.appendChild(lab);
      });
    }
    function setAll(on) {
      DB.years.forEach(function (y) { selected[y] = on; });
      Array.prototype.forEach.call(yearList.querySelectorAll("input"), function (cb) { cb.checked = on; });
      draw();
    }

    // Monthly all-time records tables (optional; only when the block exists).
    function buildRecords() {
      var host = document.getElementById("cmpx-records");
      if (!host || !DB || !DB.data || !DB.data.records) return;
      var rec = DB.data.records;
      var tUnit = unitFor.temp || "";
      var rUnit = unitFor.rain || "";
      // Full date for the hover title, in the station's rendered locale.
      function dstr(ts) {
        try { return new Date(ts * 1000).toLocaleDateString(); }
        catch (e) { return ""; }
      }
      // A cell showing "value (year)" with the exact date on hover.
      function cell(val, unit, tag, title) {
        if (val == null) return "<td>-</td>";
        var suffix = (unit && unit.charAt(0) !== "°" && unit !== "%") ? " " + unit : (unit || "");
        var t = title ? ' title="' + title + '"' : "";
        return "<td" + t + ">" + val + suffix +
               (tag ? ' <span class="cmpx-rec-tag">(' + tag + ")</span>" : "") + "</td>";
      }
      function monthName(i) { return decode(tr(MONTHS_SHORT[i]) || MONTHS_SHORT[i]); }

      if (host.getAttribute("data-temp") === "1" && rec.temp && Object.keys(rec.temp).length) {
        var block = document.getElementById("cmpx-rec-temp");
        var body = block.querySelector("tbody");
        var rows = "";
        for (var m = 1; m <= 12; m++) {
          var r = rec.temp[("0" + m).slice(-2)];
          if (!r) continue;
          var hiYr = r.hi_ts ? new Date(r.hi_ts * 1000).getFullYear() : "";
          var loYr = r.lo_ts ? new Date(r.lo_ts * 1000).getFullYear() : "";
          rows += "<tr><td>" + monthName(m - 1) + "</td>" +
                  cell(r.hi, tUnit, hiYr, r.hi_ts ? dstr(r.hi_ts) : "") +
                  cell(r.lo, tUnit, loYr, r.lo_ts ? dstr(r.lo_ts) : "") + "</tr>";
        }
        if (rows) { body.innerHTML = rows; block.hidden = false; }
      }

      if (host.getAttribute("data-rain") === "1" && rec.rain && Object.keys(rec.rain).length) {
        var rblock = document.getElementById("cmpx-rec-rain");
        var rbody = rblock.querySelector("tbody");
        var rrows = "";
        for (var mm = 1; mm <= 12; mm++) {
          var rr = rec.rain[("0" + mm).slice(-2)];
          if (!rr) continue;
          rrows += "<tr><td>" + monthName(mm - 1) + "</td>" +
                   cell(rr.max, rUnit, rr.max_yr, "") +
                   cell(rr.avg, rUnit, "", "") + "</tr>";
        }
        if (rrows) { rbody.innerHTML = rrows; rblock.hidden = false; }
      }
    }

    fetch("data/compare.json", { cache: "no-store" })
      .then(function (r) { return r.json(); })
      .then(function (d) { DB = d; buildYearBoxes(); draw(); buildRecords(); })
      .catch(function (e) { console.error("AganetWX: compare data failed", e); });
    monthSel.addEventListener("change", draw);
    metricSel.addEventListener("change", draw);
    if (btnAll) btnAll.addEventListener("click", function () { setAll(true); });
    if (btnNone) btnNone.addEventListener("click", function () { setAll(false); });
    if (modeMonthBtn) modeMonthBtn.addEventListener("click", function () { setMode("month"); });
    if (modeYearBtn) modeYearBtn.addEventListener("click", function () { setMode("year"); });
    window.addEventListener("resize", function () { if (chart) chart.resize(); });
  }
  initCompare();
})();

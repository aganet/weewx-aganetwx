## Shared head, header bar and nav. Caller sets $active; $lbl must be defined.
#encoding UTF-8
## Theme settings, with defaults so a partial [[[Extras]]] still works.
#set $th_mode    = $getVar('Extras.theme.mode', 'light')
#set $th_gtop    = $getVar('Extras.theme.gradient_top', '#FDDA74')
#set $th_gbot    = $getVar('Extras.theme.gradient_bottom', '#FFFDCA')
#set $th_bg      = $getVar('Extras.theme.page_bg', '#FFFDCA')
#set $th_accent  = $getVar('Extras.theme.accent', '#0a5ca8')
#set $th_font    = $getVar('Extras.theme.font', 'Verdana, Geneva, sans-serif')
#set $th_fsize   = $getVar('Extras.theme.base_font_size', '9pt')
#set $th_density = $getVar('Extras.theme.density', 'comfortable')
#set $th_layout  = $getVar('Extras.theme.layout', 'classic')
#set $th_dbg     = $getVar('Extras.theme.dark_bg', '#16202b')
#set $th_dpanel  = $getVar('Extras.theme.dark_panel', '#1f2c3a')
#set $th_dink    = $getVar('Extras.theme.dark_ink', '#e6edf3')
#set $th_dgtop   = $getVar('Extras.theme.dark_gradient_top', '#243447')
#set $th_dgbot   = $getVar('Extras.theme.dark_gradient_bottom', '#1a2531')
## Modern layout uses a neutral gray page bg unless the user set a custom one.
#if $th_layout == 'modern' and $th_bg in ('#FFFDCA', '#fffdca')
#set $th_bg = '#eef1f5'
#end if
<!DOCTYPE html>
<html lang="$lang">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>$station.location</title>
  <meta name="description" content="$station.location: $gettext('Meta description')">
  <link rel="icon" type="image/svg+xml" href="favicon.svg">
  <link rel="icon" href="favicon.ico" sizes="any">
  <link rel="apple-touch-icon" href="apple-touch-icon.png">
  <meta name="theme-color" content="$th_accent">
  <link rel="stylesheet" href="aganetwx.css">
## Auto-refresh: reload after the archive interval (+60s buffer) so the page
## follows new data. Extras.auto_refresh: "auto" (default), seconds, or off.
#set $ar = $str($getVar('Extras.auto_refresh', 'auto')).strip().lower()
#if $ar not in ('off', 'false', '0', 'no', '')
#if $ar == 'auto'
#try
#set $ar_secs = $int($current.interval.raw) * 60 + 60
#except
#set $ar_secs = 360
#end try
#else
#set $ar_secs = $int($ar)
#end if
  <script>
    // Auto-reload to follow new data, but stop after ~60 min so a forgotten,
    // idle tab does not reload forever. Elapsed time accrues across auto-reloads
    // (sessionStorage); a manual reload/navigation starts a fresh 60-min budget.
    (function () {
      var interval = $ar_secs * 1000;
      var budget = 3600 * 1000;               // stop auto-reloading after 60 min
      var K = "aganetwx_ar_elapsed", F = "aganetwx_ar_auto";
      var elapsed = 0;
      try {
        if (sessionStorage.getItem(F) === "1") {   // this load was our auto-reload
          elapsed = parseInt(sessionStorage.getItem(K) || "0", 10) || 0;
        } else {
          sessionStorage.setItem(K, "0");           // manual load: reset the budget
        }
        sessionStorage.removeItem(F);
      } catch (e) {}
      if (elapsed + interval <= budget) {
        setTimeout(function () {
          try {
            sessionStorage.setItem(K, String(elapsed + interval));
            sessionStorage.setItem(F, "1");
          } catch (e) {}
          location.reload();
        }, interval);
      }
    })();
  </script>
#end if
  <style>
    :root {
      --m-gradient-top: $th_gtop;
      --m-gradient-bottom: $th_gbot;
      --m-page-bg: $th_bg;
      --m-accent: $th_accent;
      --m-font: $th_font;
      --m-base-size: $th_fsize;
    }
    html[data-theme="dark"] {
      --m-gradient-top: $th_dgtop;
      --m-gradient-bottom: $th_dgbot;
      --m-page-bg: $th_dbg;
      --m-panel-bg: $th_dpanel;
      --m-ink: $th_dink;
    }
  </style>
  <script src="lib/echarts.min.js" defer></script>
#if $getVar('i18n_enabled', False)
  <script>
    // All enabled languages' strings, for the client-side switcher. The page is
    // server-rendered in $lang; the switcher swaps labels with no reload.
    window.AGANETWX_LANG = "$lang";
    window.AGANETWX_I18N_ALL = $i18n_json;
  </script>
#end if
  <script>
    // Resolve the active theme before paint to avoid a flash. A visitor's saved
    // preset (localStorage) wins over the configured default. Presets map to a
    // color mode (data-theme) and a layout; the layout class is applied to
    // <body> as soon as it parses (see the inline script right after <body>).
    (function(){
      var PRESETS = {
        modern:  { mode: "light", layout: "modern"  },
        classic: { mode: "light", layout: "classic" },
        dark:    { mode: "dark",  layout: "modern"  }
      };
      var def = "$th_layout" === "classic" ? "classic" : ("$th_mode" === "dark" ? "dark" : "modern");
      var choice = null;
      try { choice = localStorage.getItem("aganetwx_theme"); } catch (e) {}
      if (!choice || !PRESETS[choice]) choice = def;
      var m = PRESETS[choice].mode;
      if ("$th_mode" === "auto" && choice === def && def !== "dark") {
        m = (window.matchMedia && window.matchMedia("(prefers-color-scheme: dark)").matches) ? "dark" : "light";
      }
      var d = document.documentElement;
      d.setAttribute("data-theme", m);
      d.setAttribute("data-preset", choice);
      d.setAttribute("data-layout", PRESETS[choice].layout);
    })();
  </script>
</head>
<body class="lang-$lang density-$th_density layout-$th_layout" data-lang="$lang">
  <script>
    // Sync the layout class from the resolved preset (set in <head>) before paint.
    (function(){
      var lay = document.documentElement.getAttribute("data-layout");
      if (lay) { document.body.classList.remove("layout-modern", "layout-classic");
                 document.body.classList.add("layout-" + lay); }
    })();
  </script>

  <div class="header">
    <div class="gradient boxshadow">
      <div class="header_contents">
        <div class="header_top">
          <div>
            <div class="station_name">$station.location</div>
            <div class="station_sub"><span class="i18n" data-i18n="Weather Station">$gettext("Weather Station")</span></div>
            <div class="station_update"><span class="i18n" data-i18n="Latest update">$gettext('Latest update')</span>: $current.dateTime</div>
          </div>
          <div class="header_meta">
#if $tf($getVar('Extras.theme.switcher', True))
            <div class="theme-switch"><label for="theme-select" class="switch-lbl">$lbl('Theme'):</label>
              <select id="theme-select" aria-label="Theme">
                <option value="modern">$lbl('Modern')</option>
                <option value="classic">$lbl('Classic')</option>
                <option value="dark">$lbl('Dark')</option>
              </select>
            </div>
#end if
#if $getVar('i18n_enabled', False)
            <div class="lang-switch"><label for="lang-select" class="switch-lbl">$lbl('Language label'):</label>
              <select id="lang-select" aria-label="Language">
#for $l in $i18n_langs
                <option value="$l.code">$l.name</option>
#end for
              </select>
            </div>
#end if
          </div>
        </div>
      </div>
    </div>
  </div>

## Live webcam banner (aligned to content width). Toggle/configure via
## [Extras][[webcam]]. The image is cache-busted and refreshed client-side.
#set $wc = $getVar('Extras.webcam', {})
#if $getVar('show_current', False) and $wc and $tf($wc.get('enable', False)) and $wc.get('url')
  ## Effective refresh: 0 (static image, no badge) unless auto_refresh is on.
  #set $wc_refresh = int($wc.get('refresh', 0)) if $tf($wc.get('auto_refresh', True)) else 0
  #set $wc_align = str($wc.get('align', 'center')).lower()
  #if $wc_align not in ('left', 'right', 'center')
  #set $wc_align = 'center'
  #end if
  #set $wc_style = ''
  #if $wc.get('max_width')#
  #set $wc_style = $wc_style + 'max-width:' + str(int($wc.get('max_width'))) + 'px;'
  #end if
  #if $wc.get('height')#
  ## Reserve the height up front (min-height) so the image loading does not push
  ## content down (prevents cumulative layout shift).
  #set $wc_style = $wc_style + 'max-height:' + str(int($wc.get('height'))) + 'px;min-height:' + str(int($wc.get('height'))) + 'px;'
  #end if
  ## Without a click-through link, the image is click-to-enlarge (lightbox).
  #set $wc_zoom = not $wc.get('link')
  <div class="webcam-wrap wc-$wc_align">
    <div class="webcam-box"#if $wc_style# style="$wc_style"#end if#>
      #if $wc.get('link')#<a href="$wc.get('link')" target="_blank" rel="noopener">#end if#
      <img id="webcam-img" class="webcam-img#if $wc_zoom# webcam-zoomable#end if#" src="$wc.get('url')" alt="$wc.get('title', 'Webcam')"
           data-src="$wc.get('url')" data-refresh="$wc_refresh"#if $wc_zoom# tabindex="0" role="button"#end if#>
      #if $wc.get('link')#</a>#end if#
      #if $wc_refresh > 0
      <div class="webcam-live"><span class="webcam-dot"></span>$lbl('Refreshing in') <span id="webcam-count">$wc_refresh</span>$lbl('s (short for seconds)')</div>
      #end if
      #if $wc.get('title')#<div class="webcam-cap">$wc.get('title')</div>#end if#
    </div>
  </div>
  #if $wc_zoom
  <div id="webcam-lightbox" class="webcam-lightbox" hidden>
    <button type="button" class="webcam-lightbox-close" aria-label="Close">&times;</button>
    <img class="webcam-lightbox-img" src="" alt="$wc.get('title', 'Webcam')">
  </div>
  #end if
#end if

#include "_nav.inc"

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
  <script>setTimeout(function(){ location.reload(); }, $ar_secs * 1000);</script>
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
    // Resolve theme mode (light|dark|auto) before paint to avoid flash.
    (function(){
      var m = "$th_mode";
      if (m === "auto") {
        m = (window.matchMedia && window.matchMedia("(prefers-color-scheme: dark)").matches) ? "dark" : "light";
      }
      document.documentElement.setAttribute("data-theme", m);
    })();
  </script>
</head>
<body class="lang-$lang density-$th_density layout-$th_layout" data-lang="$lang">

  <div class="header">
    <div class="gradient boxshadow">
      <div class="header_contents">
        <div class="header_top">
          <div>
            <div class="station_name">$station.location</div>
            <div class="station_sub"><span class="i18n" data-i18n="Weather Station">$gettext("Weather Station")</span></div>
          </div>
          <div class="header_meta">
            <div class="hm-title">$lbl('Latest update')</div>
            <div class="hm-time">$current.dateTime</div>
#if $getVar('i18n_enabled', False)
            <div class="lang-switch">
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

#include "_nav.inc"

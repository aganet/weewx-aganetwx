# AganetWX, a WeeWX skin

A clean, fast, **fully configurable** weather dashboard skin for
[WeeWX](https://weewx.com) 5.x. After running many different skins over the
years, I built my own to get exactly the layout, data and feel I wanted.
Interactive charts, multi-period pages, sensor-agnostic, and translatable,
configured entirely from `weewx.conf`, with **no template editing required**.

Source and releases: [github.com/aganet/weewx-aganetwx](https://github.com/aganet/weewx-aganetwx)

Live demo: [aganet.gr](https://aganet.gr)

![Modern layout (default)](screenshot-modern.png)

<details>
<summary>Classic layout and dark mode (each one config line)</summary>

`layout = classic` switches to a warm, compact label-row look:

![Classic layout](screenshot-light.png)

`mode = dark`:

![Dark theme](screenshot-dark.png)
</details>

---

## Highlights

- **Multi-period pages**: Current, Yesterday, Week, Month, Year, Last Year, Archive, with a top navigation bar.
- **Interactive charts** ([Apache ECharts](https://echarts.apache.org/), self-hosted, no CDN), one per metric: temperature (with dew point, apparent, heat index, wind chill), humidity, barometer, wind speed and gust, wind direction, wind rose, rain, UV, solar radiation, evapotranspiration, cloud base.
- **Current-conditions hero**: big current temperature, feels-like, and today's high/low. By default it tints by temperature (ice-blue when freezing, through blue, amber and orange to deep red when hot) and shows a small weather-mood face with a one-word caption (Brrr! ... Perfect ... Melting!) in the selected language. Both can be turned off.
- **Trend arrows**: a small rising / steady / falling arrow next to temperature, humidity and UV, plus a 3-hour pressure tendency under the barometer.
- **Threshold highlighting**: a value's row lights up (red above `high`, blue below `low`) when it crosses per-metric limits. Ships with sensible warm-climate defaults; tune them per metric in `weewx.conf`.
- **All-time records card**: highest/lowest temperature, wind gust, humidity, barometer and rain rate ever recorded in your archive, each with the date it happened.
- **Last rain**: when it last rained and how many days ago.
- **Stale-data alert**: a banner warns when the station has not reported for over an hour (configurable), so an outage is visible on the page. Checked in the browser, so it shows even if generation has stopped, and it is timezone-independent.
- **Sensor-agnostic**: auto-discovers and displays whatever your station records (extra temp/humidity, soil, leaf, air quality, lightning, battery) with no hardcoded list. Looks complete on Davis, Ecowitt, Tempest, or a bare thermometer.
- **Config-driven theming**: colors, gradient, font, density, **light / dark / auto** mode, all from `weewx.conf`. A header switcher lets visitors pick **Modern / Classic / Dark** live (remembered per browser).
- **Multi-language with a live switcher**: English, Greek, Spanish, French, German, Italian, Portuguese included. A header dropdown swaps every label and chart instantly (no reload); the browser language is auto-picked and remembered. Add a language by dropping in one `lang/<code>.conf` file. No template edits.
- **NOAA reports**: monthly and yearly climatological text reports, linked from the Archive page.
- **About page**: station hardware, coordinates, altitude and software versions read automatically, plus editable prose and contact fields. Coordinates hide behind one toggle for privacy.
- **Optional webcam banner**: a live camera image above the navigation on the Current page. Auto-refreshes only when the frame actually changed (or stays static), shows a countdown, and click-to-enlarge opens it full-size in a lightbox. Size, position and a click-through link are configurable.
- **Useful Links card**: a small set of external links (a lightning map, Windy, a satellite view; Greece-centered by default), editable in config.
- **Optional HF propagation card**: amateur-radio solar-terrestrial data (solar flux, sunspots, A-index, K-index) and a colour-coded HF band-conditions table, fetched server-side from HamQSL. Off by default.
- **Respects your WeeWX units**: US, metric or metricwx, the skin follows your station's `unit_system` (and timezone) rather than forcing its own. Every value, chart axis and label matches the rest of your WeeWX setup. Override per-report if you want this page to differ.

## Requirements

- WeeWX **5.x** (Python 3).
- No external services. Charts and all assets are self-hosted.

## Install

Install straight from the latest release (no download step needed):

```bash
sudo weectl extension install https://github.com/aganet/weewx-aganetwx/releases/latest/download/AganetWX-1.5.3.zip
sudo systemctl restart weewx          # or: sudo /etc/init.d/weewx restart
```

Or, if you already downloaded the zip, point at its full path:

```bash
sudo weectl extension install /path/to/AganetWX-1.5.3.zip
```

This adds a `[[AganetWXReport]]` report under `[StdReport]`, installs the skin to
`skins/AganetWX`, and the sensor-discovery helper to `bin/user/aganetwx_extras.py`.
Output lands in `public_html/aganetwx/`; browse to `.../aganetwx/`.

### Running alongside another skin

WeeWX runs every enabled report under `[StdReport]`, so AganetWX can sit next to
your existing skin instead of replacing it. It writes to its own subfolder
(`HTML_ROOT = aganetwx`), so nothing collides: your current skin stays at its
usual address and AganetWX appears at `.../aganetwx/`. To make it primary
instead, set `HTML_ROOT` to your site root. To turn it off without uninstalling,
set `enable = false` in `[[AganetWXReport]]`.

### Uninstall

```bash
weectl extension uninstall AganetWX
```

## Configuration

Everything is optional. Defaults live in `skins/AganetWX/skin.conf`; override any of
them per-report in `weewx.conf` under `[StdReport] [[AganetWXReport]]` - **without
touching a template**.

> **Put your settings in `weewx.conf`, not in `skin.conf`.** An extension update
> overwrites `skin.conf`, so edits there are lost on the next upgrade. Values in
> `weewx.conf` are never touched by updates and take precedence over the skin
> defaults. Mirror the `[[[Extras]]]` structure under `[[AganetWXReport]]`.

Example:

```ini
[StdReport]
    [[AganetWXReport]]
        skin = AganetWX
        HTML_ROOT = aganetwx
        lang = en                     # default language (en el es fr de it pt)
        # unit_system inherited from your WeeWX config; set here only to differ.

        [[[Extras]]]
            languages = en, el, es, fr, de, it, pt  # in-page switcher
            extra_sensors = true      # auto-discovered sensors panel

            [[[[theme]]]]
                layout = modern       # classic (compact rows) | modern (card tiles)
                mode = dark           # light | dark | auto
                accent = "#00d8ff"
                gradient_top = "#243447"
                gradient_bottom = "#1a2531"
                font = "Verdana, Geneva, sans-serif"
                density = compact     # comfortable | compact

            [[[[nav]]]]               # show/hide period tabs
                lastyear = false
                archive = true

            [[[[charts]]]]            # show/hide individual charts
                cloudbase = false

            [[[[rows]]]]              # show/hide Current-Values rows
                heatindex = false

            [[[[branding]]]]
                show_footer_coords = true
                link_url = "https://example.com"
                link_text = "My site"
```

### Config reference

| Setting | Values | Default | Effect |
|---|---|---|---|
| `lang` | `en`,`el`,`es`,`fr`,`de`,`it`,`pt` | `en` | Server-rendered default language (loads `lang/<code>.conf`) |
| `unit_system` | `us`,`metric`,`metricwx` | (inherited) | Optional per-report override; by default follows your WeeWX config |
| `Extras.languages` | code list | all 7 | Languages in the in-page switcher; single code hides it |
| `Extras.extra_sensors` | bool | `true` | Auto-discovered extra-sensor panel |
| `Extras.hero` | bool | `true` | Current-conditions hero card (Current page) |
| `Extras.celestial` | bool | `true` | Sun and Moon card |
| `Extras.disclaimer` | bool | `true` | Amateur-station disclaimer in the footer |
| `Extras.auto_refresh` | `auto`,seconds,`off` | `auto` | Auto-reload the page to follow new data |
| `Extras.stale_alert_secs` | seconds | `3600` | Warn (banner) when the latest observation is older than this; `0` disables |
| `theme.layout` | `modern`,`classic` | `modern` | flat card-tile dashboard vs. compact rows |
| `theme.mode` | `light`,`dark`,`auto` | `light` | `auto` follows the visitor's OS preference |
| `theme.switcher` | bool | `true` | Header dropdown to switch Modern/Classic/Dark (remembered per browser) |
| `theme.hero_dynamic_color` | bool | `true` | Tint the hero by temperature: ice-blue when freezing, blue when mild, amber to deep red when hot |
| `theme.hero_mood` | bool | `true` | Weather-mood face + caption in the hero (Brrr! to Melting!) |
| `theme.accent` | CSS color | `#0a5ca8` | Chart titles, links, headings |
| `theme.gradient_top` / `gradient_bottom` | CSS color | gold/cream | Header and panel gradient |
| `theme.page_bg` | CSS color | `#FFFDCA` | Page background |
| `theme.font` | CSS font stack | Verdana | Body font |
| `theme.density` | `comfortable`,`compact` | `comfortable` | Row and chart sizing |
| `nav.<tab>` | bool | `true` | Show/hide a tab (`current`,`yesterday`,`week`,`month`,`year`,`lastyear`,`archive`,`about`) |
| `charts.<metric>` | bool | `true` | Show/hide a chart (`temp`,`humidity`,`pressure`,`windspeed`,`windvec`,`windvector`,`windrose`,`rain`,`rainrate`,`uv`,`radiation`,`et`,`cloudbase`) |
| `rows.<row>` | bool | `true` | Show/hide a Current-Values row (`humidity`,`dewpoint`,`wind`,`barometer`,`rain_today`,`rain_rate`,`rain_month`,`rain_year`,`last_rain`,`et`,`windchill`,`heatindex`,`apptemp`,`uv`,`radiation`) |
| `Extras.rows_show_range` | bool | `true` | Today's high/low (with time) beside each current value |
| `Extras.records` | bool | `true` | All-time records card (below Today's Hi/Lows) |
| `Extras.solar` | bool | `false` | HF propagation card (HamQSL solar data + band conditions) |
| `Extras.solar_timeout` | seconds | `15` | Timeout for the server-side HamQSL fetch |
| `alerts.<obs>.high` / `.low` | number | Greek-climate defaults | Highlight a Current-Values row when the value crosses this limit (in your displayed units); defaults set for temp, apparent temp, wind, humidity, rain rate, UV |
| `about.prose_en` / `prose_el` | string | empty | About-page description (inline HTML ok) |
| `about.operator` / `website_url` / `website_text` / `email` | string | empty | About-page contact fields |
| `about.hardware` | string | empty | Override the hardware label (else WeeWX's value) |
| `about.show_coordinates` | bool | `true` | Show exact coords and map link on About; `false` for privacy |
| `branding.show_footer_coords` | bool | `true` | Lat/long/altitude in footer |
| `branding.link_url` / `link_text` | string | empty | Optional footer link |
| `links.show` | bool | `true` | Show the "Useful Links" card (bottom of the left column) |
| `links.<entry>` | `url`,`text` | Greece maps | Each `[[[entry]]]` adds a link; edit/add/remove freely |
| `webcam.enable` | bool | `false` | Show the webcam banner above the nav |
| `webcam.url` | string | `cam.jpg` | Image path (relative to the site) or full URL |
| `webcam.auto_refresh` | bool | `true` | Reload the image periodically (cache-busted) with a live countdown badge; `false` for a static image |
| `webcam.refresh` | seconds | `30` | Refresh interval when `auto_refresh` is on |
| `webcam.max_width` | px | empty | Cap the image width; empty = full content width |
| `webcam.height` | px | `380` | Cap the display height |
| `webcam.align` | `left`,`center`,`right` | `center` | Position a narrower image |
| `webcam.title` / `link` | string | empty | Optional caption and click-through URL |

### Units

The skin follows your WeeWX `unit_system` (`us`, `metric`, or `metricwx`) and
does not override it. The metric systems differ: `metric` uses km/h + cm + mbar,
`metricwx` uses m/s + mm + mbar. For the everyday km/h + mm + hPa combo, set the
individual groups in your WeeWX config (not the skin):

```ini
[StdReport]
    [[Defaults]]
        unit_system = metric
        [[[Units]]]
            [[[[Groups]]]]
                group_rain = mm
                group_rainrate = mm_per_hour
                group_pressure = hPa
```

## Threshold highlighting

Give any current value high/low limits and its row lights up when it crosses
them (red above `high`, blue below `low`). Limits are in your displayed units.
The skin ships with sensible defaults for a warm Mediterranean/Greek climate
(temperature, apparent temperature, wind, humidity, rain rate, UV); override or
remove them for your own climate by editing the `[[[[alerts]]]]` block in
`weewx.conf` (delete a subsection to stop highlighting that value).

```ini
[StdReport]
    [[AganetWXReport]]
        [[[Extras]]]
            [[[[alerts]]]]
                [[[[[outTemp]]]]]
                    high = 38
                    low = 0
                [[[[[windSpeed]]]]]
                    high = 55
                [[[[[outHumidity]]]]]
                    high = 90
```

Alerts work for any observation shown in Current Values (for example `outTemp`,
`outHumidity`, `dewpoint`, `windSpeed`, `barometer`).

## All-time records

An "All-Time Records" card (below Today's Hi/Lows on the Current page) shows the
highest and lowest values ever recorded in your archive, with the date each
happened: temperature, wind gust, humidity, barometer and rain rate. Set
`Extras.records = false` to hide it. The current temperature, humidity and UV
rows also show a small rising / steady / falling trend arrow, and a "Last Rain"
row shows when it last rained and how many days ago.

## Languages

English, Greek, Spanish, French, German, Italian and Portuguese ship complete
(`en`, `el`, `es`, `fr`, `de`, `it`, `pt`). Every word, including chart labels,
lives only in `lang/<code>.conf`.

**In-page language switcher.** List the languages you want in `Extras.languages`
(default: all seven). The header then shows a language dropdown: visitors pick
their language and every label and chart relabels instantly, with no page
reload. The browser language is auto-selected on the first visit and the choice
is remembered (localStorage). All languages ride in one build (a small strings
dictionary per language, a few KB each), so there is no duplicated per-language
output. Set `lang = <code>` for the server-rendered default and fallback; leave
`Extras.languages` with a single code to pin one language and hide the switcher.

To add a language:

1. Copy `skins/AganetWX/lang/en.conf` to `skins/AganetWX/lang/<code>.conf`.
2. Translate the right-hand side of each entry under `[Texts]` and
   `[Labels][[Generic]]` (keep the left-hand keys in English). Any entry left
   untranslated falls back to English.
3. Add `<code>` to `Extras.languages` (and/or set `lang = <code>`).

No template editing. PRs adding languages are welcome.

Note on SEO: the switcher swaps labels client-side, so crawlers index the
server-rendered `lang`. For a weather station that is fine (the searchable
content, station name and location, is identical in every language). If you need
each language separately crawlable, generate one report per language into its
own folder with `hreflang` alternates instead.

## Customizing the About page

The About page shows station facts automatically and takes prose/contact from
`[Extras][[about]]`. For richer content, copy `skins/AganetWX/about.inc.example`
to `about.inc` in the same directory: when present it replaces the config prose,
accepts full HTML, and expands WeeWX tags like `$station.hardware`. Delete it to
fall back. Set `about.show_coordinates = false` to hide exact coordinates.

## Webcam

Enable a webcam banner above the navigation on the Current page (aligned to the
content width, off by default):

```ini
[StdReport]
    [[AganetWXReport]]
        [[[Extras]]]
            [[[[webcam]]]]
                enable = true
                url = "cam.jpg"       # local file or full URL
                auto_refresh = true   # reload periodically, cache-busted
                refresh = 30          # seconds
                max_width = ""        # px; empty = full content width
                height = 380          # px
                align = center        # left | center | right
                title = ""            # optional caption
                link = ""             # optional click-through URL
```

With `auto_refresh = true` the image is checked every `refresh` seconds and
reloaded only when the frame actually changed (a lightweight `Last-Modified`
check, so identical frames are not re-downloaded), with a live "Refreshing in
Ns" countdown badge. Set `auto_refresh = false` for a static image. Clicking the
image opens it full-size in a lightbox (unless a click-through `link` is set);
close with Escape, a click on the backdrop, or the close button. On phones the
banner spans the full screen width. A failed frame is treated as transient: the
banner keeps retrying and reappears on its own once the image returns, so a
brief network blip does not hide it until a reload.

Both the webcam refresh and the whole-page auto-reload **stop after about 60
minutes** so a forgotten, idle tab does not keep polling; the webcam badge then
shows "Paused". Reloading the page starts a fresh 60-minute session.

## Useful Links card

A "Useful Links" card sits at the bottom of the left column with external links
(a lightning map, Windy, and a satellite view, defaulting to Greece-wide maps).
Edit, add, remove, or hide them from `weewx.conf`, no template editing:

```ini
[StdReport]
    [[AganetWXReport]]
        [[[Extras]]]
            [[[[links]]]]
                show = true
                [[[[[lightning]]]]]
                    text = "Lightning map"
                    url = "https://www.lightningmaps.org/"
                [[[[[windy]]]]]
                    text = "Windy"
                    url = "https://www.windy.com/"
```

Each `[[[[[entry]]]]]` needs a `url` and `text`; order is the display order. Set
`show = false` to hide the card.

## HF propagation card

An optional card for amateur-radio operators showing current solar-terrestrial
conditions: solar flux, sunspot number, A-index, K-index, and a colour-coded HF
band-conditions table (80m-40m, 30m-20m, 17m-15m, 12m-10m; day and night;
Good/Fair/Poor). Data comes from [HamQSL](https://www.hamqsl.com) (N0NBH).

It is fetched server-side once per report cycle, so the generated page makes no
client-side external request. The last good reading is cached, so a temporary
fetch failure reuses it; the card stays hidden until the first successful fetch.
It is off by default:

```ini
[StdReport]
    [[AganetWXReport]]
        [[[Extras]]]
            solar = true
            solar_timeout = 15
```

## How it works

- **Pages** are thin templates that include shared partials (`_head.inc`,
  `_nav.inc`, `_periodbody.inc`, `_foot.inc`); each sets its WeeWX period binder.
- **Charts** read per-period JSON the skin writes each report cycle
  (`data/<period>.json`), rendered client-side by `aganetwx.js` + `lib/echarts.min.js`.
- **Extra sensors** come from a Search List Extension (`user/aganetwx_extras.py`)
  that inspects the archive schema/record at generation time.
- **Theme** variables are injected into `:root` from config by `_head.inc`; the
  CSS is fully variable-driven, so a config change re-skins everything.

## License

The skin is released under the GNU GPL v3 (same as WeeWX). Bundled
[Apache ECharts](https://echarts.apache.org/) is under the Apache-2.0 license.

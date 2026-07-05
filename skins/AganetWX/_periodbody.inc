## Shared period-page body. Caller sets $P, $period_key, $show_current, $period_title_key.
#import os
## Threshold highlighting: returns a CSS class when a current value crosses a
## configured limit. Limits come from [Extras][[alerts]][[[<obs>]]] high/low,
## expressed in the report's display units. Empty when no alert is configured or
## the value is within range. Only meaningful for live (current) values.
#def alertcls($obskey, $obs)
#set $acfg = $getVar('Extras.alerts', {}).get($obskey, None)
#if $obs.has_data and $acfg and hasattr($acfg, 'get') and $obs.raw is not None
#set $hi = $acfg.get('high', None)
#set $lo = $acfg.get('low', None)
#if $hi is not None and $obs.raw >= float($hi)# alert-high#slurp
#elif $lo is not None and $obs.raw <= float($lo)# alert-low#slurp
#end if
#end if
#end def
## Small 3-hour trend arrow (rising/steady/falling) for a value, mirroring the
## barometer trend. $tr is a $trend.<obs> object; $band is the dead-band (in the
## value's units) below which it reads as steady. Emits nothing without data.
#def trendarrow($tr, $band)
#if $tr.has_data and $tr.raw is not None
#if $tr.raw > $band
<span class="trend-arrow up" title="$gettext('Rising')">&#9650;</span>#slurp
#elif $tr.raw < -$band
<span class="trend-arrow down" title="$gettext('Falling')">&#9660;</span>#slurp
#else
<span class="trend-arrow steady" title="$gettext('Steady')">&#9644;</span>#slurp
#end if
#end if
#end def
## Station IANA timezone (e.g. Europe/Athens) for correct chart-time labels,
## from $TZ or /etc/localtime. Falls back to empty (viewer's browser zone).
#set $_tzname = $os.environ.get('TZ', '')
#if not $_tzname and $os.path.islink('/etc/localtime')
#set $_tzname = $os.readlink('/etc/localtime').split('zoneinfo/')[-1]
#end if

  <div class="content">

    ## LEFT COLUMN
    <div class="left_col">

      #if $show_current and $tf($getVar('Extras.hero', True))
      ## Hero: big current temperature, feels-like, and today's range.
      #set $feels = $current.heatindex if $current.heatindex.has_data else ($current.windchill if $current.windchill.has_data else $current.appTemp)
      ## Optional temperature-reactive hero color (cold blue -> hot red). The raw
      ## Celsius value is emitted so the JS mapping is unit-independent.
      #set $hero_dyn = $tf($getVar('Extras.theme.hero_dynamic_color', False)) and $current.outTemp.has_data
      ## Weather-mood: an icon + short caption that reacts to the temperature,
      ## in six bands from freezing to melting. Bands (degC): <=2, <=10, <=18,
      ## <=26, <=34, else. Caption keys are translated in lang/*.conf.
      #set $mood = ''
      #set $mood_key = ''
      #if $tf($getVar('Extras.theme.hero_mood', True)) and $current.outTemp.has_data
      #set $_tc = $current.outTemp.degree_C.raw
      #if $_tc <= 2
      #set $mood, $mood_key = 'ice', 'Brrr!'
      #elif $_tc <= 10
      #set $mood, $mood_key = 'cold', 'Chilly'
      #elif $_tc <= 18
      #set $mood, $mood_key = 'crisp', 'Crisp'
      #elif $_tc <= 26
      #set $mood, $mood_key = 'mild', 'Perfect'
      #elif $_tc <= 34
      #set $mood, $mood_key = 'warm', 'Sweating'
      #else
      #set $mood, $mood_key = 'hot', 'Melting!'
      #end if
      #end if
      <div class="col_sub gradient boxshadow dist hero#if $hero_dyn# hero-dynamic#end if##if $mood# has-mood#end if#"#if $hero_dyn# data-temp-c="#echo round($current.outTemp.degree_C.raw, 1)#"#end if#>
        #if $mood
        <div class="hero-mood mood-$mood">
          #if $mood == 'ice'
          <svg viewBox="0 0 44 44" aria-hidden="true"><circle cx="22" cy="22" r="19" fill="currentColor" opacity="0.18"/><circle cx="22" cy="22" r="19" fill="none" stroke="currentColor" stroke-width="2.4"/><path d="M13 17 l5 3 M18 17 l-5 3" stroke="currentColor" stroke-width="2.2" stroke-linecap="round"/><path d="M26 17 l5 3 M31 17 l-5 3" stroke="currentColor" stroke-width="2.2" stroke-linecap="round"/><path d="M15 31 q3 -3 7 0 q3 3 7 0" fill="none" stroke="currentColor" stroke-width="2.4" stroke-linecap="round"/></svg>
          #elif $mood == 'cold'
          <svg viewBox="0 0 44 44" aria-hidden="true"><circle cx="22" cy="22" r="19" fill="currentColor" opacity="0.18"/><circle cx="22" cy="22" r="19" fill="none" stroke="currentColor" stroke-width="2.4"/><circle cx="16" cy="19" r="1.8" fill="currentColor"/><circle cx="28" cy="19" r="1.8" fill="currentColor"/><path d="M15 30 q7 -4 14 0" fill="none" stroke="currentColor" stroke-width="2.4" stroke-linecap="round"/></svg>
          #elif $mood == 'crisp'
          <svg viewBox="0 0 44 44" aria-hidden="true"><circle cx="22" cy="22" r="19" fill="currentColor" opacity="0.18"/><circle cx="22" cy="22" r="19" fill="none" stroke="currentColor" stroke-width="2.4"/><circle cx="16" cy="19" r="1.8" fill="currentColor"/><circle cx="28" cy="19" r="1.8" fill="currentColor"/><path d="M15 28 q7 3 14 0" fill="none" stroke="currentColor" stroke-width="2.4" stroke-linecap="round"/></svg>
          #elif $mood == 'mild'
          <svg viewBox="0 0 44 44" aria-hidden="true"><circle cx="22" cy="22" r="19" fill="currentColor" opacity="0.2"/><circle cx="22" cy="22" r="19" fill="none" stroke="currentColor" stroke-width="2.4"/><circle cx="16" cy="19" r="2" fill="currentColor"/><circle cx="28" cy="19" r="2" fill="currentColor"/><path d="M14 27 q8 7 16 0" fill="none" stroke="currentColor" stroke-width="2.6" stroke-linecap="round"/></svg>
          #elif $mood == 'warm'
          <svg viewBox="0 0 44 44" aria-hidden="true"><circle cx="22" cy="22" r="19" fill="currentColor" opacity="0.2"/><circle cx="22" cy="22" r="19" fill="none" stroke="currentColor" stroke-width="2.4"/><circle cx="16" cy="19" r="2" fill="currentColor"/><circle cx="28" cy="19" r="2" fill="currentColor"/><path d="M15 28 q7 5 14 0" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round"/><path d="M34 14 q3 4 0 7 q-3 -3 0 -7Z" fill="currentColor"/></svg>
          #else
          <svg viewBox="0 0 44 44" aria-hidden="true"><circle cx="22" cy="22" r="19" fill="currentColor" opacity="0.22"/><circle cx="22" cy="22" r="19" fill="none" stroke="currentColor" stroke-width="2.4"/><path d="M13 20 q3 -3 6 0" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round"/><path d="M25 20 q3 -3 6 0" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round"/><path d="M14 30 q4 -3 8 0 q4 3 8 0" fill="none" stroke="currentColor" stroke-width="2.4" stroke-linecap="round"/><path d="M35 15 q3 5 0 9 q-3 -4 0 -9Z" fill="currentColor"/><path d="M9 17 q2.5 4 0 7 q-2.5 -3 0 -7Z" fill="currentColor"/></svg>
          #end if
          <span class="mood-cap">$lbl($mood_key)</span>
        </div>
        #end if
        <div class="hero-temp">$current.outTemp.format(add_label=False)<span class="hero-unit">$unit.label.outTemp</span></div>
        #if $feels.has_data
        <div class="hero-feels">$lbl('Feels like') $feels.format()</div>
        #end if
        <div class="hero-range">
          <span class="hero-hi">&#9650; $day.outTemp.max.format()</span>
          <span class="hero-lo">&#9660; $day.outTemp.min.format()</span>
        </div>
      </div>
      #end if

      #if $show_current
      ## Live current values. Each row is gated by Extras.rows.*
      <div class="col_sub gradient boxshadow dist">
        <div class="headline headline-row"><span>$lbl('Latest Values')</span><span class="headline-time">$current.dateTime</span></div>
        ## Extras.rows_show_range adds today's high/low (with time) beside each value.
        #set global $rng = $tf($getVar('Extras.rows_show_range', False))
        #if $rng
        $vrowhl('Temperature', $current.outTemp.format() + $trendarrow($trend.outTemp, 0.5), $day.outTemp.max.format(add_label=False), $day.outTemp.maxtime, $day.outTemp.min.format(add_label=False), $day.outTemp.mintime, $alertcls('outTemp', $current.outTemp))
        #else
        $vrow('Temperature', $current.outTemp.format() + $trendarrow($trend.outTemp, 0.5), $alertcls('outTemp', $current.outTemp))
        #end if
        #if $tf($getVar('Extras.rows.humidity', True))
        #if $rng
        $vrowhl('Humidity', $current.outHumidity.format() + $trendarrow($trend.outHumidity, 2), $day.outHumidity.max.format(add_label=False), $day.outHumidity.maxtime, $day.outHumidity.min.format(add_label=False), $day.outHumidity.mintime, $alertcls('outHumidity', $current.outHumidity))
        #else
        $vrow('Humidity', $current.outHumidity.format() + $trendarrow($trend.outHumidity, 2), $alertcls('outHumidity', $current.outHumidity))
        #end if
        #end if
        #if $tf($getVar('Extras.rows.dewpoint', True))
        #if $rng
        $vrowhl('Dew Point', $current.dewpoint.format(), $day.dewpoint.max.format(add_label=False), $day.dewpoint.maxtime, $day.dewpoint.min.format(add_label=False), $day.dewpoint.mintime, $alertcls('dewpoint', $current.dewpoint))
        #else
        $vrow('Dew Point', $current.dewpoint.format(), $alertcls('dewpoint', $current.dewpoint))
        #end if
        #end if
        #if $tf($getVar('Extras.rows.wind', True))
        <div class="line$alertcls('windSpeed', $current.windSpeed)">
          <div class="lleft">$lbl('Wind')</div>
          <div class="lright">$current.windSpeed.format()#slurp
#if $current.windDir.has_data
 <span class="wdir">$current.windDir.ordinal_compass</span><svg class="wind-arrow" viewBox="0 0 24 24" style="transform:rotate(#echo int(($current.windDir.raw + 180) % 360)#deg)" aria-hidden="true"><path d="M12 2 L18 11 L13 11 L13 22 L11 22 L11 11 L6 11 Z" /></svg>#slurp
#end if
</div>
        </div>
        #end if
        #if $tf($getVar('Extras.rows.barometer', True))
        #if $trend.barometer.has_data
        #set $bt = $trend.barometer.raw
        #if $bt > 1.0
        #set $btkey, $btarrow, $btcls = 'Rising', '&#9650;', 'up'
        #elif $bt < -1.0
        #set $btkey, $btarrow, $btcls = 'Falling', '&#9660;', 'down'
        #else
        #set $btkey, $btarrow, $btcls = 'Steady', '&#9644;', 'steady'
        #end if
        #set $btrend = '<div class="baro-trend ' + $btcls + '"><span class="bt-arrow">' + $btarrow + '</span> ' + $lbl($btkey) + ' <span class="bt-val">' + $trend.barometer.format(add_label=False) + $unit.label.barometer + ' / 3h</span></div>'
        #else
        #set $btrend = ''
        #end if
        ## Barometer shows value + 3h trend only; its hi/low lives in the Hi/Lows block.
        <div class="line$alertcls('barometer', $current.barometer)"><div class="lleft">$lbl('Barometer')</div><div class="lright"><div class="vh-line">$current.barometer.format()</div>$btrend</div></div>
        #end if
        #if $tf($getVar('Extras.rows.rain_today', True))
        $vrow("Today's Rain", $day.rain.sum.format())
        #end if
        #if $tf($getVar('Extras.rows.rain_rate', True))
        $vrow('Rain Rate', $current.rainRate.format())
        #end if
        #if $tf($getVar('Extras.rows.rain_month', True))
        $vrow('Monthly Rain', $month.rain.sum.format())
        #end if
        #if $tf($getVar('Extras.rows.rain_year', True))
        $vrow('Yearly Rain', $year.rain.sum.format("%.0f"))
        #end if
        #if $tf($getVar('Extras.rows.last_rain', True)) and $getVar('last_rain', None)
        <div class="line"><div class="lleft">$lbl('Last Rain')</div><div class="lright"><div class="vh-line">$last_rain.time</div><div class="baro-trend"><span class="bt-val">#if $last_rain.days == 0#$lbl('Today')#else#$last_rain.days $lbl('days ago')#end if#</span></div></div></div>
        #end if
        #if $tf($getVar('Extras.rows.et', True)) and $day.ET.has_data
        $vrow("Today's ET", $day.ET.sum.format())
        #end if
        #if $tf($getVar('Extras.rows.windchill', True)) and $current.windchill.has_data
        #if $rng
        $vrowhl('Wind Chill', $current.windchill.format(), $day.windchill.max.format(add_label=False), $day.windchill.maxtime, $day.windchill.min.format(add_label=False), $day.windchill.mintime)
        #else
        $vrow('Wind Chill', $current.windchill.format())
        #end if
        #end if
        #if $tf($getVar('Extras.rows.heatindex', True)) and $current.heatindex.has_data
        #if $rng
        $vrowhl('Heat Index', $current.heatindex.format(), $day.heatindex.max.format(add_label=False), $day.heatindex.maxtime, $day.heatindex.min.format(add_label=False), $day.heatindex.mintime)
        #else
        $vrow('Heat Index', $current.heatindex.format())
        #end if
        #end if
        #if $tf($getVar('Extras.rows.apptemp', True)) and $current.appTemp.has_data
        #if $rng
        $vrowhl('Apparent Temperature', $current.appTemp.format(), $day.appTemp.max.format(add_label=False), $day.appTemp.maxtime, $day.appTemp.min.format(add_label=False), $day.appTemp.mintime)
        #else
        $vrow('Apparent Temperature', $current.appTemp.format())
        #end if
        #end if
        #if $tf($getVar('Extras.rows.uv', True)) and $current.UV.has_data
        #if $rng
        $vrowhl('UV Index', $current.UV.format() + $trendarrow($trend.UV, 0.5), $day.UV.max.format(add_label=False), $day.UV.maxtime, $day.UV.min.format(add_label=False), $day.UV.mintime)
        #else
        $vrow('UV Index', $current.UV.format() + $trendarrow($trend.UV, 0.5))
        #end if
        #end if
        #if $tf($getVar('Extras.rows.radiation', True)) and $current.radiation.has_data
        #if $rng
        $vrowhl('Solar Radiation', $current.radiation.format(), $day.radiation.max.format(add_label=False), $day.radiation.maxtime, $day.radiation.min.format(add_label=False), $day.radiation.mintime)
        #else
        $vrow('Solar Radiation', $current.radiation.format())
        #end if
        #end if
      </div>
      #end if

      ## Sun & Moon card (Current page; toggle with Extras.celestial).
      #if $show_current and $tf($getVar('Extras.celestial', True))
      <div class="col_sub gradient boxshadow dist">
        <div class="headline">$lbl('Sun & Moon')</div>
        <div class="celestial">
          <div class="moon-wrap">
            <div class="moon" data-fullness="$almanac.moon_fullness"
                 #if $almanac.moon.rise.raw and $almanac.moon.set.raw and $almanac.moon.set.raw < $almanac.moon.rise.raw#data-waning="1"#end if#></div>
            <div class="moon-pct">$almanac.moon_phase, $almanac.moon_fullness%</div>
          </div>
          <div class="celestial-info">
            <div class="cel-row"><span class="cel-lbl">$lbl('Sunrise')</span><b>$almanac.sun.rise</b></div>
            <div class="cel-row"><span class="cel-lbl">$lbl('Sunset')</span><b>$almanac.sun.set</b></div>
            <div class="cel-row"><span class="cel-lbl">$lbl('Moonrise')</span><b>$almanac.moon.rise</b></div>
            <div class="cel-row"><span class="cel-lbl">$lbl('Moonset')</span><b>$almanac.moon.set</b></div>
            <div class="cel-row"><span class="cel-lbl">$lbl('Next full')</span><b>$almanac.next_full_moon</b></div>
            <div class="cel-row"><span class="cel-lbl">$lbl('Next new')</span><b>$almanac.next_new_moon</b></div>
          </div>
        </div>
      </div>
      #end if

      ## Hi/lows for this period ($P)
      <div class="col_sub gradient boxshadow dist hilows">
        <div class="headline">$lbl($period_title_key)</div>
        $hlrow('Temperature', $P.outTemp)
        $hlrow('Humidity', $P.outHumidity)
        $hlrow('Dew Point', $P.dewpoint)
        #if $P.windchill.has_data
        $hlrow('Wind Chill', $P.windchill)
        #end if
        #if $P.heatindex.has_data
        $hlrow('Heat Index', $P.heatindex)
        #end if
        #if $P.appTemp.has_data
        $hlrow('Apparent Temperature', $P.appTemp)
        #end if
        <div class="line twocol"><div class="lleft">$lbl('Barometer')</div><div class="lright"><span>&#9650; $P.barometer.max.format()<span class="hl-at">at</span><span class="hl-time">$P.barometer.maxtime</span></span><span>&#9660; $P.barometer.min.format()<span class="hl-at">at</span><span class="hl-time">$P.barometer.mintime</span></span></div></div>
        <div class="line twocol"><div class="lleft">$lbl('High Wind Gust')</div><div class="lright"><span>&#9650; $P.wind.max.format()<span class="hl-at">at</span><span class="hl-time">$P.wind.maxtime</span></span></div></div>
        <div class="line twocol"><div class="lleft">$lbl('High Rain Rate')</div><div class="lright"><span>&#9650; $P.rainRate.max.format()<span class="hl-at">at</span><span class="hl-time">$P.rainRate.maxtime</span></span></div></div>
        <div class="line"><div class="lleft">$lbl('Total Rain')</div><div class="lright">$P.rain.sum.format()</div></div>
        #if $P.UV.has_data
        $hlrow('UV Index', $P.UV)
        #end if
        #if $P.radiation.has_data
        $hlrow('Solar Rad.', $P.radiation)
        #end if
        #if $P.ET.has_data
        <div class="line"><div class="lleft">$lbl('Evapotranspiration')</div><div class="lright">$P.ET.sum.format()</div></div>
        #end if
      </div>

      ## All-time records from the whole archive (Current page; toggle with
      ## Extras.records). Sits below the period hi/lows. Each row: label, record
      ## value, and the date it occurred.
      #if $show_current and $tf($getVar('Extras.records', True))
      <div class="col_sub gradient boxshadow dist records">
        <div class="headline">$lbl('All-Time Records')</div>
        #if $alltime.outTemp.has_data
        <div class="rec-row"><span class="rec-lbl">$lbl('Highest Temperature')</span><b>$alltime.outTemp.max.format()<span class="rec-date">$alltime.outTemp.maxtime.format("%d %b %Y")</span></b></div>
        <div class="rec-row"><span class="rec-lbl">$lbl('Lowest Temperature')</span><b>$alltime.outTemp.min.format()<span class="rec-date">$alltime.outTemp.mintime.format("%d %b %Y")</span></b></div>
        #end if
        #if $alltime.windGust.has_data
        <div class="rec-row"><span class="rec-lbl">$lbl('Highest Wind Gust')</span><b>$alltime.windGust.max.format()<span class="rec-date">$alltime.windGust.maxtime.format("%d %b %Y")</span></b></div>
        #end if
        #if $alltime.outHumidity.has_data
        <div class="rec-row"><span class="rec-lbl">$lbl('Highest Humidity')</span><b>$alltime.outHumidity.max.format()<span class="rec-date">$alltime.outHumidity.maxtime.format("%d %b %Y")</span></b></div>
        #end if
        #if $alltime.barometer.has_data
        <div class="rec-row"><span class="rec-lbl">$lbl('Highest Barometer')</span><b>$alltime.barometer.max.format()<span class="rec-date">$alltime.barometer.maxtime.format("%d %b %Y")</span></b></div>
        <div class="rec-row"><span class="rec-lbl">$lbl('Lowest Barometer')</span><b>$alltime.barometer.min.format()<span class="rec-date">$alltime.barometer.mintime.format("%d %b %Y")</span></b></div>
        #end if
        #if $alltime.rainRate.has_data
        <div class="rec-row"><span class="rec-lbl">$lbl('Highest Rain Rate')</span><b>$alltime.rainRate.max.format()<span class="rec-date">$alltime.rainRate.maxtime.format("%d %b %Y")</span></b></div>
        #end if
      </div>
      #end if

      ## Auto-discovered extra sensors (Current page; toggle with Extras.extra_sensors).
      #if $show_current and $tf($getVar('Extras.extra_sensors', True)) and $getVar('has_extra_sensors', False)
      <div class="col_sub gradient boxshadow dist extra-sensors">
        <div class="headline">$gettext("Extra Sensors")</div>
        #for $grp in $extra_groups
        <div class="extra-group">$gettext($grp.title_key)</div>
        #for $o in $grp.obs
        <div class="extra-row">
          <span class="extra-label">$obs.label[$o.key]</span>
          <span class="extra-value">$o.value.format()</span>
        </div>
        #end for
        #end for
      </div>
      #end if

      ## Useful external links (toggle/edit via [Extras][[links]] in weewx.conf).
      #set $links = $getVar('Extras.links', {})
      #if $links and $tf($links.get('show', True))
      <div class="col_sub gradient boxshadow dist links-card">
        <div class="headline">$lbl('Useful Links')</div>
        #for $k in $links
        #if $k not in ('show', 'title') and hasattr($links[$k], 'get') and $links[$k].get('url')
        <a class="link-row" href="$links[$k].get('url')" target="_blank" rel="noopener">$links[$k].get('text', $k)</a>
        #end if
        #end for
      </div>
      #end if

    </div>

    ## RIGHT COLUMN: charts for $period_key
    <div class="right_col">
      <div class="col_sub gradient boxshadow dist">
        <div class="headline">$lbl('Diagrams')</div>
        <div class="diagrams_holder">
          ## One chart per metric, gated by Extras.charts.* and sensor presence.
          #if $tf($getVar('Extras.charts.temp', True))
          <div class="m-chart" id="chart-temp" role="img" aria-label="$gettext('Temperature') $gettext('chart')"></div>
          #end if
          #if $tf($getVar('Extras.charts.humidity', True))
          <div class="m-chart" id="chart-humidity" role="img" aria-label="$gettext('Humidity') $gettext('chart')"></div>
          #end if
          #if $tf($getVar('Extras.charts.pressure', True))
          <div class="m-chart" id="chart-pressure" role="img" aria-label="$gettext('Barometer') $gettext('chart')"></div>
          #end if
          #if $tf($getVar('Extras.charts.windspeed', True))
          <div class="m-chart" id="chart-windspeed" role="img" aria-label="$gettext('Wind') $gettext('chart')"></div>
          #end if
          #if $tf($getVar('Extras.charts.windvec', True))
          <div class="m-chart" id="chart-windvec" role="img" aria-label="$gettext('Wind Direction') $gettext('chart')"></div>
          #end if
          #if $tf($getVar('Extras.charts.windvector', True))
          <div class="m-chart" id="chart-windvector" role="img" aria-label="$gettext('Wind Vector') $gettext('chart')"></div>
          #end if
          #if $tf($getVar('Extras.charts.windrose', True))
          <div class="m-chart" id="chart-windrose" role="img" aria-label="$gettext('Wind Rose') $gettext('chart')"></div>
          #end if
          #if $tf($getVar('Extras.charts.rain', True))
          <div class="m-chart" id="chart-rain" role="img" aria-label="$gettext('Rain') $gettext('chart')"></div>
          #end if
          #if $tf($getVar('Extras.charts.rainrate', True))
          <div class="m-chart" id="chart-rainrate" role="img" aria-label="$gettext('Rain Rate') $gettext('chart')"></div>
          #end if
          #if $tf($getVar('Extras.charts.uv', True)) and $P.UV.has_data
          <div class="m-chart" id="chart-uv" role="img" aria-label="$gettext('UV Index') $gettext('chart')"></div>
          #end if
          #if $tf($getVar('Extras.charts.radiation', True)) and $P.radiation.has_data
          <div class="m-chart" id="chart-radiation" role="img" aria-label="$gettext('Solar Radiation') $gettext('chart')"></div>
          #end if
          #if $tf($getVar('Extras.charts.et', True)) and $P.ET.has_data
          <div class="m-chart" id="chart-et" role="img" aria-label="$gettext('Evapotranspiration') $gettext('chart')"></div>
          #end if
          #if $tf($getVar('Extras.charts.cloudbase', True)) and $P.cloudbase.has_data
          <div class="m-chart" id="chart-cloudbase" role="img" aria-label="$gettext('Cloud Base') $gettext('chart')"></div>
          #end if
        </div>
      </div>
    </div>

    <div class="clear"></div>
  </div>

  <script>
    window.AGANETWX_PERIOD = "$period_key";
    window.AGANETWX_TZ = "$_tzname";
    // Unit labels resolved from the configured unit system, so charts follow
    // us/metric/metricwx (e.g. degF/inHg/mph vs degC/hPa/km/h). Trimmed.
    window.AGANETWX_UNITS = {
      "temp": "#echo $unit.label.outTemp.strip()#", "pressure": "#echo $unit.label.barometer.strip()#",
      "wind": "#echo $unit.label.windSpeed.strip()#", "rain": "#echo $unit.label.rain.strip()#",
      "rainRate": "#echo $unit.label.rainRate.strip()#", "radiation": "#echo $unit.label.radiation.strip()#",
      "cloudbase": "#echo $unit.label.cloudbase.strip()#", "ET": "#echo $unit.label.ET.strip()#"
    };
    // Chart labels, translated server-side so wording lives only in lang/*.conf.
    window.AGANETWX_I18N = {
      "temp": "$gettext('Temperature')", "dewpoint": "$gettext('Dew Point')",
      "appTemp": "$gettext('Apparent')", "heatindex": "$gettext('Heat Index')",
      "windchill": "$gettext('Wind Chill')", "humidity": "$gettext('Humidity')",
      "pressure": "$gettext('Barometer')", "windSpeed": "$gettext('Wind Speed')",
      "windGust": "$gettext('Gust')", "windvec": "$gettext('Wind Direction')",
      "windvector": "$gettext('Wind Vector')", "windrose": "$gettext('Wind Rose')",
      "calm": "$gettext('calm')", "rain": "$gettext('Rain')", "rainRate": "$gettext('Rain Rate')", "UV": "$gettext('UV Index')",
      "radiation": "$gettext('Solar Radiation')", "ET": "$gettext('Evapotranspiration')",
      "cloudbase": "$gettext('Cloud Base')"
    };
  </script>

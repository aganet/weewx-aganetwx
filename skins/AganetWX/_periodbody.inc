## Shared period-page body. Caller sets $P, $period_key, $show_current, $period_title_key.
#import time
#import os
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
      <div class="col_sub gradient boxshadow dist hero">
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
        $vrowhl('Temperature', $current.outTemp.format(), $day.outTemp.max.format(add_label=False), $day.outTemp.maxtime, $day.outTemp.min.format(add_label=False), $day.outTemp.mintime)
        #else
        $vrow('Temperature', $current.outTemp.format())
        #end if
        #if $tf($getVar('Extras.rows.humidity', True))
        #if $rng
        $vrowhl('Humidity', $current.outHumidity.format(), $day.outHumidity.max.format(add_label=False), $day.outHumidity.maxtime, $day.outHumidity.min.format(add_label=False), $day.outHumidity.mintime)
        #else
        $vrow('Humidity', $current.outHumidity.format())
        #end if
        #end if
        #if $tf($getVar('Extras.rows.dewpoint', True))
        #if $rng
        $vrowhl('Dew Point', $current.dewpoint.format(), $day.dewpoint.max.format(add_label=False), $day.dewpoint.maxtime, $day.dewpoint.min.format(add_label=False), $day.dewpoint.mintime)
        #else
        $vrow('Dew Point', $current.dewpoint.format())
        #end if
        #end if
        #if $tf($getVar('Extras.rows.wind', True))
        <div class="line">
          <div class="lleft">$lbl('Wind')</div>
          <div class="lright">$current.windSpeed.format() <span class="wdir">at $current.windDir.ordinal_compass</span></div>
          <div class="clear"></div>
        </div>
        #end if
        #if $tf($getVar('Extras.rows.barometer', True))
        #if $rng
        $vrowhl('Barometer', $current.barometer.format(), $day.barometer.max.format(add_label=False), $day.barometer.maxtime, $day.barometer.min.format(add_label=False), $day.barometer.mintime)
        #else
        $vrow('Barometer', $current.barometer.format())
        #end if
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
        $vrowhl('UV Index', $current.UV.format(), $day.UV.max.format(add_label=False), $day.UV.maxtime, $day.UV.min.format(add_label=False), $day.UV.mintime)
        #else
        $vrow('UV Index', $current.UV.format())
        #end if
        #end if
        #if $tf($getVar('Extras.rows.radiation', True)) and $current.radiation.has_data
        #if $rng
        $vrowhl('Solar Radiation', $current.radiation.format(), $day.radiation.max.format(add_label=False), $day.radiation.maxtime, $day.radiation.min.format(add_label=False), $day.radiation.mintime)
        #else
        $vrow('Solar Radiation', $current.radiation.format())
        #end if
        #end if
        #if $tf($getVar('Extras.rows.sun', True))
        $vrow('Sunrise', $almanac.sunrise)
        $vrow('Sunset', $almanac.sunset)
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

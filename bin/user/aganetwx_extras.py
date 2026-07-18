"""Search List Extension: discovers the station's extra observation types and
groups them for the template as $extra_groups (no hardcoded sensor list), and
loads the [Texts] of each configured language for the in-page switcher ($i18n)."""

import json
import os

import configobj

import weewx.units
from weewx.cheetahgenerator import SearchList
from weewx.tags import ObservationBinder



# Types shown by the main/hi-low panels; skipped here.
CORE = {
    'outTemp', 'inTemp', 'outHumidity', 'inHumidity', 'dewpoint', 'windchill',
    'heatindex', 'appTemp', 'barometer', 'pressure', 'altimeter',
    'windSpeed', 'windGust', 'windDir', 'windGustDir', 'rain', 'rainRate',
    'ET', 'UV', 'radiation', 'cloudbase', 'dateTime', 'usUnits', 'interval',
    'windrun', 'rainfall', 'maxSolarRad',
}

# Map an obs name to a group. First match wins.
def _bucket(key):
    k = key.lower()
    if k.startswith('extratemp') or k.startswith('soiltemp'):
        return 'temp'
    if k.startswith('extrahumid'):
        return 'humidity'
    if k.startswith('soilmoist') or k.startswith('soil'):
        return 'soil'
    if k.startswith('leafwet') or k.startswith('leaftemp') or k.startswith('leaf'):
        return 'leaf'
    if k.startswith('pm') or 'pm2_5' in k or 'pm10' in k or k.startswith('co2') \
            or k.startswith('aqi') or k.startswith('no2') or k.startswith('o3'):
        return 'air'
    if 'lightning' in k:
        return 'lightning'
    if 'battery' in k or k.endswith('_batt') or 'voltage' in k or k.startswith('supplyvolt') \
            or k.startswith('consbattery') or 'signal' in k or 'rssi' in k:
        return 'status'
    return 'other'


# Display order and heading text (a [Texts] key).
GROUP_ORDER = [
    ('temp',      'Extra Temperatures'),
    ('humidity',  'Extra Humidity'),
    ('soil',      'Soil'),
    ('leaf',      'Leaf'),
    ('air',       'Air Quality'),
    ('lightning', 'Lightning'),
    ('other',     'Other Sensors'),
    ('status',    'Sensor Status'),
]


class AganetWXExtras(SearchList):
    def get_extension_list(self, timespan, db_lookup):
        db_manager = db_lookup()

        try:
            sqlkeys = db_manager.sqlkeys
        except Exception:
            sqlkeys = []

        try:
            record = db_manager.getRecord(timespan.stop)
        except Exception:
            record = None

        # Fields with a value now; fall back to schema keys if no record.
        present = {}
        if record:
            for k, v in record.items():
                if v is not None:
                    present[k] = v
        else:
            for k in sqlkeys:
                present[k] = None

        # Bucket the non-core types.
        buckets = {}
        for key in sorted(present.keys()):
            if key in CORE:
                continue
            grp = _bucket(key)
            buckets.setdefault(grp, []).append(key)

        # Optional Extras.extra_sensor_groups: a list of group keys to show, in
        # the given order. Empty/unset shows all groups in the default order.
        extras = self.generator.skin_dict.get('Extras', {})
        wanted = extras.get('extra_sensor_groups', '')
        if isinstance(wanted, str):
            wanted = [g.strip() for g in wanted.replace(',', ' ').split()]
        wanted = [g for g in wanted if g]
        if wanted:
            order = [(g, dict(GROUP_ORDER).get(g, g)) for g in wanted]
        else:
            order = GROUP_ORDER

        # Wrap each kept type's latest value as a formattable ValueHelper.
        groups = []
        for grp_key, title in order:
            keys = buckets.get(grp_key, [])
            if not keys:
                continue
            obs = []
            for key in keys:
                binder = ObservationBinder(
                    key, timespan, db_lookup, None, 'current',
                    self.generator.formatter, self.generator.converter)
                vh = binder.last
                if not vh.has_data:
                    continue
                obs.append({'key': key, 'value': vh})
            if obs:
                groups.append({'group': grp_key, 'title_key': title, 'obs': obs})

        # When did it last rain? Newest archive record with rain > 0.
        last_rain = self._last_rain(db_manager, timespan.stop)

        # Optional custom temperature-chart series.
        series, labels = self._temp_chart(extras)

        return [{'extra_groups': groups, 'has_extra_sensors': len(groups) > 0,
                 'last_rain': last_rain,
                 'temp_chart_series': series, 'temp_chart_labels': labels}]

    # Built-in temperature series the temp chart already knows.
    _TEMP_BUILTIN = ('outTemp', 'dewpoint', 'appTemp', 'heatindex', 'windchill')

    def _temp_chart(self, extras):
        """Resolve Extras.temp_chart_series into an ordered list of observation
        names plus a label map for the extra (non-built-in) ones. Empty list
        means the chart uses its default series."""
        raw = extras.get('temp_chart_series', '')
        if hasattr(raw, 'split'):
            raw = raw.replace(',', ' ').split()
        names = [str(n).strip() for n in raw if str(n).strip()]
        labels = {}
        for name in names:
            if name not in self._TEMP_BUILTIN:
                labels[name] = self._obs_label(name)
        return names, labels

    def _obs_label(self, obs):
        """Display label for an observation, from the skin's label dict."""
        try:
            labels = self.generator.skin_dict.get('Labels', {}).get('Generic', {})
            return labels.get(obs, obs)
        except Exception:
            return obs

    def _last_rain(self, db_manager, now_ts):
        """ValueHelper for the timestamp of the last rain, plus an 'ago' string.
        Returns None if the archive has no rain or the query fails."""
        try:
            row = db_manager.getSql(
                "SELECT MAX(dateTime) FROM %s WHERE rain > 0"
                % db_manager.table_name)
        except Exception:
            return None
        if not row or row[0] is None:
            return None
        ts = int(row[0])
        vt = weewx.units.ValueTuple(ts, 'unix_epoch', 'group_time')
        vh = weewx.units.ValueHelper(vt, 'current',
                                     self.generator.formatter,
                                     self.generator.converter)
        days = int((int(now_ts) - ts) // 86400)
        return {'time': vh, 'days': days}


class AganetWXI18n(SearchList):
    """Loads the [Texts] of every language in Extras.languages and exposes them
    as $i18n_json (a JSON map code -> {key: text}) plus $i18n_langs (ordered
    list of {code, name}). Powers the client-side language switcher: one build
    carries all languages, no per-language duplication of the site."""

    def __init__(self, generator):
        SearchList.__init__(self, generator)
        extras = generator.skin_dict.get('Extras', {})
        codes = extras.get('languages', '')
        if isinstance(codes, str):
            codes = [c.strip() for c in codes.replace(',', ' ').split()]
        skin_dir = self._skin_dir(generator)
        self.dicts, self.names = {}, []
        for code in [c for c in codes if c]:
            texts, name = self._load(skin_dir, code)
            if texts is None:
                continue
            self.dicts[code] = texts
            self.names.append({'code': code, 'name': name or code})

    @staticmethod
    def _skin_dir(generator):
        sd = generator.skin_dict
        root = sd.get('SKIN_ROOT', '')
        name = sd.get('skin', sd.get('SKIN_NAME', ''))
        weewx_root = generator.config_dict.get('WEEWX_ROOT', '')
        if root and not os.path.isabs(root):
            root = os.path.join(weewx_root, root)
        return os.path.join(root, name)

    @staticmethod
    def _load(skin_dir, code):
        path = os.path.join(skin_dir, 'lang', '%s.conf' % code)
        if not os.path.exists(path):
            return None, None
        try:
            cfg = configobj.ConfigObj(path, encoding='utf-8')
        except Exception:
            return None, None
        texts = dict(cfg.get('Texts', {}))
        # Fold in observation names so chart/label keys resolve too.
        for k, v in dict(cfg.get('Labels', {}).get('Generic', {})).items():
            texts.setdefault(k, v)
        return texts, texts.get('Language')

    def get_extension_list(self, timespan, db_lookup):
        return [{
            'i18n_json': json.dumps(self.dicts, ensure_ascii=True),
            'i18n_langs': self.names,
            'i18n_enabled': len(self.names) > 1,
        }]


class AganetWXSolar(SearchList):
    """Fetches HamQSL (N0NBH) solar-terrestrial data and exposes it to the
    template as $solar. Self-contained: only the server makes the request, so
    the generated page has no client-side external calls.

    The reading is cached on disk with its fetch time. A network fetch happens
    only when the cache is missing or older than solar_refresh (default 3600s);
    every other report cycle reads the cache and adds no delay. HamQSL itself
    updates roughly hourly, so an hour is a sensible default. A failed fetch
    falls back to the cached reading; the card stays hidden only until the first
    successful fetch."""

    URL = 'https://www.hamqsl.com/solarxml.php'

    def get_extension_list(self, timespan, db_lookup):
        extras = self.generator.skin_dict.get('Extras', {})
        enabled = str(extras.get('solar', 'false')).strip().lower() \
            not in ('false', '0', 'no', 'off', '')
        if not enabled:
            return [{'solar': None}]
        try:
            timeout = int(extras.get('solar_timeout', 15))
        except (TypeError, ValueError):
            timeout = 15
        try:
            refresh = int(extras.get('solar_refresh', 3600))
        except (TypeError, ValueError):
            refresh = 3600

        import time
        cache_path = self._cache_path()
        cached, age = self._read_cache(cache_path)
        # Serve from cache unless it is missing or stale.
        if cached is not None and age is not None and age < refresh:
            return [{'solar': cached}]

        data = self._fetch(timeout)
        if data is not None:
            self._write_cache(cache_path, data, time.time())
            return [{'solar': data}]
        # Fetch failed: reuse the last good reading if we have one.
        return [{'solar': cached}]

    def _cache_path(self):
        import tempfile
        return os.path.join(tempfile.gettempdir(), 'aganetwx_solar.json')

    def _read_cache(self, path):
        import time
        try:
            with open(path, 'r', encoding='utf-8') as f:
                obj = json.load(f)
            return obj.get('data'), time.time() - float(obj.get('fetched', 0))
        except Exception:
            return None, None

    def _write_cache(self, path, data, fetched):
        try:
            tmp = path + '.tmp'
            with open(tmp, 'w', encoding='utf-8') as f:
                json.dump({'fetched': fetched, 'data': data}, f)
            os.replace(tmp, path)
        except Exception:
            pass

    def _fetch(self, timeout):
        import urllib.request
        import xml.etree.ElementTree as ET
        try:
            with urllib.request.urlopen(self.URL, timeout=timeout) as resp:
                raw = resp.read()
            root = ET.fromstring(raw)
            sd = root.find('solardata')
            if sd is None:
                return None

            def txt(tag):
                el = sd.find(tag)
                return el.text.strip() if el is not None and el.text else ''

            bands = {}
            cc = sd.find('calculatedconditions')
            if cc is not None:
                for band in cc.findall('band'):
                    name = band.get('name')
                    time = band.get('time')
                    if name and time and band.text:
                        bands.setdefault(name, {})[time] = band.text.strip()

            return {
                'updated': txt('updated'),
                'sfi': txt('solarflux'),
                'sunspots': txt('sunspots'),
                'aindex': txt('aindex'),
                'kindex': txt('kindex'),
                'xray': txt('xray'),
                'geomag': txt('geomagfield'),
                # Ordered list of (band-label, day-condition, night-condition).
                'bands': [(n, bands[n].get('day', ''), bands[n].get('night', ''))
                          for n in ('80m-40m', '30m-20m', '17m-15m', '12m-10m')
                          if n in bands],
            }
        except Exception:
            return None


class AganetWXCompare(SearchList):
    """Compares today with the recent past using the station's own archive,
    exposed as $compare on the Current page. All comparisons use only local
    data. None of the fields are shown unless there is enough history for them.

    Fields (each may be None):
      temp_vs_yesterday : {'delta': ValueHelper, 'warmer': bool}
          Today's highest temperature so far vs yesterday up to the SAME time of
          day (a fair like-for-like comparison, not today-so-far vs a full day).
      month_rain        : {'pct': int, 'more': bool, 'month_name_key': str,
                           'years': int}
          This month's rain total vs the average of the same month in prior
          years (needs at least two prior years). Percentage difference.
      lastyear_hi       : ValueHelper
          The highest temperature on this calendar date one year ago, as a plain
          factual data point (a single day is weather, not climate, so it is not
          phrased as a trend).

    Off unless Extras.compare is set."""

    MONTHS = ['January', 'February', 'March', 'April', 'May', 'June', 'July',
              'August', 'September', 'October', 'November', 'December']

    def get_extension_list(self, timespan, db_lookup):
        extras = self.generator.skin_dict.get('Extras', {})
        enabled = str(extras.get('compare', 'false')).strip().lower() \
            not in ('false', '0', 'no', 'off', '')
        if not enabled:
            return [{'compare': None}]
        try:
            result = self._compare(db_lookup(), timespan.stop)
        except Exception:
            result = None
        return [{'compare': result}]

    def _temp_vh(self, value):
        if value is None:
            return None
        vt = weewx.units.ValueTuple(float(value), 'degree_C', 'group_temperature')
        return weewx.units.ValueHelper(vt, 'current', self.generator.formatter,
                                       self.generator.converter)

    def _compare(self, db_manager, now_ts):
        import time
        table = db_manager.table_name
        now_ts = int(now_ts)
        lt = time.localtime(now_ts)
        secs_into_day = lt.tm_hour * 3600 + lt.tm_min * 60 + lt.tm_sec
        day_start = now_ts - secs_into_day

        def temp_c(value, usUnits):
            """outTemp to degC (US stores degF)."""
            if value is None:
                return None
            return (value - 32.0) * 5.0 / 9.0 if usUnits == 1 else value

        # unit system for conversions
        try:
            sysid = db_manager.std_unit_system
        except Exception:
            sysid = 16
        us = (sysid == 1)

        out = {}

        # --- Today's max so far vs yesterday up to the same time of day ---
        row_t = db_manager.getSql(
            "SELECT MAX(outTemp) FROM %s WHERE dateTime > ?" % table, (day_start,))
        row_y = db_manager.getSql(
            "SELECT MAX(outTemp) FROM %s WHERE dateTime > ? AND dateTime <= ?" % table,
            (day_start - 86400, day_start - 86400 + secs_into_day))
        if row_t and row_t[0] is not None and row_y and row_y[0] is not None:
            t_c = temp_c(row_t[0], us)
            y_c = temp_c(row_y[0], us)
            delta_c = t_c - y_c
            if abs(delta_c) >= 0.1:
                out['temp_vs_yesterday'] = {
                    'delta': self._temp_vh(abs(delta_c)),
                    'warmer': delta_c > 0,
                }

        # --- This month so far vs the same month, up to the same day AND time
        # of day, in prior years. Truly like-for-like: Jul 1..18 09:00 this year
        # against Jul 1..18 09:00 of each past year, so today's partial day is
        # matched against prior years' partial days (accurate even on day 1). ---
        mm = time.strftime('%m', lt)
        cur_year = time.strftime('%Y', lt)
        # Cutoff as month-day-time within the year; compare as a string.
        cutoff = time.strftime('%m-%d %H:%M', lt)
        totals = {}
        for yr, tot in db_manager.genSql(
                "SELECT strftime('%%Y', dateTime, 'unixepoch', 'localtime') yr, "
                "SUM(rain) FROM %s WHERE strftime('%%m', dateTime, 'unixepoch', "
                "'localtime') = ? AND strftime('%%m-%%d %%H:%%M', dateTime, "
                "'unixepoch', 'localtime') <= ? GROUP BY yr" % table, (mm, cutoff)):
            if tot is not None:
                totals[yr] = tot
        this_month = totals.get(cur_year)
        prior = [v for y, v in totals.items() if y != cur_year]
        if this_month is not None and len(prior) >= 2:
            avg = sum(prior) / len(prior)
            if avg > 0:
                pct = int(round((this_month - avg) / avg * 100.0))
                if abs(pct) >= 1:
                    out['month_rain'] = {
                        'pct': abs(pct), 'more': pct > 0,
                        'month_name_key': self.MONTHS[int(mm) - 1],
                        'years': len(prior),
                    }

        # --- This month's average temperature so far vs the same month up to
        # the same day in prior years (like-for-like, same as the rain). ---
        avgs = {}
        for yr, avgt in db_manager.genSql(
                "SELECT strftime('%%Y', dateTime, 'unixepoch', 'localtime') yr, "
                "AVG(outTemp) FROM %s WHERE strftime('%%m', dateTime, 'unixepoch', "
                "'localtime') = ? AND strftime('%%m-%%d %%H:%%M', dateTime, "
                "'unixepoch', 'localtime') <= ? GROUP BY yr" % table, (mm, cutoff)):
            if avgt is not None:
                avgs[yr] = temp_c(avgt, us)
        this_avg = avgs.get(cur_year)
        prior_avg = [v for y, v in avgs.items() if y != cur_year]
        if this_avg is not None and len(prior_avg) >= 2:
            mean = sum(prior_avg) / len(prior_avg)
            delta_c = this_avg - mean
            if abs(delta_c) >= 0.1:
                out['month_temp'] = {
                    'delta': self._temp_vh(abs(delta_c)),
                    'warmer': delta_c > 0,
                    'month_name_key': self.MONTHS[int(mm) - 1],
                    'years': len(prior_avg),
                }

        # --- Today's high so far vs the same calendar date last year, up to the
        # same time of day (like-for-like, not today-so-far vs a full past day).
        # A single day year-to-year is weather not climate, but it is a fun
        # data point; phrased as a plain warmer/cooler difference. ---
        ly_date = '%04d-%s' % (int(cur_year) - 1, time.strftime('%m-%d', lt))
        ly_start = None
        row_ls = db_manager.getSql(
            "SELECT MIN(dateTime) FROM %s WHERE "
            "strftime('%%Y-%%m-%%d', dateTime, 'unixepoch', 'localtime') = ?" % table,
            (ly_date,))
        if row_ls and row_ls[0] is not None:
            ly_start = int(row_ls[0]) - 1   # just before that day's first record
        row_ly = None
        if ly_start is not None:
            row_ly = db_manager.getSql(
                "SELECT MAX(outTemp) FROM %s WHERE dateTime > ? AND dateTime <= ?" % table,
                (ly_start, ly_start + secs_into_day))
        if (row_ly and row_ly[0] is not None
                and row_t and row_t[0] is not None):
            ly_c = temp_c(row_ly[0], us)
            today_c = temp_c(row_t[0], us)
            delta_c = today_c - ly_c
            if abs(delta_c) >= 0.1:
                out['vs_lastyear'] = {
                    'delta': self._temp_vh(abs(delta_c)),
                    'warmer': delta_c > 0,
                }

        return out or None


class AganetWXCompareData(SearchList):
    """Builds the dataset for the interactive Compare page: for each metric,
    the value on every day-of-month, grouped by calendar month and year, across
    the whole archive. Lets a visitor overlay, say, July's daily mean
    temperature for every year. Exposed as $compare_data (a JSON string) and
    $compare_years. Only built when Extras.compare_page is on, and only for the
    Compare page (period_key == 'compare'), so it costs nothing elsewhere.

    Shape: {metric: {"07": {"2023": [d1..d31 or null], ...}, ...}, ...}
    Values are in the report's display units (converted from the DB)."""

    # metric key -> (db column, aggregate, unit group)
    METRICS = [
        ('temp',      'outTemp',     'avg', 'group_temperature'),
        ('rain',      'rain',        'sum', 'group_rain'),
        ('wind',      'windSpeed',   'avg', 'group_speed'),
        ('humidity',  'outHumidity', 'avg', 'group_percent'),
        ('pressure',  'barometer',   'avg', 'group_pressure'),
        ('uv',        'UV',          'max', 'group_uv'),
        ('radiation', 'radiation',   'avg', 'group_radiation'),
    ]

    # Held in-process for the report run (the SLE runs for every template).
    _cached = None

    def get_extension_list(self, timespan, db_lookup):
        extras = self.generator.skin_dict.get('Extras', {})
        enabled = str(extras.get('compare_page', 'false')).strip().lower() \
            not in ('false', '0', 'no', 'off', '')
        if not enabled:
            return [{'compare_data': None, 'compare_years': []}]
        if AganetWXCompareData._cached is None:
            AganetWXCompareData._cached = self._load_or_build(extras, db_lookup)
        return [AganetWXCompareData._cached]

    def _load_or_build(self, extras, db_lookup):
        """Aggregating the whole archive is expensive, and the history barely
        changes within a day, so the result is cached on disk and rebuilt only
        when older than compare_refresh seconds (default 86400 = once a day)."""
        try:
            ttl = int(extras.get('compare_refresh', 86400))
        except (TypeError, ValueError):
            ttl = 86400
        path = self._cache_path()
        cached = self._read_cache(path, ttl)
        if cached is not None:
            return cached
        try:
            data, years = self._build(db_lookup())
            result = {
                'compare_data': json.dumps(data, ensure_ascii=True) if data else None,
                'compare_years': years,
            }
        except Exception:
            result = {'compare_data': None, 'compare_years': []}
        self._write_cache(path, result)
        return result

    def _cache_path(self):
        import tempfile
        return os.path.join(tempfile.gettempdir(), 'aganetwx_compare.json')

    def _read_cache(self, path, ttl):
        import time
        try:
            if time.time() - os.path.getmtime(path) >= ttl:
                return None
            with open(path, 'r', encoding='utf-8') as f:
                return json.load(f)
        except Exception:
            return None

    def _write_cache(self, path, result):
        try:
            tmp = path + '.tmp'
            with open(tmp, 'w', encoding='utf-8') as f:
                json.dump(result, f)
            os.replace(tmp, path)
        except Exception:
            pass

    def _convert(self, value, group, sysid):
        """Convert a native-unit DB value to the report's display units."""
        if value is None:
            return None
        # Native unit for this group in the database's unit system.
        unit = weewx.units.std_groups.get(sysid, {}).get(group)
        vt = weewx.units.ValueTuple(float(value), unit, group)
        try:
            return round(self.generator.converter.convert(vt)[0], 2)
        except Exception:
            return round(float(value), 2)

    def _build(self, db_manager):
        table = db_manager.table_name
        try:
            sysid = db_manager.std_unit_system
        except Exception:
            sysid = 16

        years = set()
        data = {}
        for key, col, agg, group in self.METRICS:
            month_map = {}
            sql = ("SELECT strftime('%%m', dateTime, 'unixepoch', 'localtime') mo, "
                   "strftime('%%Y', dateTime, 'unixepoch', 'localtime') yr, "
                   "CAST(strftime('%%d', dateTime, 'unixepoch', 'localtime') AS INTEGER) dy, "
                   "%s(%s) FROM %s WHERE %s IS NOT NULL GROUP BY mo, yr, dy"
                   % (agg.upper(), col, table, col))
            for mo, yr, dy, val in db_manager.genSql(sql):
                if val is None or dy is None:
                    continue
                years.add(yr)
                arr = month_map.setdefault(mo, {}).setdefault(yr, [None] * 31)
                if 1 <= dy <= 31:
                    arr[dy - 1] = self._convert(val, group, sysid)
            data[key] = month_map
        return data, sorted(years)

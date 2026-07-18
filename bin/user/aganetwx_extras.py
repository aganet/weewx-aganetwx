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

        # Wrap each kept type's latest value as a formattable ValueHelper.
        groups = []
        for grp_key, title in GROUP_ORDER:
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

        return [{'extra_groups': groups, 'has_extra_sensors': len(groups) > 0,
                 'last_rain': last_rain}]

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


class AganetWXForecast(SearchList):
    """Local Zambretti forecast from the station's own barometer: sea-level
    pressure, its trend, wind direction and season map to one of 26 short
    forecast phrases (the classic analogue-barometer method). No external data.

    This is a faithful port of the pywws implementation (jim-easterbrook/pywws,
    forecast.py); the formulas, wind/season adjustments and lookup tables match
    it exactly. Exposed as $zambretti = {'code', 'text_key'}; None when disabled
    or pressure/trend are unavailable. Off by default (Extras.forecast)."""

    PHRASES = {
        'A': 'Settled fine', 'B': 'Fine weather', 'C': 'Becoming fine',
        'D': 'Fine, becoming less settled', 'E': 'Fine, possible showers',
        'F': 'Fairly fine, improving', 'G': 'Fairly fine, possible showers early',
        'H': 'Fairly fine, showery later', 'I': 'Showery early, improving',
        'J': 'Changeable, mending', 'K': 'Fairly fine, showers likely',
        'L': 'Rather unsettled clearing later', 'M': 'Unsettled, probably improving',
        'N': 'Showery, bright intervals', 'O': 'Showery, becoming less settled',
        'P': 'Changeable, some rain', 'Q': 'Unsettled, short fine intervals',
        'R': 'Unsettled, rain later', 'S': 'Unsettled, some rain',
        'T': 'Mostly very unsettled', 'U': 'Occasional rain, worsening',
        'V': 'Rain at times, very unsettled', 'W': 'Rain at frequent intervals',
        'X': 'Rain, very unsettled', 'Y': 'Stormy, may improve', 'Z': 'Stormy, much rain',
    }
    # Weather-type icon per forecast code (rendered as an SVG in the template).
    ICONS = {
        'A': 'sun', 'B': 'sun', 'C': 'sun',
        'D': 'partly', 'E': 'partly', 'F': 'partly', 'G': 'partly',
        'H': 'showers', 'I': 'showers', 'J': 'partly', 'K': 'partly',
        'L': 'partly', 'M': 'partly', 'N': 'showers', 'O': 'showers',
        'P': 'rain', 'Q': 'partly', 'R': 'rain', 'S': 'rain',
        'T': 'rain', 'U': 'rain', 'V': 'rain', 'W': 'rain',
        'X': 'rain', 'Y': 'storm', 'Z': 'storm',
    }
    _WIND = (5.2, 4.2, 3.2, 1.05, -1.1, -3.15, -5.2, -8.35,
             -11.5, -9.4, -7.3, -5.25, -3.2, -1.15, 0.9, 3.05)
    _RISING = ('A', 'B', 'B', 'C', 'F', 'G', 'I', 'J', 'L', 'M', 'M', 'Q', 'T', 'Y')
    _FALLING = ('B', 'D', 'H', 'O', 'R', 'U', 'V', 'X', 'X', 'Z')
    _STEADY = ('A', 'B', 'B', 'B', 'E', 'K', 'N', 'N', 'P', 'P', 'S', 'W', 'W',
               'X', 'X', 'X', 'Z')

    def get_extension_list(self, timespan, db_lookup):
        extras = self.generator.skin_dict.get('Extras', {})
        enabled = str(extras.get('forecast', 'false')).strip().lower() \
            not in ('false', '0', 'no', 'off', '')
        if not enabled:
            return [{'zambretti': None}]
        try:
            code = self._code(db_lookup(), timespan.stop)
        except Exception:
            code = None
        if code is None:
            return [{'zambretti': None}]
        return [{'zambretti': {'code': code, 'text_key': self.PHRASES[code],
                               'icon': self.ICONS[code]}}]

    def _hpa(self, value, us_units):
        """Barometer to hPa (US stores inHg; metric systems store mbar == hPa)."""
        return value * 33.8639 if us_units == 1 else value

    def _code(self, db_manager, now_ts):
        import time
        table = db_manager.table_name
        row = db_manager.getSql(
            "SELECT barometer, windDir, windSpeed, usUnits FROM %s "
            "WHERE barometer IS NOT NULL ORDER BY dateTime DESC LIMIT 1" % table)
        if not row or row[0] is None:
            return None
        rel_pressure = self._hpa(row[0], row[3])
        wind_dir, wind_speed = row[1], row[2]

        past = db_manager.getSql(
            "SELECT barometer, usUnits FROM %s WHERE dateTime <= ? AND "
            "barometer IS NOT NULL ORDER BY dateTime DESC LIMIT 1" % table,
            (int(now_ts) - 10800,))
        if not past or past[0] is None:
            return None
        # Trend in hPa per hour (3-hour change / 3), as pywws expects.
        trend = (rel_pressure - self._hpa(past[0], past[1])) / 3.0

        try:
            north = float(self.generator.stn_info.latitude_f) >= 0
        except Exception:
            north = True

        # Normalise into the 950..1050 band (identity in range; clamps outside).
        pressure = 950.0 + (1050.0 - 950.0) * (rel_pressure - 950.0) / (1050.0 - 950.0)

        # Wind adjustment (only when there is meaningful wind).
        if wind_dir is not None and wind_speed is not None and wind_speed > 0.3:
            wind = int(wind_dir / 22.5 + 0.5) % 16
            if not north:
                wind = (wind + 8) % 16
            pressure += self._WIND[wind]

        month = int(time.strftime('%m', time.localtime(now_ts)))
        summer = north == (4 <= month <= 9)

        if trend >= 0.1:
            if summer:
                pressure += 3.2
            f = 0.1740 * (1031.40 - pressure)
            lut = self._RISING
        elif trend <= -0.1:
            if summer:
                pressure -= 3.2
            f = 0.1553 * (1029.95 - pressure)
            lut = self._FALLING
        else:
            f = 0.2314 * (1030.81 - pressure)
            lut = self._STEADY

        f = min(max(int(f + 0.5), 0), len(lut) - 1)
        return lut[f]

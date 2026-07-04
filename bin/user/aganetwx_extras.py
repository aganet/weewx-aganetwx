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

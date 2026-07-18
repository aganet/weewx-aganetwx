# AganetWX skin installer. Requires WeeWX 5.x.

import weewx
from weecfg.extension import ExtensionInstaller

REQUIRED_WEEWX = "5.0.0"


def loader():
    if weewx.__version__ < REQUIRED_WEEWX:
        raise weewx.UnsupportedFeature(
            "The AganetWX skin requires WeeWX %s or later; you have %s."
            % (REQUIRED_WEEWX, weewx.__version__))
    return AganetWXInstaller()


class AganetWXInstaller(ExtensionInstaller):
    def __init__(self):
        super(AganetWXInstaller, self).__init__(
            version="1.8.3",
            name="AganetWX",
            description="AganetWX: a configurable WeeWX dashboard skin with interactive charts, multiple layouts, and i18n.",
            author="George Anestopoulos",
            author_email="",
            config={
                "StdReport": {
                    "AganetWXReport": {
                        "skin": "AganetWX",
                        "enable": "true",
                        # Own subfolder; served at .../aganetwx/.
                        "HTML_ROOT": "aganetwx",
                        # Default UI language; loads lang/<lang>.conf.
                        "lang": "en",
                        # Units, timezone and formatting are inherited from your
                        # WeeWX config ([StdReport] Defaults / [Station]); the
                        # skin does not override them. Set unit_system here only
                        # if you want this report to differ from the rest.
                    }
                }
            },
            files=[
                ("bin/user", [
                    "bin/user/aganetwx_extras.py",
                ]),
                ("skins/AganetWX", [
                    "skins/AganetWX/skin.conf",
                    "skins/AganetWX/index.html.tmpl",
                    "skins/AganetWX/yesterday.html.tmpl",
                    "skins/AganetWX/week.html.tmpl",
                    "skins/AganetWX/month.html.tmpl",
                    "skins/AganetWX/year.html.tmpl",
                    "skins/AganetWX/lastyear.html.tmpl",
                    "skins/AganetWX/archive.html.tmpl",
                    "skins/AganetWX/about.html.tmpl",
                    "skins/AganetWX/about.inc.example",
                    "skins/AganetWX/_head.inc",
                    "skins/AganetWX/_foot.inc",
                    "skins/AganetWX/_nav.inc",
                    "skins/AganetWX/_periodbody.inc",
                    "skins/AganetWX/aganetwx.css",
                    "skins/AganetWX/aganetwx.js",
                    "skins/AganetWX/favicon.ico",
                    "skins/AganetWX/favicon.svg",
                    "skins/AganetWX/apple-touch-icon.png",
                ]),
                ("skins/AganetWX/data", [
                    "skins/AganetWX/data/day.json.tmpl",
                    "skins/AganetWX/data/yesterday.json.tmpl",
                    "skins/AganetWX/data/week.json.tmpl",
                    "skins/AganetWX/data/month.json.tmpl",
                    "skins/AganetWX/data/year.json.tmpl",
                    "skins/AganetWX/data/lastyear.json.tmpl",
                ]),
                ("skins/AganetWX/lang", [
                    "skins/AganetWX/lang/en.conf",
                    "skins/AganetWX/lang/el.conf",
                    "skins/AganetWX/lang/es.conf",
                    "skins/AganetWX/lang/fr.conf",
                    "skins/AganetWX/lang/de.conf",
                    "skins/AganetWX/lang/it.conf",
                    "skins/AganetWX/lang/pt.conf",
                ]),
                ("skins/AganetWX/NOAA", [
                    "skins/AganetWX/NOAA/NOAA-%Y-%m.txt.tmpl",
                    "skins/AganetWX/NOAA/NOAA-%Y.txt.tmpl",
                ]),
                ("skins/AganetWX/lib", [
                    "skins/AganetWX/lib/echarts.min.js",
                ]),
            ],
        )

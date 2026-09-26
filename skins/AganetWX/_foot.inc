## Shared footer + script tags. Honors $Extras.branding.*
#if $tf($getVar('Extras.disclaimer', True))
  <div class="disclaimer-bar"><span class="i18n" data-i18n="Disclaimer">$gettext('Disclaimer')</span></div>
#end if
  <div class="footer">
    <span class="st_type">$station.location</span>
    #set $f_link = $getVar('Extras.branding.link_url', '')
    #set $f_text = $getVar('Extras.branding.link_text', '')
    #if $f_link != ''
    <div><a href="$f_link">$f_text</a></div>
    #end if
    <small>#slurp
    #if $tf($getVar('Extras.branding.show_footer_coords', True))
$station.latitude[0]°$station.latitude[1]'$station.latitude[2] $station.longitude[0]°$station.longitude[1]'$station.longitude[2] | $station.altitude | #slurp
    #end if
Powered by <a href="https://weewx.com">WeeWX</a> $station.version</small>
    #if $tf($getVar('Extras.branding.show_skin_credit', True))
    <small><a href="https://github.com/aganet/weewx-aganetwx">AganetWX</a> $getVar('SKIN_VERSION', '')#if $tf($getVar('Extras.branding.show_theme_credit', True))# theme by <a href="https://anegio.com" rel="author">anegio.com</a>#end if#<span id="update-flag" hidden> &middot; <a href="https://github.com/aganet/weewx-aganetwx/releases/latest" target="_blank" rel="noopener">$lbl('update available')</a></span></small>
    #end if
  </div>

## Floating support link (plain link, no third-party widget) and scroll-to-top.
#set $c_url = $getVar('Extras.support.url', '')
#if $tf($getVar('Extras.support.enable', False)) and $c_url != ''
## text: unset falls back to the translated label; a blank string (a space in
## weewx.conf) means the caller wants the compact icon-only button.
#set $c_raw = $getVar('Extras.support.text', None)
#set $c_text = $gettext('Support this station') if $c_raw is None else $str($c_raw).strip()
#set $c_tip = $str($getVar('Extras.support.tooltip', '')).strip()
#set $c_label = $c_tip if $c_tip != '' else $gettext('Support this weather station')
  <a class="support-float#if $c_text == ''# support-float-icon#end if#" href="$c_url" target="_blank" rel="noopener" title="$c_label" aria-label="$c_label">
    <svg class="support-cup" viewBox="0 0 24 24" aria-hidden="true"><path d="M5.6 0.95c-1.1 1.05-1.1 2.15 0 3.25s1.1 2.15 0 2.9" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linecap="round"/><path d="M9.6 0.95c-1.1 1.05-1.1 2.15 0 3.25s1.1 2.15 0 2.9" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linecap="round"/><path d="M13.6 0.95c-1.1 1.05-1.1 2.15 0 3.25s1.1 2.15 0 2.9" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linecap="round"/><path d="M1.6 9h15.6v6.2c0 3.5-2.85 6.35-6.35 6.35H7.95c-3.5 0-6.35-2.85-6.35-6.35z" fill="none" stroke="currentColor" stroke-width="2" stroke-linejoin="round"/><path d="M17.3 11.1h1.15c2.05 0 3.7 1.65 3.7 3.7s-1.65 3.7-3.7 3.7H17.3" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"/><path d="M1.4 22.9h16" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"/><path class="support-heart" d="M9.4 19.3c-2.8-2.1-4.55-3.85-4.55-5.85 0-1.55 1.25-2.8 2.8-2.8.8 0 1.55.36 1.75 1 .2-.64.95-1 1.75-1 1.55 0 2.8 1.25 2.8 2.8 0 2-1.75 3.75-4.55 5.85z" fill="#e2342f"/></svg>#slurp
#if $c_text != ''#<span>$c_text</span>#end if#
  </a>
#end if
#if $tf($getVar('Extras.back_to_top', True))
  <button class="to-top" type="button" aria-label="$gettext('Scroll to top')" hidden>
    <svg viewBox="0 0 24 24" aria-hidden="true" fill="currentColor"><path d="M11 20h2v-9.2l4.1 4.1 1.4-1.4L12 6.9l-6.5 6.6 1.4 1.4L11 10.8z"/></svg>
  </button>
#end if

  <script src="aganetwx.js?v=$getVar('SKIN_VERSION', '')" defer></script>
</body>
</html>

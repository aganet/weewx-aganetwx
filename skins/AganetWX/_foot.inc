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
#set $c_text = $str($getVar('Extras.support.text', '')).strip()
#set $c_label = $c_text if $c_text != '' else $gettext('Buy me a coffee')
  <a class="support-float#if $c_text == ''# support-float-icon#end if#" href="$c_url" target="_blank" rel="noopener" title="$c_label" aria-label="$c_label">
    <svg viewBox="0 0 24 24" aria-hidden="true" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M17 8h1a4 4 0 0 1 0 8h-1"/><path d="M3 8h14v9a4 4 0 0 1-4 4H7a4 4 0 0 1-4-4Z"/><path d="M6 2v2M10 2v2M14 2v2"/></svg>#slurp
#if $c_text != ''#<span>$c_text</span>#end if#
  </a>
#end if
#if $tf($getVar('Extras.back_to_top', True))
  <button class="to-top" type="button" aria-label="$gettext('Scroll to top')" hidden>
    <svg viewBox="0 0 24 24" aria-hidden="true" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 19V5M5 12l7-7 7 7"/></svg>
  </button>
#end if

  <script src="aganetwx.js?v=$getVar('SKIN_VERSION', '')" defer></script>
</body>
</html>

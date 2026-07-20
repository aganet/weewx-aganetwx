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

  <script src="aganetwx.js?v=$getVar('SKIN_VERSION', '')" defer></script>
</body>
</html>

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
    <small><a href="https://github.com/aganet/weewx-aganetwx">AganetWX</a> $getVar('SKIN_VERSION', '') theme by <a href="https://anegio.com" rel="author">anegio.com</a></small>
  </div>

  <script src="aganetwx.js" defer></script>
</body>
</html>

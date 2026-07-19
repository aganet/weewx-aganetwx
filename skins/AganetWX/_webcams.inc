## Renders the extra-cameras grid from $extra_webcams (a list of cam dicts,
## normalized by the AganetWXExtras search-list extension). Used by the
## dedicated Cameras page. Each cam keeps the cache-busted first frame, optional
## countdown badge, optional click-through link, and click-to-enlarge lightbox.
#set $cams = $getVar('extra_webcams', [])
#set $cols = $int($getVar('webcam_columns', 0) or 0)
#if $cams
  ## Fixed column count when set (1..6); otherwise a responsive auto-fit grid.
  #set $grid_style = 'grid-template-columns:repeat(' + $str($cols) + ',1fr);' if $cols > 0 else ''
  <div class="webcam-grid#if $len($cams) == 1# webcam-grid-single#end if#"#if $grid_style# style="$grid_style"#end if#>
    #for $cam in $cams
    #set $c_align = $cam.get('align', 'center')
    #if $c_align not in ('left', 'right', 'center')
    #set $c_align = 'center'
    #end if
    #set $c_style = ''
    #if $cam.get('max_width')#
    #set $c_style = $c_style + 'max-width:' + $str($int($cam.get('max_width'))) + 'px;'
    #end if
    #if $cam.get('height')#
    #set $c_style = $c_style + 'max-height:' + $str($int($cam.get('height'))) + 'px;'
    #end if
    #set $c_zoom = not $cam.get('link')
    #set $c_url = $cam.get('url')
    #set $c_sep = '&' if '?' in $c_url else '?'
    #set $c_refresh = $int($cam.get('refresh', 0))
    <div class="webcam-wrap wc-$c_align">
      <div class="webcam-box"#if $c_style# style="$c_style"#end if#>
        #if $cam.get('link')#<a href="$cam.get('link')" target="_blank" rel="noopener">#end if#
        <img class="webcam-img#if $c_zoom# webcam-zoomable#end if#" src="${c_url}${c_sep}_t=$current.dateTime.raw" alt="$cam.get('title', 'Webcam')"
             data-src="$c_url" data-refresh="$c_refresh"#if $c_zoom# tabindex="0" role="button"#end if#>
        #if $cam.get('link')#</a>#end if#
        #if $c_refresh > 0
        <div class="webcam-live"><span class="webcam-dot"></span>$lbl('Refreshing in') <span class="webcam-count">$c_refresh</span>$lbl('s (short for seconds)')</div>
        #end if
        #if $cam.get('title')#<div class="webcam-cap">$cam.get('title')</div>#end if#
      </div>
    </div>
    #end for
  </div>
  <div id="webcam-lightbox" class="webcam-lightbox" hidden>
    <button type="button" class="webcam-lightbox-close" aria-label="Close">&times;</button>
    <img class="webcam-lightbox-img" src="" alt="Webcam">
  </div>
#end if

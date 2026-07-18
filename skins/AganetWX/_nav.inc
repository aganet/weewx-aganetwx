## Shared top navigation bar. The caller sets $active to one of:
## current | yesterday | week | month | year | lastyear | archive | about
#def navlink($id, $href, $key)
#if $id == $active
<a class="navtab active" href="$href" aria-current="page"><span class="i18n" data-i18n="$key">$gettext($key)</span></a>#slurp
#else
<a class="navtab" href="$href"><span class="i18n" data-i18n="$key">$gettext($key)</span></a>#slurp
#end if
#end def
<nav class="m-nav" aria-label="$gettext('Navigation')">
  <div class="m-nav-inner">
    #if $tf($getVar('Extras.nav.current', True))
    $navlink('current',   'index.html',     'Current')
    #end if
    #if $tf($getVar('Extras.nav.yesterday', True))
    $navlink('yesterday', 'yesterday.html', 'Yesterday')
    #end if
    #if $tf($getVar('Extras.nav.week', True))
    $navlink('week',      'week.html',      'Week')
    #end if
    #if $tf($getVar('Extras.nav.month', True))
    $navlink('month',     'month.html',     'Month')
    #end if
    #if $tf($getVar('Extras.nav.year', True))
    $navlink('year',      'year.html',      'Year')
    #end if
    #if $tf($getVar('Extras.nav.lastyear', True))
    $navlink('lastyear',  'lastyear.html',  'Last Year')
    #end if
    #if $tf($getVar('Extras.nav.archive', True))
    $navlink('archive',   'archive.html',   'Archive')
    #end if
    #if $tf($getVar('Extras.compare_page', False))
    $navlink('compare',   'compare.html',   'History')
    #end if
    #if $tf($getVar('Extras.nav.about', True))
    $navlink('about',     'about.html',     'About')
    #end if
  </div>
</nav>

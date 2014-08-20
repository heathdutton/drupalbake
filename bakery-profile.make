api = 2
core = 7.x

; --------
; Modules

; jQuery Update is needed for Bootstrap 3 (the current theme I'm using)
projects[jquery_update][version] = "2.4"
projects[jquery_update][type] = "module"
projects[jquery_update][subdir] = "contrib"

; Views to display list of recipes created
projects[views][version] = "3.8"
projects[views][type] = "module"
projects[views][subdir] = "contrib"

; Views Isotope for cool filtering / arranging
projects[views_isotope][version] = "2.0-alpha1"
projects[views_isotope][type] = "module"
projects[views_isotope][subdir] = "contrib"

; Queue UI for viewing the list of batch processing jobs waiting for completion.
projects[queue_ui][version] = "2.0-rc1"
projects[queue_ui][type] = "module"
projects[queue_ui][subdir] = "contrib"

; Elysia Cron, for recieving and firing off "workers" from the queue
; and thus building drupal gzips with drush.
projects[elysia_cron][version] = "2.1"
projects[elysia_cron][type] = "module"
projects[elysia_cron][subdir] = "contrib"

; THINGS NOT IN USE AT THIS TIME

; Features will likely be handy for saving/reverting configuration later
; projects[features][version] = "2.2"
; projects[features][type] = "module"
; projects[features][subdir] = "contrib"

; Feeds Import to import Projects from Drupal.org
projects[feed_import][version] = "7.x-3.3"
projects[feed_import][type] = "module"
projects[feed_import][subdir] = "contrib"


; Themes
; --------

; Bootstrap is a good fit for a tiny app like this
projects[bootstrap][version] = "3.0"
projects[bootstrap][type] = "theme"
projects[bootstrap][subdir] = "contrib"


; Libraries
; ---------

; Drush - Essential, and best retrieved from github (6.x is deceptively the stable release for D7 currently)
; libraries[drush][download][type] = "git"
; libraries[drush][download][url] = "https://github.com/drush-ops/drush.git"
; libraries[drush][download][branch] = "6.x"

; Isotope library - For Views Isotope
libraries[isotope][download][type] = "git"
libraries[isotope][download][url] = "https://github.com/metafizzy/isotope.git"
libraries[isotope][download][branch] = "master"

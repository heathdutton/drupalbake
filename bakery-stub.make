; Drupal Bake - Drupal distribution generator built in Drupal.
;
; In this project you see a make file
;                 that will make a project
;                        to make profiles
;                 that will make
;                           make files for a project.  ^‿^

api = 2
core = 7.x
includes[] = drupal-org-core.make

; Add the l10n_update installation profile, because we will merge with it
projects[l10n_install][type] = "profile"
projects[l10n_install][version] = "1.0-rc4"

; Drupal.org drush - Adds features so that this can be a big-boy distro (if applicable in future)
libraries[drupalorg_drush][download][type] = "git"
libraries[drupalorg_drush][download][url] = "http://git.drupal.org/project/drupalorg_drush.git"
libraries[drupalorg_drush][download][branch] = "7.x-1.x"
libraries[drupalorg_drush][download][subtree] = "drupalorg_drush"
libraries[drupalorg_drush][destination] = "drush"

; Add our own profile for baking
projects[bakery][type] = "profile"
projects[bakery][download][type] = "file"
projects[bakery][download][url] = "./bakery-profile.make"

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

; Add our own profile for baking
projects[drupalbake-profile][type] = "profile"
projects[drupalbake-profile][download][type] = "file"
projects[drupalbake-profile][download][url] = "./drupalbake-profile.make"

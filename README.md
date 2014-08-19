drupalbake
==========

Drupal distribution generator.


Developer tools
===============

$ bake.sh - Build out the /drupal and /vendor directories for the project.

$ bake-dev.sh - Rebuilds the directories, then watches for changes in /custom, instantly mirroring those changes as needed.

$ bake-dist.sh - Rebuilds, and makes a flat zip file distribution.

Optional
--------

Integrate gulp in future?

Gulp
$ npm install --save-dev gulp
$ gulp

The Custom Folder
=================

Things should only go in this folder if they are one-off usages to speed development.

The drupal and vendor paths are automatically merged over the respective folders in the project root.

Once a module is stable and ready to be put into a sandbox or standalone repository,
it should be removed from this path and included in your profile.make file.

If you wish to prepend to a file instead of replace it, prefix your file with "prepend___".

Similarly, if you'd like to append a file, append "append___" to your file name.
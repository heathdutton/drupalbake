<?php

/**
 *   Drush Merge
 *
 *   Merges installation profiles.
 */

/**
 * Default name of the merged installation profile
 */
define('MERGE_DEFAULT_NAME', 'Merged');

/**
 * Default description of the merged installation profile
 * "auto" = Will be replaced with a merged description
 */
define('MERGE_DEFAULT_DESCRIPTION', 'auto');

/**
 * Default exclusivity of the merged installation profile
 */
define('MERGE_DEFAULT_EXCLUSIVE', 'false');

/**
 * Default overwrite behavior
 */
define('MERGE_DEFAULT_OVERWRITE', 'false');

/**
 * Default profiles to ignore (comma delimited)
 */
define('MERGE_DEFAULT_EXCLUDE', 'standard,minimal,testing');

/**
 * Default cleanup behavior (deleting of remaining profiles)
 */
define('MERGE_DEFAULT_CLEANUP', 'false');

/**
 * Default profiles to merge. If auto, we will grab all profiles not ignored.
 */
define('MERGE_DEFAULT_INCLUDE', 'auto');

/**
 * Implementation of hook_drush_command().
 *
 * @return array
 */
function merge_drush_command() {
  $items = array();

  // The 'merge' command
  $items['merge'] = array(
    'description' => "Merges all custom installation profiles into one.",
    'options' => array(
      'name' => 'Optionally set the name of the resulting installation profile. Default: "merged"',
      'description' => 'Optionally define a description of the resulting installation profile. Defaults to a combination of descriptions.',
      'exclusive' => 'Set to "false" to allow other installation profiles to be used. Default: "true"',
      'force' => 'Set to "false" to prevent overwrite of an existing merged profile of the same name. Default: "true"',
      'profiles-included' => 'Optionally list the profiles you want to merge (by machine name). Useful to merge in a particular order.',
      'profiles-excluded' => 'Optionally define a list of profiles to ignore (comma delimited). Default: "standard,minimal,testing"',
      'cleanup' => 'Optionally remove all other installation profiles to save space. Default: "false"'
    ),
    'examples' => array(
      'drush merge' => 'Merges all non-standard profiles into a new one called "merged" and forces it to be used.',
      'drush merge --exclusive=false' => 'Merges all non-default profiles and still allows the user to choose which profile they want.',
      'drush merge --exclusive=false --name="Awesome Sauce"' => 'Merges all non-default profiles into one called "awesome_sauce", and allows the user choose.',
      'drush merge --cleanup=true' => 'Merges all non-default profiles and deletes all but that one.',
      'drush merge --exclude' => 'Merges ALL profiles into one (none are excluded).'
    ),
    'aliases' => array('merge-profiles'),
    'bootstrap' => DRUSH_BOOTSTRAP_MAX,
    // Get as high as we can (so we have the path, at least)
  );

  return $items;
}

/**
 * Implementation of hook_drush_help().
 *
 * @param
 *   A string with the help section (prepend with 'drush:')
 *
 * @return string
 */
function merge_drush_help($section) {
  switch ($section) {
    case 'drush:merge':
      return dt("Assists in merging of installation profiles into one for rapid deployment.");
  }
}

function drush_merge() {

  // Get option defaults
  $options = array(
    'name' => MERGE_DEFAULT_NAME,
    'description' => MERGE_DEFAULT_DESCRIPTION,
    'exclusive' => MERGE_DEFAULT_EXCLUSIVE,
    'overwrite' => MERGE_DEFAULT_OVERWRITE,
    'profiles-excluded' => MERGE_DEFAULT_EXCLUDE,
    'profiles-included' => MERGE_DEFAULT_INCLUDE,
    'cleanup' => MERGE_DEFAULT_CLEANUP
  );

  // Import options the user may have defined, fall-back to defaults
  foreach ($options as $key => &$option) {
    $option = drush_get_option($key, $option);
  }

  // Get the path to Drupal's root
  $base_path = drush_get_context('DRUSH_DRUPAL_ROOT');
  if ($base_path) {

    // Get list of functional profiles in the Drupal instance
    $profiles = drush_find_profiles($base_path);
    if ($profiles) {

      // Define machine-name for our new profile
      $machine_name = _merge_machine_name($options['name']);

      // Determine which profiles should be included
      $profiles_to_include = explode(',', $options['profiles-included']);

      // Prevent a merge profile from including itself, to prevent recursion
      $profiles_to_include = array_diff($profiles_to_include, array(
        $machine_name,
        'auto'
      ));

      // Determine which profiles should be excluded
      $profiles_to_exclude = explode(',', $options['profiles-excluded']);

      // If the merged profile already exists, ignore it to prevent recursion
      $profiles_to_exclude[] = $machine_name;

      // Remove exclusions that are explicitly listed in inclusions
      if ($profiles_to_include) {
        $profiles_to_exclude = array_diff($profiles_to_exclude, $profiles_to_include);
      }

      // Remove profiles from the array that should be excluded
      foreach ($profiles as $profile_name => $profile) {
        if (
          // Inclusions are specified, and this is NOT one of them
          ($profiles_to_include && !in_array($profile_name, $profiles_to_include))
          // Exclusions are specified, and this IS one of them
          || ($profiles_to_exclude && in_array($profile_name, $profiles_to_exclude))
        ) {
          unset($profiles[$profile_name]);
        }
      }

      // Should now have 2 or more installation profiles to merge
      $profile_count = count($profiles);
      if ($profile_count > 1) {

        // Check to see if the designated destination of this profile already exists.
        $new_profile_final_path = $base_path . '/profiles/' . $machine_name;
        if (!file_exists($new_profile_final_path)
          || (file_exists($new_profile_final_path) && $options['overwrite'] != 'true')
        ) {
          // Either the folder doesn't exist, or we are meant to overwrite it.

          // If inclusion was specified, re-order remaining profiles based on that order specified.
          if ($profiles_to_include) {
            $reordered_profiles = array();
            foreach ($profiles_to_include as $profile_name) {
              if (isset($profiles[$profile_name])) {
                $reordered_profiles[$profile_name] = $profiles[$profile_name];
              }
            }
            $profiles = $reordered_profiles;
            unset($reordered_profiles);
          }

          // We can let the user know what we are doing now, as it could take a minute.
          drush_log(
            dt(
              'Merging installation profiles: @profile_list ===> @machine_name',
              array(
                '@profile_list' => implode(', ', array_keys($profiles)),
                '@machine_name' => $machine_name
              )),
            'status');

          // We'll be working with a temporary folder until we are done with the process.
          $tmp_dir = drush_tempdir();
          $new_profile_tmp_dir = $tmp_dir . '/new_profile';
          drush_mkdir($new_profile_tmp_dir);
          drush_register_file_for_deletion($new_profile_tmp_dir);

          $profile_files = array();

          // Stub the drupal_alter, because we do not necessarily have a
          // functional Drupal intall, and do not need that level of detail.
          if (!function_exists('drupal_alter')) {
            function drupal_alter($type, &$data, &$context1 = NULL, &$context2 = NULL, &$context3 = NULL) {
              return NULL;
            }
          }

          // We'll need some common Drupal stuff for this next bit
          include_once $base_path . '/includes/common.inc';

          // Loop through profiles, collecting projects
          $modules =
            $themes =
            $installs =
            $infos =
            $translations =
            $licenses =
            $readmes =
            $libraries = array();
          foreach ($profiles as $profile_name => $profile) {
            // Recursively copy files, ignoring files at the root of the profile

            // Find new modules
            $profile_path = $base_path . '/profiles/' . $profile_name;
            if (is_dir($profile_path . '/modules')) {

              $new_modules = _merge_rebuild_module_data('profiles/' . $profile_name . '/modules');

              // Merge in those modules
              _merge_project_merge($modules, $new_modules);
            }

            // Find new themes
            if (is_dir($profile_path . '/themes')) {

              $new_themes = _merge_rebuild_theme_data('profiles/' . $profile_name . '/themes');
              // Merge in those themes
              _merge_project_merge($themes, $new_themes);
            }

            // Find any/all translation files to merge
            // Should probably be more deterministic about these in future.
            if (is_dir($profile_path . '/translations')) {
              $new_translations = drush_scan_directory(
                $profile_path . '/translations',
                "/.*\.po/",
                array('.', '..', 'CVS', 'tests'), 0, 2);
              if ($new_translations) {
                $translations = array_merge($translations, $new_translations);
              }
            }

            // Find any/all library files to merge
            // Dangerously nondeterministic!
            if (is_dir($profile_path . '/libraries')) {
              $new_libraries = _merge_get_subdirs($profile_path . '/libraries');
              if ($new_libraries) {
                $libraries = array_merge($libraries, $new_libraries);
              }
            }

            // Find install file for this profile (even if nested in there)
            $new_installs = drush_scan_directory(
              $profile_path,
              "/" . preg_quote($profile_name) . "\.install$/",
              array('.', '..', 'CVS', 'tests'), 0, 2);
            if ($new_installs) {
              $installs = array_merge($installs, $new_installs);
            }

            // Find info files for this profile (even if nested in there)
            $new_infos = drush_scan_directory(
              $profile_path,
              "/" . preg_quote($profile_name) . "\.info/",
              array('.', '..', 'CVS', 'tests'), 0, 2);
            if ($new_infos) {
              $infos = array_merge($infos, $new_infos);
            }

            // Find licenses
            $new_licenses = drush_scan_directory(
              $profile_path,
              "/LICENSE\..*/i",
              array('.', '..', 'CVS', 'tests'), 0, 1);
            if ($new_licenses) {
              $licenses = array_merge($licenses, $new_licenses);
            }

            // Find Readmes
            $new_readmes = drush_scan_directory(
              $profile_path . '/',
              "/README\..*/i",
              array('.', '..', 'CVS', 'tests'), 0, 1);
            if ($new_readmes) {
              $readmes = array_merge($readmes, $new_readmes);
            }
          }

          // Merge modules
          _merge_copy_bulk($modules, $base_path, $new_profile_tmp_dir, 'modules');
          // Merge themes
          _merge_copy_bulk($themes, $base_path, $new_profile_tmp_dir, 'themes');

          // Merge translations
          _merge_copy_bulk($translations, $base_path, $new_profile_tmp_dir, 'translations');

          // Merge libraries
          _merge_copy_bulk($libraries, $base_path, $new_profile_tmp_dir, 'libraries');

          // Merge .info files
          _merge_info_syntax($infos, $new_profile_tmp_dir, $options['name'], $machine_name);


          // TO DO +++==================================


          // Merge .profile files

          // Merge .install files

          // Merge LICENSEs

          // Merge READMEs

          // Drop in a Readme explaining what we've done.

          // Move the old profile out of the way, if needed
          $overwriting = file_exists($new_profile_final_path);
          if ($overwriting) {
            $old_profile_tmp_dir = $tmp_dir . '/old_profile';
            drush_move_dir($new_profile_final_path, $old_profile_tmp_dir);
          }

          // Move the new profile into place from the temp dir
          if (drush_move_dir($new_profile_tmp_dir, $new_profile_final_path)) {

            // All went well! Let's celebrate.
            drush_log(
              dt(
                'Installation profile "@machine_name" is complete.',
                array(
                  '@machine_name' => $machine_name
                )),
              'status');
          }
          else {

            // Revert changes because the move failed
            if ($overwriting) {
              if (file_exists($new_profile_final_path)) {
                rmdirr($new_profile_final_path);
              }
              drush_move_dir($old_profile_tmp_dir, $new_profile_final_path);
            }

            drush_set_error(
              'MERGE_COULD_NOT_MOVE',
              dt('I could not move your new profile into place. Any changes have been reverted.')
            );
          }
        }
        else {
          drush_set_error(
            'MERGE_PROFILE_EXISTS',
            dt('A profile already exists at "@profile_path". Try using --overwrite=true',
               array('@profile_path' => $new_profile_final_path)
            )
          );
        }
      }
      else {
        drush_log(dt('Installation profile merge not necessary. Only @profile_count profile/s specified.', array('@profile_count' => $profile_count)), 'status');
      }
    }
    else {
      drush_set_error('MERGE_NO_PROFILES_FOUND', dt('No installation profiles found.'));
    }
  }
  else {
    drush_set_error('MERGE_ROOT_PATH_NOT_FOUND', dt('Could not find the Drupal root path.'));
  }
}

/**
 * Get a list of subdirectories.
 * Used for aggregating libraries safely.
 *
 * @param $dir
 */
function _merge_get_subdirs($dir){
  $subdirs = array();
  if (is_string($dir) && is_dir($dir) && $handle = opendir($dir)) {
    while (FALSE !== ($subdir = readdir($handle))) {
      if ($subdir != '..' && $subdir != '.' && is_dir($dir . '/' . $subdir)){
        $subdirs[$subdir] = new stdClass();
        $subdirs[$subdir]->name = $subdir;
        $subdirs[$subdir]->filename = $dir . '/' . $subdir;
      }
    }
  }
  return $subdirs;
}

/**
 * Parse an array of info-syntax files and save the result
 *
 * @param $items
 * @param $destination_folder
 * @param $new_profile_name
 * @param $machine_name
 */
function _merge_info_syntax($items, $destination_folder, $new_profile_name, $machine_name){
  $info = array();

  $descriptions = array();
  $core = drush_drupal_major_version();
  // Some values should be excluded from the merge (using blacklist for futureproofness)
  $excluded_keys = array('name', 'description', 'version', 'core', 'version', 'core', 'project', 'datestamp');
  foreach ($items as $item){

    // Parse the file
    $new_info = drush_drupal_parse_info_file($item->filename);

    // Collect the name and desription for later use as a concatenated description
    $descriptions[] = $new_info['name'] . (isset($new_info['description']) ? ' (' . $new_info['description'] . ')' : '');

    // Get core value and ensure we are consistent with this Drupal core
    if (isset($new_info['core'])){
      $item_core = trim(str_replace(array('.', 'x', 'X', '*'), '', $new_info['core']));
      if ($item_core && $item_core != $core){
        drush_set_error(
          'MERGE_CORE_CONFLICT',
          dt('An item is not @version compatible: @item', array(
            '@version' => $core,
            '@file' => (isset($new_info['name']) ? $new_info['name'] : $item->basename)
          ))
        );
      }
    }

    // Exclude bits that we don't need before recursive merge
    foreach ($excluded_keys as $key){
      if (isset($new_info[$key])) {
        unset($new_info[$key]);
      }
    }

    // Correct files, if they are looking for a file that is profile specific in name
    if (isset($new_info['files'])){
      foreach ($new_info['files'] as &$file_name){
        $segments = explode('.', $file_name);
        if ($segments && $segments[0] == $item->name){
          // Redirect to a new equivalent of this file, assuming the merge will create it
          $segments[0] = $machine_name;
          $file_name = implode('.', $segments);
        }
      }
    }

    // Merge the remaining data into the info array if applicable
    if (count($new_info)){
      $info = array_merge_recursive($info, $new_info);
    }
  }
  // Finally, generate the singular values
  $top_info = array();
  $top_info['name'] = $new_profile_name;
  $top_info['description'] = dt('This is a merged profile, generated from: ') . implode(", ", $descriptions);
  $top_info['project'] = $machine_name;
  $top_info['core'] = $core . '.x';

  // And make sure those values are at the top as they should be
  $info = array_merge($top_info, $info);

  // Save the info file to it's destination
  $info_string = _merge_info_generate($info);

  file_put_contents($destination_folder . '/' . $machine_name . '.info', $info_string);
}

function _merge_info_generate($info, $prefix = ''){
  $result = '';
  foreach ($info as $key => $value) {
    if (is_array($value)){
      // @todo - Remove duplicate project/lib/file dependencies for performance & clarity
      $result .= _merge_info_generate($value, $key);
    }
    else {
      if (is_numeric($key)) {
        $key = '';
      }
      if ($prefix){
        $key = '[' . $key . ']';
      }
      $result .= $prefix . $key . ' = ' . $value . "\n";
    }
  }
  return $result;
}


/**
 * Mirror files to the destination root
 *
 * @param $items
 * @param $destination_root
 * @param $type
 */
function _merge_copy_bulk($items, $base_path, $destination_root, $type){
  $paths = array();
  switch($type){
    case 'modules':
    case 'themes':
      foreach ($items as $item){
        // For modules/themes we want to copy the root of the path,
        // which is where the .module/.theme file lives
        $path_segments = explode('/', $item->filename);
        // Remove the extension from the path
        array_pop($path_segments);
        // Make the path literal
        $relative_source = implode('/', $path_segments);
        $relative_destination = str_replace($path_segments[0] . '/' . $path_segments[1] . '/', '', $relative_source);
        $paths[$item->name] = array(
          'source' => $base_path . '/' . $relative_source,
          'destination' => $destination_root . '/' . $relative_destination
        );
      }
      break;
    case 'libraries':
    case 'translations':
      foreach ($items as $item){
        // For these, they are straight file-copies,
        // but we need a relative destination path from the absolute one provided.
        $path_segments = explode('/', $item->filename);
        $relative_source = str_replace($base_path . '/profiles/', '', $item->filename);
        $path_segments = explode('/', $relative_source);
        array_shift($path_segments);
        $relative_source = implode('/', $path_segments);

        $paths[$item->name] = array(
          'source' => $item->filename,
          'destination' => $destination_root . '/' . $relative_source
        );
      }
      break;

  }

  // Loop through files/folders and merge
  foreach ($paths as $project_name => $path){
    // Determine the relative destination path
    _merge_copy($path['source'], $path['destination']);
  }
}

/**
 * Recursively copy a directory, or file
 *
 * @param $source
 * @param $destination
 */
function _merge_copy($source, $destination) {
  if (is_dir($source)) {
    // Ensure the destination folder exists before we begin
    drush_mkdir($destination, TRUE);

    // Recursively copy folders
    drush_copy_dir($source, $destination, FILE_EXISTS_MERGE);
  }
  elseif (is_file($source)) {
    // Same as above, but for the path to this file
    $segments = explode('/', $destination);
    array_pop($segments);
    drush_mkdir(implode('/', $segments), TRUE);

    // Copy the file
    copy($source, $destination);
  }
}

/**
 * Generate a machine-readable name
 *
 * @param $name
 * @return mixed|string
 */
function _merge_machine_name($name) {
  $machine_name = preg_replace('@[^a-z0-9_]+@', '_',
                               strtolower(trim($name))
  );
  while (strpos($machine_name, '__')) {
    $machine_name = str_replace('__', '_', $machine_name);
  }
  return $machine_name;
}

/**
 * Recursively copy a profile's files, making note of root-level files
 *
 * @param $source
 * @param $dest
 * @param $profile
 * @param bool $top_level
 * @return int
 */
//function _merge_profile_copy($source, $dest, $profile_name, $top_level = TRUE) {
//  global $misc_files, $profile_files;
//  $file_count = 0;
//  if (is_dir($source)) {
//    $dir_handle = opendir($source);
//    while ($file = readdir($dir_handle)) {
//      if ($file != "." && $file != "..") {
//        $source = $source . '/' . $file;
//        if (is_dir($source)) {
//          if (!is_dir($dest . '/' . $file)) {
//            mkdir($dest . '/' . $file, 0700);
//          }
//          $file_count += _merge_profile_copy($source, $dest . '/' . $file, $profile_name, FALSE);
//        }
//        elseif (!$top_level) {
//          copy($source, $dest . '/' . $file);
//          $file_count++;
//        }
//      }
//    }
//    closedir($dir_handle);
//  }
//  // Do not copy files of the top level... we want to do that manually
//  elseif (!$top_level) {
//    copy($source, $dest);
//    $file_count++;
//  }
//
//  // If we're on the top level, we want to store the source path for later based on type.
//  if ($top_level) {
//    _merge_profile_file_sort($source, $profile_name);
//  }
//  return $file_count;
//}
//
///**
// * Evaluate if the file is a "profile" file, meaning it must be merged.
// *
// * @param $file
// * @param $profile_name
// */
//function _merge_profile_file_sort($file_path, $profile_path, $profile_name, $top_level) {
//  global $profile_files;
//  $is_profile_file = FALSE;
//
//  $file_extension = pathinfo($file_path, PATHINFO_EXTENSION);
//
//  // Check the file extension for one of the profile-relevant file extensions
//  if (in_array(
//    $file_extension,
//    array(
//      'info',
//      'profile',
//      'install'
//    ))
//  ) {
//    // Great, now check that the file name is exactly profile_name.extension
//    $file_name = pathinfo($file_path, PATHINFO_FILENAME);
//    if (str_replace($profile_name . '.' . $file_extension, '', $file_name) == $profile_name) {
//      // Yep, this file needs to be merged
//      $is_profile_file = TRUE;
//    }
//  }
//
//  // Figure out where we should sort the file, default to misc if it isn't pertinent.
//  $file_type = $is_profile_file ? 'unknown' : $file_extension;
//
//  // We need to treat modules and themes differently, so let's check for that
//  if (!$top_level && $file_type == 'unknown') {
//
//    // I am being intentionally non-deterministic here to be kind to those who might
//    // shove libraries or hacked files into a profile, etc. We will preserve their junk.
//    // We can't just re-generate their dependencies because we won't always have version numbers.
//    // So copying modules and themes is straight forward, except for when there's a conflict,
//    // in which case we want the latest iteration (in the chain of profiles), and not copying the first iteration.
//    if (strpos($file_path, $profile_path . '/modules/') === 0) {
//      // This is likely part of a module.
//      $file_type = 'modules';
//    }
//    elseif (strpos($file_path, $profile_path . '/themes/') === 0) {
//      // This is likely part of a module.
//      $file_type = 'modules';
//    }
//
//    if ($file_type !== 'unknown') {
//      $file_relative_path = substr($file_path, strlen($profile_path . '/' . $file_type . '/') - 1);
//
//    }
//  }
//
//  if (!isset($profile_files[$file_type])) {
//    $profile_files[$file_type] = array();
//  }
//  $profile_files[$file_type][] = $file_path;
//
//  return $is_profile_file;
//}


/**
 * Helper function to scan and collect module .info data.
 *
 * Ripped & modified from system.module.
 *
 * @return
 *   An associative array of module information.
 */
function _merge_rebuild_module_data($profile_path) {
  // Find modules
  $modules = drupal_system_listing('/^' . DRUPAL_PHP_FUNCTION_PATTERN . '\.module$/', $profile_path, 'name', 0);

// Exclude the profile, because it isn't actually a module.
//  // Include the installation profile in modules that are loaded.
//  $profile = drupal_get_profile();
//  $modules[$profile] = new stdClass();
//  $modules[$profile]->name = $profile;
//  $modules[$profile]->uri = 'profiles/' . $profile . '/' . $profile . '.profile';
//  $modules[$profile]->filename = $profile . '.profile';
//
//  // Installation profile hooks are always executed last.
//  $modules[$profile]->weight = 1000;

  // Set defaults for module info.
  $defaults = array(
    'dependencies' => array(),
    'description' => '',
    'package' => 'Other',
    'version' => NULL,
    'php' => DRUPAL_MINIMUM_PHP,
    'files' => array(),
    'bootstrap' => 0,
  );

  // Read info files for each module.
  foreach ($modules as $key => $module) {
    // The module system uses the key 'filename' instead of 'uri' so copy the
    // value so it will be used by the modules system.
    $modules[$key]->filename = $module->uri;

    // Look for the info file.
    $module->info = drupal_parse_info_file(dirname($module->uri) . '/' . $module->name . '.info');

    // Skip modules that don't provide info.
    if (empty($module->info)) {
      unset($modules[$key]);
      continue;
    }

    // Merge in defaults and save.
    $modules[$key]->info = $module->info + $defaults;

    // Prefix stylesheets and scripts with module path.
    $path = dirname($module->uri);
    if (isset($module->info['stylesheets'])) {
      $module->info['stylesheets'] = _system_info_add_path($module->info['stylesheets'], $path);
    }
    if (isset($module->info['scripts'])) {
      $module->info['scripts'] = _system_info_add_path($module->info['scripts'], $path);
    }

    // Installation profiles are hidden by default, unless explicitly specified
    // otherwise in the .info file.
    if ($key == $profile && !isset($modules[$key]->info['hidden'])) {
      $modules[$key]->info['hidden'] = TRUE;
    }

    // Invoke hook_system_info_alter() to give installed modules a chance to
    // modify the data in the .info files if necessary.
    $type = 'module';
    drupal_alter('system_info', $modules[$key]->info, $modules[$key], $type);
  }

  if (isset($modules[$profile])) {
    // The installation profile is required, if it's a valid module.
    $modules[$profile]->info['required'] = TRUE;
    // Add a default distribution name if the profile did not provide one. This
    // matches the default value used in install_profile_info().
    if (!isset($modules[$profile]->info['distribution_name'])) {
      $modules[$profile]->info['distribution_name'] = 'Drupal';
    }
  }

  return $modules;
}

/**
 * Helper function to scan and collect theme .info data and their engines.
 *
 * Ripped & modified from system.module.
 *
 * @return
 *   An associative array of themes information.
 */
function _merge_rebuild_theme_data($profile_path) {
  // Find themes
  $themes = drupal_system_listing('/^' . DRUPAL_PHP_FUNCTION_PATTERN . '\.info$/', $profile_path);
  // Allow modules to add further themes.

  // Find theme engines
  $engines = drupal_system_listing('/^' . DRUPAL_PHP_FUNCTION_PATTERN . '\.engine$/', 'themes/engines');

  // Set defaults for theme info.
  $defaults = array(
    'engine' => 'phptemplate',
    'regions' => array(
      'sidebar_first' => 'Left sidebar',
      'sidebar_second' => 'Right sidebar',
      'content' => 'Content',
      'header' => 'Header',
      'footer' => 'Footer',
      'highlighted' => 'Highlighted',
      'help' => 'Help',
      'page_top' => 'Page top',
      'page_bottom' => 'Page bottom',
    ),
    'description' => '',
    'features' => _system_default_theme_features(),
    'screenshot' => 'screenshot.png',
    'php' => DRUPAL_MINIMUM_PHP,
    'stylesheets' => array(),
    'scripts' => array(),
  );

  $sub_themes = array();
  // Read info files for each theme
  foreach ($themes as $key => $theme) {
    $themes[$key]->filename = $theme->uri;
    $themes[$key]->info = drupal_parse_info_file($theme->uri) + $defaults;

    // Invoke hook_system_info_alter() to give installed modules a chance to
    // modify the data in the .info files if necessary.
    $type = 'theme';
    drupal_alter('system_info', $themes[$key]->info, $themes[$key], $type);

    if (!empty($themes[$key]->info['base theme'])) {
      $sub_themes[] = $key;
    }
    if ($themes[$key]->info['engine'] == 'theme') {
      $filename = dirname($themes[$key]->uri) . '/' . $themes[$key]->name . '.theme';
      if (file_exists($filename)) {
        $themes[$key]->owner = $filename;
        $themes[$key]->prefix = $key;
      }
    }
    else {
      $engine = $themes[$key]->info['engine'];
      if (isset($engines[$engine])) {
        $themes[$key]->owner = $engines[$engine]->uri;
        $themes[$key]->prefix = $engines[$engine]->name;
        $themes[$key]->template = TRUE;
      }
    }

    // Prefix stylesheets and scripts with module path.
    $path = dirname($theme->uri);
    $theme->info['stylesheets'] = _system_info_add_path($theme->info['stylesheets'], $path);
    $theme->info['scripts'] = _system_info_add_path($theme->info['scripts'], $path);

    // Give the screenshot proper path information.
    if (!empty($themes[$key]->info['screenshot'])) {
      $themes[$key]->info['screenshot'] = $path . '/' . $themes[$key]->info['screenshot'];
    }
  }

  // Now that we've established all our master themes, go back and fill in data
  // for subthemes.
  foreach ($sub_themes as $key) {
    $themes[$key]->base_themes = drupal_find_base_themes($themes, $key);
    // Don't proceed if there was a problem with the root base theme.
    if (!current($themes[$key]->base_themes)) {
      continue;
    }
    $base_key = key($themes[$key]->base_themes);
    foreach (array_keys($themes[$key]->base_themes) as $base_theme) {
      $themes[$base_theme]->sub_themes[$key] = $themes[$key]->info['name'];
    }
    // Copy the 'owner' and 'engine' over if the top level theme uses a theme
    // engine.
    if (isset($themes[$base_key]->owner)) {
      if (isset($themes[$base_key]->info['engine'])) {
        $themes[$key]->info['engine'] = $themes[$base_key]->info['engine'];
        $themes[$key]->owner = $themes[$base_key]->owner;
        $themes[$key]->prefix = $themes[$base_key]->prefix;
      }
      else {
        $themes[$key]->prefix = $key;
      }
    }
  }

  return $themes;
}

/**
 * Merge two arrays of projects (modules/themes)
 * keeping the versions that are the greatest, for most likely success
 * when merging multiple profiles (for the sake of dependencies).
 *
 * @param $existing_projects
 * @param $new_projects
 */
function _merge_project_merge(&$existing_projects, $new_projects) {
  if (count($existing_projects)) {
    foreach ($new_projects as $new_project_name => $new_project) {
      if (isset($existing_projects[$new_project_name])) {
        // Uh oh, this module is already required by another profile.

        $existing_project = $existing_projects[$new_project_name];
        $comparison = version_compare($existing_project->info->version, $new_project->info->version);
        $greatest_version = $new_project->info->version;
        if ($comparison !== 0) {

          // Both profiles use the same version... Neat.
        }
        elseif ($comparison == -1) {

          // The previous profile had a lower version than the new profile
          // Replace the version in the array with the new module.
          $existing_projects[$new_project_name] = $new_project;
        }
        elseif ($comparison == 1) {

          // The previous profile had a newer version than the new profile
          // Keep the previous (newer) version in the array.
          $greatest_version = $existing_project->info->version;
        }
        if ($comparison !== 0) {
          drush_log(
            dt(
              'Potential conflict. @project_name is used in more than one profile, but with differing versions. Using greatest version: @greatest_version',
              array(
                '@project_name' => $new_project_name,
                '@greatest_version' => $greatest_version
              )
            ),
            'warning');
        }
      }
      else {
        // The new project hasn't been crawled yet... Add it
        $existing_projects[$new_project_name] = $new_project;
      }
    }
  }
  else {
    $existing_projects = $new_projects;
  }
}

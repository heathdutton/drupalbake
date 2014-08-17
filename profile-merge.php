<?php
/**
 * Profile Merge
 *
 * Merge 2 or more Drupal profiles down to one.
 *
 * This is highly experimental!
 *
 * Usage:
 *     php profile-merge.php [-drupal-path="/path/to/drupal"] [-exclusive=0] [-name=new_profile] <profile1> <profile2> <profile3> ...
 *
 *
 * Assumptions:
 *     This can be a destructive process, the latest profile will take precedence,
 *     which means it can overwrite modules of a previous profile.
 *
 * To Do:
 *   * In the future have this detect modules, and gracefully swap out one
 *     version for another, instead of simply overwriting files which could be
 *     prone to errors if one of the copies has an extra file.
 *   * Convert this to a drush command, so that we can have access to drupal 
 *     bootstrap, and be more deterministic about profiles, etc.
 *     Would also make the code less stinky.
 *
 */

define('NL', "\r\n");

// Collect user arguments, and fall back to defaults (these may grow)
$defaults = array(
  'drupal-path::' => '',
  'exclusive::' => '1',
  'name::' => 'merged',
  'force::' => '0'
);

$option_args = array();
$options = getopt("", array_flip($defaults));
foreach ($defaults as $key => $value) {
  $key = str_replace(':', '', $key);
  if (!isset($options[$key])) {
    $options[$key] = $value;
  }
  else {
    $option_args[] = '--' . $key . '=' . $options[$key];
  }
}

// Determine the path to Drupal
if ($options['drupal-path']) {
  $working_dir = realpath($options['drupal-path']);
}
else {
  $working_dir = rtrim(getcwd(), '/');
}

// Check for Profiles directory and arguments before continuing.
$profiles_path = $working_dir . '/profiles';
if (file_exists($profiles_path)) {
  if (count($argv) > 2) {

    // Prep a list of all profiles in this Drupal instance
    $all_profiles = scandir($profiles_path);
    $all_profiles = array_diff($all_profiles, array('.', '..'));

    // Get an ordered list of requested profiles
    $requested_profiles = array_diff(array_slice($argv, 1), $option_args);

    // Make sure that the profiles the user wants to merge actually exist
    $diff = array_diff($requested_profiles, $all_profiles);
    if (!count($diff)) {

      // Prep the new merged folder
      $new_profile_path = $profiles_path . '/' . $options['name'];
      if (file_exists($new_profile_path) && $options['force']) {
        // We can forcibly unlink the existing folder
        echo 'Removing existing profile.' . NL;
        rmdirr($new_profile_path);
      }
      if (!file_exists($new_profile_path)) {
        if (mkdir($new_profile_path, 0700, TRUE)) {

          // Loop through the profiles and copy in most files destructively.
          $file_count = 0;
          $root_profile_files = array();
          foreach ($requested_profiles as $profile) {
            $profile_path = $profiles_path . '/' . $profile;
            // Recursively copy files, ignoring files at the root of the profile
            // And collect the root level files for later use
            $file_count += profile_copy($profile_path, $new_profile_path, $profile);
          }

          echo $file_count . ' files copied.' . NL;

          // Now for the fun bits...

          // Collect the top-level files by profile,
          // We need *.info, *.install, *.profile in an array
          die(print_r($root_profile_files));

          // Merge .info files

          // Merge .profile files

          // Merge .install files (most dubious)

          // Shove all OTHER files, into a nested "parents" folder.

          // Drop in a Readme explaining what we've done.
        }
        else {
          die('Could not create folder: ' . $new_profile_path . NL);
        }
      }
      else {
        die('Cannot merge! The folder already exists: ' . $new_profile_path . NL);
      }
    }
    else {
      die('Profile/s you specified do not appear to exist (' . implode(', ', $diff) . '). Make sure you use the profile folder names.' . NL);
    }
  }
  else {
    die('Please include the names of at least 2 profiles you want to merge.' . NL);
  }

}
else {
  die('Could not find profiles path. Make sure that you are running this from Drupal root.' . NL);
}

// Crappy function for recursively copying a folder, modded for this usecase
function profile_copy($source, $dest, $profile, $top_level = TRUE) {
  global $root_profile_files;
  $file_count = 0;
  if (is_dir($source)) {
    $dir_handle = opendir($source);
    while ($file = readdir($dir_handle)) {
      if ($file != "." && $file != "..") {
        if (is_dir($source . '/' . $file)) {
          if (!is_dir($dest . '/' . $file)) {
            mkdir($dest . '/' . $file, 0700);
          }
          $file_count += profile_copy($source . '/' . $file, $dest . '/' . $file, $profile, FALSE);
        }
        elseif (!$top_level) {
          copy($source . '/' . $file, $dest . '/' . $file);
          $file_count++;
        }
        elseif ($top_level) {
          $extension = pathinfo($file, PATHINFO_EXTENSION);
          if (!isset($root_profile_files[$extension])){
            $root_profile_files[$extension] = array();
          }
          $root_profile_files[$extension][] = $source . '/' . $file;
        }
      }
    }
    closedir($dir_handle);
  }
  // Do not copy files of the top level... we want to do that manually
  elseif (!$top_level) {
    copy($source, $dest);
    $file_count++;
  }
  elseif ($top_level){
    $extension = pathinfo($source, PATHINFO_EXTENSION);
    if (!isset($root_profile_files[$extension])){
      $root_profile_files[$extension] = array();
    }
    $root_profile_files[$extension][] = $source;
  }
  return $file_count;
}

// Recursively delete folders as needed.
function rmdirr($dirname) {
  // Sanity check
  if (!file_exists($dirname)) {
    return FALSE;
  }

  // Simple delete for a file
  if (is_file($dirname)) {
    return unlink($dirname);
  }

  // Loop through the folder
  $dir = dir($dirname);
  while (FALSE !== $entry = $dir->read()) {
    // Skip pointers
    if ($entry == '.' || $entry == '..') {
      continue;
    }

    // Recurse
    rmdirr($dirname . '/' . $entry);
  }

  // Clean up
  $dir->close();
  return rmdir($dirname);
}

// Ported directly from Drupal
function drupal_parse_info_format($data) {
  $info = array();
  $constants = get_defined_constants();

  if (preg_match_all('
    @^\s*                           # Start at the beginning of a line, ignoring leading whitespace
    ((?:
      [^=;\[\]]|                    # Key names cannot contain equal signs, semi-colons or square brackets,
      \[[^\[\]]*\]                  # unless they are balanced and not nested
    )+?)
    \s*=\s*                         # Key/value pairs are separated by equal signs (ignoring white-space)
    (?:
      ("(?:[^"]|(?<=\\\\)")*")|     # Double-quoted string, which may contain slash-escaped quotes/slashes
      (\'(?:[^\']|(?<=\\\\)\')*\')| # Single-quoted string, which may contain slash-escaped quotes/slashes
      ([^\r\n]*?)                   # Non-quoted string
    )\s*$                           # Stop at the next end of a line, ignoring trailing whitespace
    @msx', $data, $matches, PREG_SET_ORDER)) {
    foreach ($matches as $match) {
      // Fetch the key and value string.
      $i = 0;
      foreach (array('key', 'value1', 'value2', 'value3') as $var) {
        $$var = isset($match[++$i]) ? $match[$i] : '';
      }
      $value = stripslashes(substr($value1, 1, -1)) . stripslashes(substr($value2, 1, -1)) . $value3;

      // Parse array syntax.
      $keys = preg_split('/\]?\[/', rtrim($key, ']'));
      $last = array_pop($keys);
      $parent = &$info;

      // Create nested arrays.
      foreach ($keys as $key) {
        if ($key == '') {
          $key = count($parent);
        }
        if (!isset($parent[$key]) || !is_array($parent[$key])) {
          $parent[$key] = array();
        }
        $parent = &$parent[$key];
      }

      // Handle PHP constants.
      if (isset($constants[$value])) {
        $value = $constants[$value];
      }

      // Insert actual value.
      if ($last == '') {
        $last = count($parent);
      }
      $parent[$last] = $value;
    }
  }

  return $info;
}
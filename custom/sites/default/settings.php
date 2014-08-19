<?php
/**
 * Begin Drupal Bakery settings
 * ============================
 * The following are standard settings that are automatically appended
 * to the default settings.php files.
 */

/**
 * If we detect that we have been deployed to PagodaBox,
 * use the default database credentials given by that environment.
 */
if (getenv('ENVIRONMENT') == 'PAGODABOX'){
  $databases = array(
    'default' => array(
      'default' => array(
        'driver' => 'mysql',
        'database' => getenv('DATABASE1_NAME'),
        'username' => getenv('DATABASE1_USER'),
        'password' => getenv('DATABASE1_PASS'),
        'host' => getenv('DATABASE1_HOST'),
        'port' => getenv('DATABASE1_PORT'),
        'prefix' => ''
      )
    )
  );
}

/**
 * Composer Manager Settings.
 * This should be appended to every settings.php file
 */
$conf['composer_manager_vendor_dir'] = '../vendor';
$conf['composer_manager_file_dir'] = '../';

/**
 * End Drupal Bakery settings
 * ==========================
 */
<?php

/**
 * For Drupal Bakery, we occasionally need to extend drush.
 * Specify includes such that we can run custom commands
 * prior to Drupal bootstrap or installation.
 */
$options['include'] = 'sites/all/drush';
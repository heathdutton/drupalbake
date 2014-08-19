#!/usr/bin/env bash

echo ":::::::::::::::::::::::: DRUPAL BAKING START :::::::::::::::::::::::::"

# Test if PHP is installed
php -v > /dev/null 2>&1
PHP_IS_INSTALLED=$?

if [[ $PHP_IS_INSTALLED -ne 0 ]]; then
    echo "PHP is required to bake."
    exit 0;
fi

# Test if Composer is installed
composer -v > /dev/null 2>&1
COMPOSER_IS_INSTALLED=$?

if [[ $COMPOSER_IS_INSTALLED -ne 0 ]]; then
    echo "Composer is required to bake."
    exit 0;
fi

# Echo starting working dirrectory (for debugging deployments)
CWD=$(pwd)
printf "Working from directory: %s\n" ${CWD}

echo "Folder contents: "
cd vendor
ls

echo "Running Composer (used primarally for Drush)."
if [ -e "composer.lock" ]
then
  composer update --lock
fi
composer install --no-interaction --prefer-dist --no-dev

echo "Building the stub make file into a new drupal folder."
php vendor/bin/drush.php make bakery-stub.make drupal_new --force-complete --md5 --working-copy --prepare-install

echo "Dropping the auto-generated sites/default folder"
rm -rf drupal_new/sites/default

echo "Merging in custom code in /custom/drupal."
cp -R custom/ drupal_new/

echo "Moving static assets and settings to new Drupal."
mv drupal/sites/default drupal_new/sites/default

echo "Swapping old drupal for new."
mv drupal drupal_old
mv drupal_new drupal

echo "Cleaning up."
rm -rf drupal_old

echo "::::::::::::::::::::::::: DRUPAL BAKING END ::::::::::::::::::::::::::"

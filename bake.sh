#!/usr/bin/env bash

# Test if PHP is installed
php -v > /dev/null 2>&1
PHP_IS_INSTALLED=$?

if [[ $PHP_IS_INSTALLED -ne 0 ]]; then
    echo "PHP is required to build drupalbake."
    exit 0;
fi

# Test if Composer is installed
composer -v > /dev/null 2>&1
COMPOSER_IS_INSTALLED=$?

if [[ $COMPOSER_IS_INSTALLED -ne 0 ]]; then
    echo "Installing Composer (your password will be required)"
    curl -sS https://getcomposer.org/installer | php
    sudo mv composer.phar /usr/local/bin/composer
fi

echo "Running Composer (used primarally for Drush)."
composer install --no-interaction --prefer-source

echo "Merging in custom code in /custom/vendor."
cp -R custom/vendor/ vendor/

echo "Building the stub make file into a new drupal folder."
php vendor/bin/drush.php make drupalbake-stub.make drupal_new --force-complete --md5 --working-copy --prepare-install

echo "Merging in custom code in /custom/drupal."
cp -R custom/drupal/ drupal_new/

echo "Moving static assets and settings to new Drupal."
mv drupal/sites/default drupal_new/sites/default

echo "Swapping old drupal for new."
mv drupal drupal_old
mv drupal_new drupal

echo "Cleaning up."
rm -rf drupal_old

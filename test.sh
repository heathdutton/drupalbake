#!/usr/local/bin/php

echo ":::::::::::::::::::::::: DRUPAL BAKING START :::::::::::::::::::::::::"


echo "Building the stub make file into a new drupal folder."
cd vendor/bin
php -f drush.php info
#php -f vendor/bin/drush.php make bakery-stub.make drupal_new --force-complete --md5 --working-copy --prepare-install

php -i
echo "::::::::::::::::::::::::: DRUPAL BAKING END ::::::::::::::::::::::::::"

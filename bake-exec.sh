#!/usr/bin/env sh

echo ":::::::::::::::::::::::: DRUPAL BAKING START :::::::::::::::::::::::::"

# Echo starting working dirrectory (for debugging deployments)
CWD=$(pwd)
printf "Working from directory: %s\n" ${CWD}

if [ -e "composer.json" ]
then
    echo "Updating local composer."

    if [ -e "composer.lock" ]
    then
      composer update --lock
    fi
    composer install --no-interaction --prefer-dist --no-dev

fi

# Drush needs to be installed Globally, because it contains binaries
echo "Ensuring we have drush globally."
composer global require drush/drush:6.3.0

echo "Building the stub make file into a new drupal folder."
bash ~/.composer/vendor/bin/drush make bakery-stub.make drupal --force-complete --working-copy --prepare-install

echo "Merging in custom code in /custom/drupal."
cp -R custom/ drupal/

echo "::::::::::::::::::::::::: DRUPAL BAKING END ::::::::::::::::::::::::::"

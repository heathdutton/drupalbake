#!/usr/bin/env bash

echo "Refreshing build"
bash bake.sh

echo "Waiting for changes..."
daemon() {
    previouscheck=`find custom -type f -mtime -5s -exec md5 {} \;`
    while [[ true ]]
    do
        newcheck=`find custom -type f -mtime -5s -exec md5 {} \;`
        if [[ $newcheck != "" ]] ; then
            if [[ $previouscheck != $newcheck ]] ; then
                echo "Changes found: $newcheck"
                cp -R custom/vendor/ vendor/
                cp -R custom/drupal/ drupal/
            fi
        fi
        previouscheck=$newcheck
        sleep .250
    done
}

daemon$*
#!/usr/bin/env sh

echo "Waiting for changes..."
daemon() {
    previouscheck=`find custom -type f -mtime -5s -exec md5 {} \;`
    while [[ true ]]
    do
        newcheck=`find custom -type f -mtime -5s -exec md5 {} \;`
        if [[ $newcheck != "" ]] ; then
            if [[ $previouscheck != $newcheck ]] ; then
                echo "Changes found: $newcheck"
                cp -R custom/ drupal/
            fi
        fi
        previouscheck=$newcheck
        sleep .250
    done
}

daemon$*
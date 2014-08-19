#!/usr/bin/env sh

# Rebuild
bash bake.sh

# Remove existing first.
rm bake-dist.zip

echo "Creating a flat distribution of all files."
zip -r -9 -o -q bake-dist.zip ./ -x@bake-exclude.lst
echo "bake-dist.zip created."
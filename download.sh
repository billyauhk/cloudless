#!/bin/bash

echo "Usage: ./download.sh row col"
echo "Say, ./download.sh 12 32"

# Extract days which we have data
days=`GET http://lance-modis.eosdis.nasa.gov/imagery/subsets/RRGlobal_r${1}c${2}/ | \
grep -E "<a href=[^>]+>[0-9]+/</a>" | sed -E -e "s/<a href=[^>]+>([0-9]+)\/<\/a>.*/\1/g" | tr -d " "`
echo -n "We have "
echo $days | tr " " "\n" | wc -l | tr -d "\n"
echo " days with data."

# Data subset...as I do not have so much HDD space
days=`echo $days | tr " " "\n" | grep -E "^2012"`
echo -n "Subsetting: In 2012, we have "
echo $days | tr " " "\n" | wc -l | tr -d "\n"
echo " days with data."

# Looping and downloading both aqua and terra data
(echo $days | tr " " "\n" | xargs -n1 -Iday echo http://lance-modis.eosdis.nasa.gov/imagery/subsets/RRGlobal_r${1}c${2}/day/RRGlobal_r${1}c${2}.day.terra.2km.jpg
echo $days | tr " " "\n" | xargs -n1 -Iday echo http://lance-modis.eosdis.nasa.gov/imagery/subsets/RRGlobal_r${1}c${2}/day/RRGlobal_r${1}c${2}.day.aqua.2km.jpg) | \
xargs -n10 -P5 wget -q --continue

echo "Download finished."

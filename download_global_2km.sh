#!/bin/bash
echo "This is a fast script to retrieve 2km data from the world, 2012."

for row in {00..19};do
  for col in {00..39};do
    mkdir r${row}c${col}_2km;
    pushd r${row}c${col}_2km;
    ../download.sh $row $col;
    popd;
  done;
done


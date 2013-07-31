#!/bin/bash

for i in {00..19};do
  for j in {00..39};do
    if test -f "r${i}c${j}_2km.png";then
      echo "r${i}c${j} is done";
    else
      echo -n "Stitching...";date
      ./cloudless r${i}c${j}_2km 2012;
    fi
  done;
done

montage -tile 40x20 -geometry +0+0 \
`for i in {19..00};do for j in {00..39};
do echo -n "r${i}c${j}_2km.png ";done;done` world.png

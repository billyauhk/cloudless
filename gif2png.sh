#!/bin/bash

pushd $1
for i in `ls RRGlobal*.opaque.250m.gif`;do
  oldname=$i;
  newname=`echo $i | sed -e"s/gif/png/g"`;
  wait
  convert $oldname $newname &
done
wait
echo "Conversion done";
find -iname "RRGlobal*.opaque.250m.gif" -exec rm {} +
popd
echo "Exit"

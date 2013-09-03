#!/bin/bash

# Let us guarantee data is here first
for r in {12..16};do
  for c in {27..36};do
#    mkdir r${r}c${c}_250m
    pushd /media/unix_data/cloudless_data/r${r}c${c}_250m
    /home/bill/Desktop/cloudless/download.sh $r $c;
    popd
  done
done
#echo "All data downloaded."

exit
export OMP_NUM_THREADS=4
# Then we could use the stitcher
for r in {12..16};do
  for c in {27..36};do
    ./maker_250m.sh $r $c |& tee -a china_250m.log
  done
done
echo "All tile stitched"
montage -tile 5x10 -geometry +0+0 \
  `for r in {16..12};do for c in {27..36};do echo -n "r${r}c${c}_250m_all.png ";done;done` china_250m.png
convert china_250m.png -normalize china_250m_normalized.png

#!/bin/bash

index=0
for y in `seq 0 1024 4095`;do
  for x in `seq 0 1024 4095`;do
    echo -n "Start working on tile "$index" ";date
    (OMP_NUM_THREADS=4
     ./cloudless r${1}c${2}_250m $x $y 1024 1024 $index 2012 &>/dev/null;
     echo -n "Stop working on tile "$index" ";date)
    index=$(($index + 1));
  done
done
echo "All tiles done"

all=`seq 0 1 $(($index - 1))`

montage -tile 4x4 -geometry +0+0 \
  `for i in $all;do echo -n "r"${1}"c"${2}"_250m_"$i".png ";done` \
  r${1}c${2}_250m_all.png
#convert r${1}c${2}_250m_all.png -normalize r13c33_250m_all_normalized.png
rm `for i in $all;do echo -n "r"${1}"c"${2}"_250m_"$i".png ";done`
echo -n "All finished";date

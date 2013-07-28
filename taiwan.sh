#!/bin/bash

# In this example shell-script, we are going to generate image of Taiwan
# It is easy as it only involves the tile r12c33
# And the corners are (x,y) = (720, 1280) and (2512,2560)

index=0
for y in `seq 720 256 2355`;do
  for x in `seq 1280 256 2359`;do
    echo -n "Start working on tile "$index" ";date
    (./cloudless r12c33_250m $x $y 256 256 $index 2012 &>/dev/null;
     echo -n "Stop working on tile "$index" ";date) &
    index=$(($index + 1));
    if test $index -eq 1;then
      sleep 60;
    elif test $index -lt 35;then
      sleep 100;
    fi
  done
done
echo "Waiting for them finish"
wait
echo "All tiles done"

all=`seq 0 1 $(($index - 1))`
exit
montage -tile 5x7 -geometry +0+0 \
  `for i in $all;do echo -n "r12c33_250m_"$i".png ";done` \
  taiwan.png
convert taiwan.png -normalize taiwan_normalized.png
#rm r12c33_2km_*.png
echo "All finished"

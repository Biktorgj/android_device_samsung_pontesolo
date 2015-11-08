#!/bin/ksh
FILE=/data/swapdir/swapfile
if [[ ! -f $FILE ]];
then
mkdir /data/swapdir && dd if=/dev/zero of=/data/swapdir/swapfile bs=1m count=512 && mkswap /data/swapdir/swapfile
else
swapon /data/swapdir/swapfile && sysctl -w vm.swappiness=10
fi

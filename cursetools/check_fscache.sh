#!/bin/sh

echo "I am ready!"
read c

if ! [ -e /big0 ]; then
	echo "creating big0..."
	dd if=/dev/zero of=/big0 bs=1024k count=100
	echo " "
fi

if ! [ -e /big1 ]; then
	echo "creating big1..."
	dd if=/dev/zero of=/big1 bs=1024k count=100
	echo " "
fi

./uncache /big0
./uncache /big1

echo "1st read..."
dd if=/big0 of=/dev/null bs=512k
echo " "

echo "2st read..."
dd if=/big0 of=/dev/null bs=512k
echo " "

echo "disrupt and press enter..."
read c

sleep 1

echo "3rd read..."
dd if=/big0 of=/dev/null bs=512k
echo " "


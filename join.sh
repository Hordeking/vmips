#!/bin/sh
# Just a dumb little program to postprocess the output
# of objdump -i.
read junk
while true
do
read targ
if test "x$targ" = "x"
then
	exit 0
fi
read fmt
read arch
if test "x$arch" = "x"
then
	arch=unknown
	echo "$targ $fmt $arch"
	exit 0
fi
echo "$targ $fmt $arch"
done
exit 0

#!/bin/sh
# Just a dumb little program to postprocess the output
# of objdump -i.
read junk
while true
do
read targ
if test x$targ = x
then
	exit 0
fi
read fmt
read arch
echo "$targ $fmt $arch"
done
exit 0

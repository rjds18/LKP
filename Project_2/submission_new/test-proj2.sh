#!/bin/sh

sudo insmod proj2.ko int_str=1,2,3,4,5
cat /proc/proj2
rmmod proj2
#dmesg
#dmesg -c > cleaned


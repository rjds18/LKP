#!/bin/sh

sudo insmod proj4.ko
#cat /var/log/syslog | tail -2
cat /proc/proj4
rmmod proj4
dmesg
dmesg -c > cleaned


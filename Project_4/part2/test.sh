#!/bin/sh

mkdir mnt
sudo insmod s2fs.ko
#cat /var/log/syslog | tail -2
sudo mount -t s2fs nodev mnt
mount
sudo umount ./mnt
sudo rmmod s2fs.ko

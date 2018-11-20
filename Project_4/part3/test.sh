#!/bin/sh

sudo insmod s2fs.ko
#cat /var/log/syslog | tail -2
sudo mount -t s2fs nodev mnt
mount
#cd ./mnt/foo
#ls
#cat bar

#sudo umount mnt
#sudo rmmod s2fs.ko

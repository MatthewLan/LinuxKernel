
### root shell
    >> su root

### build & clean
    >> make
    >> make clean

### insmod & rmmod
    >> insmod globalmem.ko
    >> rmmod globalmem

### check module
    >> cat /proc/devices
    Character devices:
    ...
    238 globalmem
    ...

### mknod & rm
    >> mknod /dev/globalmem c 238 0
    >> ll /dev/globalmem
    crw-r--r--. 1 root root 238, 0 Jan  6 20:13 /dev/globalmem
    >> rm /dev/globalmem

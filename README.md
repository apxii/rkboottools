rkboottools
===========

Quick and dirty Rockchip 3188 boot tools.

rk-splitboot
------------
    Splits and decodes Rockchip bootloader
    Usage: rk-splitboot <RKboot.bin>

rk-makebootable
---------------
    Makes Rockchip SD Card bootable image
    Usage: rk-makebootable <Stage1> <Stage2> <Output>

    Stage1 is a "FlashData" part of official RK Bootloader, it gets loaded by bootrom at 0x10080800 and sets up DRAM controller.

    Stage2 is a "FlashBoot" part of official RK Bootloader, or it can be a Barebox or U-Boot binary. It gets loaded at 0x60000000 by bootrom.

    Output file can be written to SD card by
    dd if=<Output> of=</dev/sdcard> bs=$((0x200) seek=$((0x40))


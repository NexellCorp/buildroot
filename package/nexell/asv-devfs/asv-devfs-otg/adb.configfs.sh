#!/bin/sh
# Mount ConfigFS and create Gadget

mount -t configfs none /sys/kernel/config
mkdir -p /sys/kernel/config/usb_gadget/g1
cd /sys/kernel/config/usb_gadget/g1

echo 0x18d1 > idVendor
echo 0x0001 > idProduct

mkdir -p strings/0x409

hostname > strings/0x409/serialnumber;
echo "Nexell" > strings/0x409/manufacturer;
hostname > strings/0x409/product

mkdir -p configs/c.1
mkdir -p configs/c.1/strings/0x409
echo "USB ADB" > configs/c.1/strings/0x409/configuration

#echo 120 > configs/c.1/MaxPower

# stop adbd
killall adbd

mkdir functions/ffs.adb
ln -s functions/ffs.adb configs/c.1/ffs.adb

# Create Adb FunctionFS function
mkdir -p /dev/usb-ffs/adb
mount -o uid=2000,gid=2000 -t functionfs adb /dev/usb-ffs/adb

#Start adbd application
/usr/bin/adbd &
sleep 0.2

#Enable the Gadget
ls /sys/class/udc/ > UDC
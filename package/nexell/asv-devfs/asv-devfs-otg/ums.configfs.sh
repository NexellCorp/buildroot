#! /bin/sh
#
# This script support usb gadget mass storage function driver.
#
#  #> ums.configfs.sh img
#	- create image with dd and use image to massstorage
#
#  #> ums.configfs.sh <device node:/dev/mmcblk0p1>
#

ID_VENDOR=0x18d1
ID_PRODUCT=0x0003

MANUFACT=`hostname`
PRODUCT=Gadget
SERIAL=`hostname`

USB_GADGET=/sys/kernel/config/usb_gadget
GADGET_NAME=g
GADGET_DIR=$USB_GADGET/$GADGET_NAME
INSTANCE_NAME=mass_storage.0
LUN_IMAGE=/tmp/lun0.img
LUN_BS_COUNT=16

function usage() {
	echo "Usage : $0 {lun|dev/sdX} [bs(1M) count]"
	echo "lun      = create logical volume and launch lun for ums"
	echo "         = create lun image 1Mx$LUN_BS_COUNT to $LUN_IMAGE"
	echo "/dev/sdX = launch physical volume with input device"
	exit 1
}

case "$1" in
	lun)
		if [ $# -eq 2 ]; then
			LUN_BS_COUNT=$2
		fi

		luni=$LUN_IMAGE
		if [ ! -e "$luni" ]; then
			echo "create lun image 1Mx$LUN_BS_COUNT to $luni"
			dd bs=1M count=$LUN_BS_COUNT if=/dev/zero of=$luni
			mkdosfs $luni
		fi
		STORAGE=$luni
		;;
	help|h|-h|-help)
		usage; exit 1;;
	*)
		STORAGE=$1
		;;
esac

if [ ! -e "$STORAGE" ]; then
	echo "No such file or directory $STORAGE"
	echo ""
	usage; exit 1;
fi

echo "path         : $GADGET_DIR"
echo "gadget       : $GADGET_NAME"
echo "id           : $ID_VENDOR, $ID_PRODUCT"
echo "manufacturer : $MANUFACT"
echo "product      : $PRODUCT"
echo "serial       : $SERIAL"

if ! mount | grep /sys/kernel/config > /dev/null; then
        mount -t configfs none /sys/kernel/config
fi

mkdir -p $GADGET_DIR
mkdir -p $GADGET_DIR/strings/0x409
mkdir -p $GADGET_DIR/configs/c.1
mkdir -p $GADGET_DIR/configs/c.1/strings/0x409

echo $ID_VENDOR  > $GADGET_DIR/idVendor
echo $ID_PRODUCT > $GADGET_DIR/idProduct
echo $MANUFACT   > $GADGET_DIR/strings/0x409/manufacturer
echo $PRODUCT    > $GADGET_DIR/strings/0x409/product
echo $SERIAL     > $GADGET_DIR/strings/0x409/serialnumber;

echo "function     : $INSTANCE_NAME"
mkdir -p $GADGET_DIR/functions/$INSTANCE_NAME

echo $STORAGE > $GADGET_DIR/functions/$INSTANCE_NAME/lun.0/file

if [ ! -e "$GADGET_DIR/configs/c.1/$INSTANCE_NAME" ]; then
	ln -s $GADGET_DIR/functions/$INSTANCE_NAME $GADGET_DIR/configs/c.1;
fi

echo "Bring up usb function '$GADGET_DIR/UDC' ..."

#
# Bring up USB (RESET)
# echo c0040000.dwc2otg > $GADGET_DIR/UDC
#
ls /sys/class/udc/ > $GADGET_DIR/UDC

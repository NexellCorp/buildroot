#!/bin/bash

set -u
set -e

SKELETON_FS_DIR=board/nexell/fs

if grep -Eq "^BR2_PACKAGE_ANDROID_TOOLS_ADBD=y$" ${BR2_CONFIG}; then
	install -m 0755 $SKELETON_FS_DIR/usr/bin/start_adbd.sh ${TARGET_DIR}/usr/bin/start_adbd.sh
	install -m 0755 $SKELETON_FS_DIR/usr/bin/stop_adbd.sh ${TARGET_DIR}/usr/bin/stop_adbd.sh
	install -m 0755 $SKELETON_FS_DIR/etc/init.d/adbd.sh ${TARGET_DIR}/etc/init.d/adbd.sh

	if grep -Eq "^BR2_INIT_SYSTEMD=y$" ${BR2_CONFIG}; then
		for i in 0 1 2 3 4 5 6
		do
			mkdir -p ${TARGET_DIR}/etc/rc$i.d
			link=S01adbd; [ $i -lt 2 ] && link=K01adbd;
			ln -sf /etc/init.d/adbd.sh ${TARGET_DIR}/etc/rc$i.d/$link
		done
	fi
fi

if grep -Eq "^BR2_PACKAGE_TSLIB=y$" ${BR2_CONFIG}; then
	install $SKELETON_FS_DIR/etc/profile ${TARGET_DIR}/etc/profile
fi

if grep -Eq "^BR2_PACKAGE_DIRECTFB=y$" ${BR2_CONFIG}; then
	install $SKELETON_FS_DIR/etc/directfbrc ${TARGET_DIR}/etc/directfbrc
fi

if grep -Eq "^BR2_PACKAGE_MEMTESTER=y$" ${BR2_CONFIG}; then
	install -m 0755 $SKELETON_FS_DIR/usr/bin/memtest.sh ${TARGET_DIR}/usr/bin/memtest.sh
fi

if grep -Eq "^BR2_PACKAGE_LMBENCH=y$" ${BR2_CONFIG}; then
	install -m 0755 $SKELETON_FS_DIR/usr/bin/bw_mem.sh ${TARGET_DIR}/usr/bin/bw_mem.sh
fi

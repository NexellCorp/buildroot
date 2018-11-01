#!/bin/bash

set -u
set -e

SKELETON_FS_DIR=board/nexell/fs

if [ -e ${TARGET_DIR}/etc/inittab ]; then
	if ! grep -Eq "^BR2_INIT_SYSTEMD=y$" ${BR2_CONFIG}; then
		# copy to nologin
		cp $SKELETON_FS_DIR/etc/inittab ${TARGET_DIR}/etc/inittab
	fi
fi

if grep -Eq "^BR2_TARGET_ROOTFS_CPIO=y$" ${BR2_CONFIG}; then
        if [ ! -e ${TARGET_DIR}/init ]; then
                install -m 0755 fs/cpio/init ${TARGET_DIR}/init;
        fi

	cp -a $SKELETON_FS_DIR/dev/console ${TARGET_DIR}/dev/console
fi

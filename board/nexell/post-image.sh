#!/bin/bash

set -u
set -e

SKELETON_FS_DIR=board/nexell/fs

if grep -Eq "^BR2_PACKAGE_ANDROID_TOOLS_ADBD=y$" ${BR2_CONFIG}; then
	cp $SKELETON_FS_DIR/usr/bin/start_adbd.sh ${TARGET_DIR}/usr/bin
	cp $SKELETON_FS_DIR/usr/bin/stop_adbd.sh ${TARGET_DIR}/usr/bin
	chmod 755 ${TARGET_DIR}/usr/bin/start_adbd.sh
	chmod 755 ${TARGET_DIR}/usr/bin/stop_adbd.sh
fi

if grep -Eq "^BR2_PACKAGE_TSLIB=y$" ${BR2_CONFIG}; then
	cp $SKELETON_FS_DIR/etc/profile ${TARGET_DIR}/etc/profile
fi

#!/bin/bash

set -u
set -e

SKELETON_FS_DIR=board/nexell/fs

if grep -Eq "^BR2_PACKAGE_ANDROID_TOOLS_ADBD=y$" ${BR2_CONFIG}; then
	cp $SKELETON_FS_DIR/usr/bin/start_adb.sh ${TARGET_DIR}/usr/bin
fi

if grep -Eq "^BR2_PACKAGE_TSLIB=y$" ${BR2_CONFIG}; then
	cp $SKELETON_FS_DIR/etc/profile ${TARGET_DIR}/etc/profile
fi

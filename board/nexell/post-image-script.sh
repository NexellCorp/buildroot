#!/bin/bash

source board/nexell/post-image.sh

if [ -e ${TARGET_DIR}/etc/inittab ]; then
	if ! grep -Eq "^BR2_INIT_SYSTEMD=y$" ${BR2_CONFIG}; then
		# set autologin
		cp ${SKELETON_FS_DIR}/etc/inittab ${TARGET_DIR}/etc/inittab
	fi
fi

if grep -Eq "^BR2_TARGET_ROOTFS_CPIO=y$" ${BR2_CONFIG}; then
        if [ ! -e ${TARGET_DIR}/init ]; then
                install -m 0755 fs/cpio/init ${TARGET_DIR}/init;
        fi

	cp -a ${SKELETON_FS_DIR}/dev/console ${TARGET_DIR}/dev/console
fi

if grep -Eq "^BR2_INIT_SYSTEMD=y$" ${BR2_CONFIG}; then
	# autologin service
	cp ${SKELETON_FS_DIR}/lib/systemd/system/serial-getty@.service \
	   ${TARGET_DIR}/lib/systemd/system/serial-getty@.service

	ln -sf /lib/systemd/system/serial-getty@.service \
	     ${TARGET_DIR}/etc/systemd/system/getty.target.wants/serial-getty@ttyS2.service

	# remove prompt login services
	rm -f ${TARGET_DIR}/lib/systemd/system/console-getty.service
	rm -f ${TARGET_DIR}/usr/lib/systemd/system/container-getty@.service
fi

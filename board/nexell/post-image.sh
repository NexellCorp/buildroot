#!/bin/bash

set -u
set -e

function fix_permissions() {
    local files=`ls output/target/bin/* | xargs stat --printf "$(pwd)/%n %A \n" | grep [-]rws`
    local n=0

    for i in $files
    do
	if [ $(( $n % 2 )) -eq 0 ]; then
	    echo "change permission $i"
	    chmod 755 $i
	fi
	n=$((n + 1))
    done
}

if [ -e ${TARGET_DIR}/etc/inittab ]; then
    if ! grep -Eq "^BR2_INIT_SYSTEMD=y$" ${BR2_CONFIG}; then

	# set autologin
	cp board/nexell/skeleton/etc/inittab ${TARGET_DIR}/etc/inittab

    	# change permission
	fix_permissions
    fi
fi

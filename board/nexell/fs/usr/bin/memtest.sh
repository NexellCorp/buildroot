#!/bin/sh

REPEAT=1
MEMSIZE=20M # MegaBytes
REBOOT=false

function usage() {
	echo "usage: `basename $0` [-l byte [K|M|G]][-c repeat][-r reboot] "
	echo ""
}

while getopts 'hl:c:r' opt
do
        case $opt in
        l )
		MEMSIZE=$OPTARG
		;;
        c )
		REPEAT=$OPTARG
		;;
        r )
		REBOOT=true
		;;
        h | *)
        	usage
		exit 1;;
		esac
done

PARALL=`grep processor /proc/cpuinfo | wc -l`

cnt=0
while [ $cnt -lt $REPEAT ]
do
	temp=`cat /sys/class/thermal/thermal_zone0/temp`
	cnt=$(($cnt + 1))

	echo  -e "\033[0;33m ================================================= \033[0m"
	echo  -e "\033[0;33m [$REPEAT:$cnt] Temperature: $temp \033[0m"
	echo  -e "\033[0;33m ================================================= \033[0m"

	PID="" cpu=0
	while [ $cpu -lt $PARALL ]
	do
		memtester $MEMSIZE 1 &
		PID=$PID" "$!
		cpu=$(($cpu + 1))
	done;

	for pid in $PID;
	do
		wait $pid
		if [ $? -ne 0 ]; then
			echo  -e "\033[0;31m ================================================= \033[0m"
			echo  -e "\033[0;31m FAILED PID.$pid \033[0m"
			echo  -e "\033[0;31m ================================================= \033[0m"
			exit 1;
		fi
	done
done

echo  -e "\033[0;33m ================================================= \033[0m"
echo  -e "\033[0;33m OK REBOOT \033[0m"
echo  -e "\033[0;33m ================================================= \033[0m"

# force reboot command
[ $REBOOT == true ] && reboot -f;


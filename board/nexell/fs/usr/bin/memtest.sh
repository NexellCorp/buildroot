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
PID=""

CNT=0
while [ $CNT -lt $PARALL ]
do
	memtester $MEMSIZE $REPEAT &
   	PID=$PID" "$!
	CNT=$(($CNT + 1))
done;

for i in $PID;
do
	wait $i
	if [ $? -ne 0 ]; then
		echo  -e "\033[0;31m ================================================= \033[0m"
		echo  -e "\033[0;31m FAILED PID.$i \033[0m"
		echo  -e "\033[0;31m ================================================= \033[0m"
		exit 1;
	fi
done

echo  -e "\033[0;33m ================================================= \033[0m"
echo  -e "\033[0;33m OK REBOOT \033[0m"
echo  -e "\033[0;33m ================================================= \033[0m"

# force reboot command
[ $REBOOT == true ] && reboot -f;


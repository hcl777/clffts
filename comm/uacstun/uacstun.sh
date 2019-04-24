#!/bin/sh

F1="uacstun.e"
DIR="$( cd "$( dirname "$0"  )" && pwd  )"
NAME=$(basename $0)
RCPATH="/etc/rc.d/rc.local"
BOOTCMD="$DIR/$NAME start &"
start()
{
	#single run 
	{
	flock -n 10
	[ $? -eq 1 ] && exit
	ulimit -c 409600
	for j in $(seq 0 100)
	do
   		echo $j > coredump.txt
    		$DIR/$F1 > /dev/null
    		sleep 20
	done
	} 10<> $DIR/.run
}
stop()
{
	echo "stop!"
	kill -9 `ps x|grep $NAME |grep -v $$ |grep -v grep  | awk '{print $1}'`
	killall -9 $F1
}
boot_on()
{
	cmd=`cat $RCPATH | grep "$BOOTCMD"`
	if [ -z "$cmd" ]
	then
		echo $BOOTCMD >> $RCPATH && echo "boot_on ok!"
	else
		echo "***boot_on exist!"
	fi
}
boot_off()
{
	num=`grep -n "$BOOTCMD" $RCPATH | head -1 | cut -d ":" -f 1`
	if [ -z "$num" ]
	then
		echo "***no boot_on cmd!***"
	else
		#echo $num	
		sed -i "${num}d"  $RCPATH  && echo "boot_off ok!"
	fi
}

#echo " $DIR/$NAME ($$)"

case "$1" in
boot_on)
	boot_on
	;;
boot_off)
	boot_off
	;;
start)
	start
	;;
stop)
	stop
	;;
*)
	echo $"Usage:sh $0 {start|stop|boot_on|boot_off}"
esac

exit 0

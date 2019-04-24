#!/bin/sh

RETVAL=0
MYPATH="/opt/clyun/cly_down/"
MYFILE1="cly_down"
MYFILE2="cly_down.xml"
MYFILE3="clyd.sh"
MYFILE4="cly_dconf.ini"
BOOTCMD="$MYPATH$MYFILE3 start"
RCPATH="/etc/rc.d/rc.local"

setup()
{
	#setup vllscpd to $MYPATH
	echo "#Setup $MYPATH$MYFILE1..."
	mkdir -p $MYPATH && echo "#mkdir $APPPATH ok" || exit
	cp $MYFILE1 $MYPATH && echo "#cp $MYFILE1 ok" || exit
	cp $MYFILE2 $MYPATH && echo "#cp $MYFILE2 ok" || echo "*** no $MYFILE2"
	cp $MYFILE3 $MYPATH && echo "#cp $MYFILE3 ok" || echo "*** no $MYFILE3"
	cp $MYFILE4 $MYPATH && echo "#cp $MYFILE4 ok" || echo "*** no $MYFILE4"
	chmod +x $MYPATH$MYFILE1 && echo "#add execute permission to $MYFILE1 ok" || exit
	chmod +x $MYPATH$MYFILE3 && echo "#add execute permission to $MYFILE3 ok" || exit
	ln -sf $MYPATH$MYFILE3 /sbin && echo "#ln /sbin/$MYFILE3 ok" || exit
	echo "#Setup done!"
}
unsetup()
{
	#可能会把自己同名脚本杀死
	#stop 
	sleep 1
	unlink /sbin/$MYFILE3
	rm -rf $MYPATH && echo "unsetup ok!"
}

boot_on()
{
	#add cmdline to "/etc/rc.d/rc.local" file tail
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
	#delete cmdline from "/etc/rc.d/rc.local" file
	num=`grep -n "$BOOTCMD" $RCPATH | head -1 | cut -d ":" -f 1`
	if [ -z "$num" ]
	then
		echo "***no boot_on cmd!***"
	else
		#echo $num	
		sed -i "${num}d"  $RCPATH  && echo "boot_off ok!"
	fi
}

start()
{
	#如果需要用其它账号启动,请修改以下命令
	su -c "$MYPATH$MYFILE1 &" root
	RETVAL=$?
	[ $RETVAL -eq 0 ] && echo "$MYFILE1 start ok!" || echo "*** $MYFILE1 start fail! ***"
	return $RETVAL
}
stop()
{
	#echo -n $"Stopping $MYFILE1..."
	kill -9 `ps -A|grep $MYFILE1 |awk '{print $1}'`
	RETVAL=$?
	[ $RETVAL -eq 0 ] && echo "stop ok!"
	return $RETVAL
}

case "$1" in
setup)
	setup
	;;
unsetup)
	unsetup
	;;
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
	echo $"Usage:sh $0 {setup|unsetup|boot_on|boot_off|start|stop}"
esac

exit $RETVAL

ifc="$ARGS_IFACE"
addr="$ARGS_ADDRESS"
if test ! -z "$addr" ; then
	/sbin/ifconfig $ifc $addr 2>&1 >/dev/null ; fi
addr=`/sbin/ifconfig $ifc | grep "inet addr:"`
addr=`echo $addr | cut -d\  -f2 | cut -d: -f2`
echo "$PORT_TSESSION result $addr"



 


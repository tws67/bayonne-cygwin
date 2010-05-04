seconds=`cat /proc/uptime | cut -d\  -f1 | cut -d. -f1`
echo "$PORT_TSESSION result $seconds"



fs="$ARGS_FILESYSTEM"
df=`df $fs | tail -n 1`
df=`echo $df | cut -d\  -f5`
echo "$PORT_TSESSION result $df"



 


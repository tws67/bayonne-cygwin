file="$ARGS_PATH"

if test ! -f ${file} ; then
	echo "$PORT_TSESSION error source-missing"
	exit 0
fi

exec audiotool -strip -framing=10 $file


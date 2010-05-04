file="$ARGS_PATH"
pad="$ARGS_PADDING"

if test ! -f ${file} ; then
	echo "$PORT_TSESSION error source-missing"
	exit 0
fi

exec audiotool -trim -padding=$pad -framing=10 $file


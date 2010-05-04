file="$ARGS_PATH"
text="$ARGS_TEXT"

if test ! -f ${file} ; then
	echo "$PORT_TSESSION error source-missing"
	exit 0
fi

exec audiotool -notation $file ${text}



source="$ARGS_PATH"
index="$ARGS_INDEX"
dir=`dirname $source`
ext=""
case "$source" in
*.au)
	ext="au"
	;;
*.gsm)
	ext="gsm"
	;;
*.wav)
	ext="wav"
	;;
esac

if test -z "$ext" ; then
	target=${source}.ogg
else
	target=`echo $source | sed -e "s/$ext\$/ogg/"`
fi

if test ! -f ${source} ; then
	echo "$PORT_TSESSION error source-missing"
	exit 0
fi

rm -f $tmp
sox $source $tmp
if test $? -gt 0 ; then
	echo "$PORT_TSESSION error convert-failed"
	exit 0
fi

mv -f $tmp $target



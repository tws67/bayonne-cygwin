sender="$SMTP_SENDER"
sendmail="$SMTP_SENDMAIL"
errors="$SMTP_ERRORS"
body="$ARGS_TMP"
to="$ARGS_TO"
cc="$ARGS_CC"
reply="$ARGS_REPLY"
dest="$to"
from="$ARGS_FROM"
subject="$ARGS_SUBJECT"
file="$ARGS_PATH"
mime=""

if test ! -z "$file" ; then
	if test ! -f ${file} ; then
		echo "$PORT_TSESSION error file-missing"
		exit 0
	fi
	case ${file} in
	*.au|*.au)
		mime="audio/basic"
		;;
	*.txt|*.text|*.log)
		mime="text/plain"
		;;
	*.wav)
		mime="audio/x-wav"
		;;
	*.mp3)
		mime="audio/mpeg"
		;;
	*.gif)
		mime="image/gif"
		;;
	*.tif|*.tiff)
		mime="image/tiff"
		;;
	*.jpg|*.jpeg)
		mime="image/jpeg"
		;;
	*.htm|*.html)
		mime="text/html"
		;;
	*.xml)
		mime="application/xml"
		;;
	*.png)
		mime="image/png"
		;;
	*.xpm)
		mime="image/xpm"
		;;
	esac
fi		

if test ! -z "$cc" ; then
	$dest="$dest $cc" ; fi

if test ! -x ${sendmail} ; then
	rm -f $tmp
	echo "$PORT_TSESSION error sendmail-missing"
	exit 0
fi

btsencode -e "$errors" -f "$from" -t "$to" -c "$cc" \
	-s "$subject" -b "$body" -m "$mime" -a "$file" | \
		${sendmail} -f $sender $dest






agent="$URL_AGENT"
proxy="$URL_PROXY"
refer="$URL_PREFIX"
userid="$URL_USERID"
secret="$URL_SECRET"
cookies="$ARGS_SESSION_COOKIES"
convert="$ARGS_CONVERT"
rate="ARGS_RATE"
file="$ARGS_PATH"
url="$ARGS_URL"
query="$ARGS_QUERY"
encoding="$ARGS_ENCODING"
format=""
rate="-r8000"

if test -f "$cookies" ; then
	load="--load-cookies $cookies" ; fi

save="--save-cookies $cookies"

if test ! -z "$convert" ; then
	case ${convert} in
	*.au|*.snd)
		format="-U"
		;;
	*.wav|*.raw)
		format="-sw"
		;;
	*.vox)
		rate=""
		encoding=""
		;;
	*.gsm|*.ul|*.al|*.sw|*.uw)
		encoding=""
		;;
	esac
fi

if test ! -z "$encoding" ; then
	case ${encoding} in
	*ulaw)
		format="-U"
		;;
	alaw)
		format="-A"
		;;
	gsm)
		format="-tgsm"
		;;
	esac
fi

if test ! -z "$refer" ; then
	refer="--referer=$refer" ; fi

if test ! -z "$agent" ; then
	agent="--user-agent=$agent" ; fi

if test ! -z "$proxy" ; then
	export http_proxy="$proxy"
	export ftp_proxy="$proxy"
	if test ! -z "$userid" ; then
		proxy_userid="--proxy-user=$userid" ; fi
	if test ! -z "$secret" ; then
		proxy_secret="--proxy-secret=$secret" ; fi
fi

rm -f ${file}
if test ! -z "$convert" ; then
	rm -f ${convert} ; fi

if test ! -z "$query" ; then
	url=${url}${query} ; fi

wget $load $save -q --passive-ftp -O ${file} \
	"$agent" "$refer" $userid $secret ${url}

if test "$?" -gt 0 ; then
	rm -f ${file}
	echo "$PORT_TSESSION error url-failed" 
	exit 0
fi

if test ! -f ${file} ; then
	echo "$PORT_TSESSION error url-invalid"
	exit 0
fi

if test -z "$convert" ; then
	exit 0 ; fi

sox ${file} $rate $format -c1 ${convert}
rm -f ${file}
if test "$?" -gt 0 ; then
	rm -f ${convert}
	echo "$PORT_TSESSION error convert-failed" ; fi

exit 0
 

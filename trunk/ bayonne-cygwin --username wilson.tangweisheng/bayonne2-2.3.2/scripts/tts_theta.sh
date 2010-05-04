file="$ARGS_PATH"
voice="$ARGS_VOICE"
text="$ARGS_TEXT"
encoding="$file-$HEAD_ENCODING"
theta="/opt/theta/bin/theta"

rm -f $file
case "$voice" in
male)
	voice="-N Frank"
	;;
female)
	voice="-N Emily"
	;;
*)
	voice="-N $voice"
	;;
esac
case "$encoding" in
*.raw-linear|*.wav-linear)
	$theta -F 8000 -o $file $voice "'$text  '"
	;;
*)
	$theta -F 8000 -o $file.wav $voice "'$text  '"
	audiotool --build -encoding="$HEAD_ENCODING" $file $file.wav
	rm -f $file.wav
	;;
esac
	

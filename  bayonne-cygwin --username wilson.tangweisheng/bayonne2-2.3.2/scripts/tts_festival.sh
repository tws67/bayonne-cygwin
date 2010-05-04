file="$ARGS_PATH"
voice="$ARGS_VOICE"
text="$ARGS_TEXT"
encoding="$file-$HEAD_ENCODING"
script=/usr/bin/text2wave
festival=/usr/bin/festival

rm -f $file
case "$voice" in
male)
	voice="english"
	;;
female)
	voice="english"
	;;
*)
	voice="$voice"
	;;
esac
case "$encoding" in
*.raw-linear|*.wav-linear)
	echo $text | $festival --script $script -o $file -F 8000
	;;
*)
	echo $text | $festival --script $script -o $file.wav -F 8000
	audiotool --build -encoding="$HEAD_ENCODING" $file $file.wav
	rm -f $file.wav
	;;
esac
	

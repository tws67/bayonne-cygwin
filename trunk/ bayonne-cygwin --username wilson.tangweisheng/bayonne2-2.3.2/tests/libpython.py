#!/usr/bin/python

from libexec import Libexec

if __name__ == '__main__':
	tgi = Libexec()
	tgi.echo("LIBEXEC STARTED")
	result = tgi.speak("&number 123")
	tgi.echo("PROMPT DONE, RESULT=%d" % result)
	tgi.result(result)

	file1 = tgi.pathname("tmp:edit")
	file2 = tgi.pathname("temp/edit")
	tgi.echo("PATHS %s and %s" % (file1, file2))


# Copyright (C) 2005 David Sugar, Tycho Softworks
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
# Bayonne python libexec interface module.  

import sys, os, string

class Libexec:
	tsession = ''
	query = ''
	digits = ''
	exitcode = 0
	resultcode = 0
	reply = 0
	level = 0;
	position = '00:00:00.000'
	version = '4.0'
	voice = ''
	head = {}
	args = {}

	def __init__(self):
		self.tsession = os.environ.get('PORT_TSESSION')
		if self.tsession == "" or not self.tsession:
			self.exitcode = 3
			return
		sys.stdout.write("%s HEAD\n" % self.tsession)
		sys.stdout.flush()
		while self.exitcode < 1:
			line = sys.stdin.readline()
			if not line:
				exitcode = 3
				break
			if line == "\n":
				break
			try:
				num = int(line.strip().split()[0])
			except (ValueError, IndexError):
				num = 0
			if num > 900:
				self.exitcode = num - 900
			if num > 0:
				self.reply = num
				continue
			tmp = line.split(': ')
			tmp[1] = tmp[1][:-1]
			self.head[tmp[0]] = tmp[1]
		sys.stdout.write("%s ARGS\n" % self.tsession)
		sys.stdout.flush()
		while self.exitcode < 1:
			line = sys.stdin.readline()
			if not line:
				exitcode = 3
				break
			if line == "\n":
				break
			try:
				num = int(line.strip().split()[0])
			except (ValueError, IndexError):
				num = 0
			if num > 900:
				self.exitcode = num - 900
			if num > 0:
				self.reply = num
				continue
			tmp = line.split(': ')
			tmp[1] = tmp[1][:-1]
			self.args[tmp[0]] = tmp[1]
		return

	def postSymbol(self, id, value):
		sid = self.head['SESSION']
		sys.stdout.write("%s POST %s %s" % (sid, id, value))
		return		

	def pathname(self, file):
		ext = self.head['EXTENSION']
		var = os.environ.get('SERVER_PREFIX')
		ram = os.environ.get('SERVER_TMPFS')
		tmp = os.environ.get('SERVER_TMP')

		if not file or file == "" or ".." in file or "/." in file:
			return ""

		if file[0:1] == '/' or file[0:1] == '.':
			return ""
		
		spos = file.rfind('/')
		epos = file.rfind('.')

		if epos < spos:
			epos = -1

		if epos < 0:
			file = "%s%s" % (file, ext)
	
		if file[:4] == "tmp:":
			return "%s/%s" % (tmp, file[4:])		

		if file[:4] == "ram:":
			return "%s/%s" % (ram, file[4:])

		if file.count(':') > 0:
			return ""

		if file.count('/') < 1:
			try:
				prefix = self.head['PREFIX']

			except (KeyError):
				return ""

			return "%s/%s/%s" % (var, prefix, file)

		return "%s/%s" % (var, file)

	def filename(self, file):
		if not file or file == "" or ".." in file or "/." in file:
			return ""

		if file[0:1] == '/' or file[0:1] == '.':
			return ""
		
		if file[:4] == "tmp:":
			return file		

		if file[:4] == "ram:":
			return file

		if file.count(':') > 0:
			return ""

		if file.count('/') < 1:
			try:
				prefix = self.head['PREFIX']

			except (KeyError):
				return ""

			return "%s/%s" % (prefix, file)

		return file

	def echo(self, text):
		if self.tsession == "" or not self.tsession:
			sys.stdout.write("%s\n" % text)
			sys.stdout.flush()
		else:
			sys.stderr.write("%s\n" % text)
			sys.stderr.flush()
		return

	def hangup(self):
		if self.tsession == "" or not self.tsession or self.exitcode != 0:
			return
		sys.stdout.write("%s HANGUP\n" % self.tsession)
		sys.stdout.flush()
		self.tsession = ""
		return

	def error(self, code):
		if self.tsession == "" or not self.tsession or self.exitcode != 0:
			return
		sys.stdout.write("%s ERROR %s\n" % (self.tsession, code))
		sys.stdout.flush()
		self.tsession = ""
		return

	def detach(self, code):
		if self.tsession == "" or not self.tsession or self.exitcode != 0:
                        return     
		sys.stdout.write("%s EXIT %d\n" % (self.tsession, code))
		sys.stdout.flush()
		self.tsession = ""
		return
		
	def command(self, text):
		self.resultcode = 255
		self.query = ""
		if self.tsession == "" or not self.tsession or self.exitcode != 0:
			return 255
		num = 0
		sys.stdout.write("%s %s\n" % (self.tsession, text))
		sys.stdout.flush()
		while self.exitcode == 0:
			line = sys.stdin.readline()
			if not line:
				self.exitcode = 3
				break
			if line == "\n":
				break
			try:
				num = int(line.strip().split()[0])
			except (ValueError, IndexError):
				num = 0
			if num > 900:
				self.exitcode = num - 900
				break
			if num > 0:
				self.reply = num
				continue
			tmp = line.split(': ')
			value = tmp[1][:-1]
			keyword = tmp[0]
			if self.reply == 100:
				if keyword == "RESULT":
					self.resultcode = int(value)
				elif keyword == "DIGITS":
					self.digits = value
				elif keyword == "POSITION":
					self.position = value
			elif self.reply == 200:
				self.query = value
				self.resultcode = 0
		return self.resultcode

	def replayFile(self, file):
		file = self.filename(file)
		if not file or file == "":
			self.resultcode = 254
			return 254
		return self.command("REPLAY %s" % file)

	def replayOffset(self, file, offset):
		file = self.filename(file)
		if not file or file == "":
			self.resultcode = 254
			return 254
		return self.command("REPLAY %s %s" % (file, offset))

	def recordFile(self, file, total, silence):
		file = self.filename(file)
		if not file or file == "":
			self.resultcode = 254
			return 254
		return self.command("RECORD %s %d %d" % (file, total, silence))

	def recordOffset(self, file, offset, total, silence):
		file = self.filename(file)
		if not file or file == "":
			self.resultcode = 254
			return 254
		return self.command("RECORD %s %d %d %s" % (file, total, silence, offset))

	def moveFile(self, file1, file2):
		file1 = self.pathname(file1)
		file2 = self.pathname(file2)
		if not file1 or file1 == "" or not file2 or file2 == "":
			self.resultcode = 254
			return 254
		os.rename(file1, file2)
		self.resultcode = 0
		return 0

	def eraseFile(self, file):
		file = self.pathname(file)
		if not file or file == "":
			self.resultcode = 254
			return 254
		os.unlink(file)
		self.resultcode = 0
		return 0

	def setVoice(self, voice):
		self.voice = voice
		return true

	def setLevel(self, level):
		self.level = level
		return

	def clearInput(self):
		return self.command("FLUSH")

	def readKey(self, timeout):
		if self.tsession == "" or not self.tsession or self.exitcode != 0:
			return ""
		self.digits = ""
		if self.command("READ %d" % timeout) != 0:
			return ""
		return self.digits

	def readInput(self, count, timeout):
		if self.tsession == "" or not self.tsession or self.exitcode != 0:
			return ""
		self.digits = ""
		if self.command("READ %d %d" % (timeout, count)) != 0:
			return ""
		return self.digits

	def waitInput(self, timeout):
		if self.tsession == "" or not self.tsession or self.exitcode != 0:
			return false
		self.digits = ""
		if self.command("WAIT %d" % timeout) != 0:
			return false
		if len(self.digits) > 0:
			return true
		return false

	def result(self, text):
		return self.command("RESULT %s" % text)

	def xferCall(self, text):
		return self.command("XFER %s" % text)

	def playTone(self, tone, duration):
		return self.command("TONE %s %d %d" % (tone, duration, self.level))

	def playSingleTone(self, tone, duration):
		return self.command("STONE %d %d %d" % (tone, duration, self.level))

	def playDualTone(self, tone1, tone2, duration):
		return self.command("DTONE %d %d %d %d" % (tone1, tone2, duration, self.level))

	def speak(self, text):
		if self.voice == "" or not self.voice:
			voice = "PROMPT"
		else:
			voice = self.voice
		return self.command("%s %s" % (voice, text))

	def sizeSymbol(self, sym, count):
		return self.command("NEW %s %d" % (sym, count))

	def setSymbol(self, sym, value):
		return self.command("SET %s %s" % (sym, value))

	def addSymbol(self, sym, value):
		return self.command("ADD %s %s" % (sym, value))

	def getSymbol(self, sym):
		if self.tsession == "" or not self.tsession or self.exitcode != 0:
			return ""
		self.query = ""
		self.reply = 0
		self.command("GET %s" % sym)
		if self.reply != 200:
			return ""
		return self.query 

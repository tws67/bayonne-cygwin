// Copyright (C) 2005 Open Source Telecom Corp.
//  
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software 
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#include "module.h"
#include "private.h"

namespace binder {
using namespace ost;
using namespace std;

ParseThread::ParseThread(ScriptInterp *interp, const char *url, ScriptImage **ipt) :
ScriptThread(interp, 0), XMLStream()
{
	const char *ext = strrchr(url, '.');
	if(!stricmp(ext, ".xml"))
		ext = "";
	else
		ext = ".xml";

	if(*url == '/')
		snprintf(buffer, sizeof(buffer), "%s%s", url, ext);
	else
#ifdef	WIN32
		snprintf(buffer, sizeof(buffer), "%s/%s%s",
			"C:\\Program Files\\GNU Telephony\\Bayonne XML", 
			url, ext);
#else
		snprintf(buffer, sizeof(buffer), "%s/%s%s",
			BAYONNE_FILES "/bayonnexml", url, ext);
#endif

#ifdef	WIN32
	file.handle = INVALID_HANDLE_VALUE;
#else
	file.fd = -1;
#endif

	current = &main;
	document = false;
	voice = NULL;
	lang = NULL;
	img = NULL;
	ip = ipt;
	bcount = 0;
	bnext = NULL;
}

ParseThread::~ParseThread()
{
	terminate();
#ifdef	WIN32
	if(file.handle != INVALID_HANDLE_VALUE)
		CloseHandle(file.handle);
#else
	::close(file.fd);
#endif

	if(img)
	{
		delete img;
		img = NULL;
	}
}

int ParseThread::read(unsigned char *buffer, size_t len)
{
#ifdef	WIN32
	DWORD count;
	if(!ReadFile(file.handle, buffer, (DWORD)len, &count, NULL))
		return -1;
	return count;
#else
	return ::read(file.fd, (char *)buffer, len);
#endif
}

void ParseThread::characters(const unsigned char *text, size_t len)
{
	const char *vargs[130];
	const char **args = vargs; 
	unsigned diff;
	unsigned nlcount = 0;
//	const char *nlargs[2] = {" ", NULL};

	if(!len || textstate[textstack] == TEXT_NONE)
		return;

	if(textstate[textstack] == TEXT_TTS && current->ttsvoice)
	{
		args[0] = "=voice";
		args[1] = current->ttsvoice;
		args += 2;
	}
		
	while(len)
	{
		if(*text == '\r')
		{
			--len;
			++text;
		}
		if(*text == '\n' && len)
		{
			--len;
			++text;
			++nlcount;
			continue;
		}
		diff = img->getList(args, (const char *)text, len, 127);
		len -= diff;
		text += diff;
		if(!*args)
			continue;
		switch(textstate[textstack])
		{
		case TEXT_DEBUG:
			img->addCompile(current, 0, "-debug", args);
			break;
		case TEXT_ERROR:
			img->addCompile(current, 0, "-error", args);
			break;
		case TEXT_NOTICE:
			img->addCompile(current, 0, "-notice", args);
			break;
		case TEXT_ECHO:
			img->addCompile(current, 0, "echo", args);
			break;
		case TEXT_TTS:
			img->addCompile(current, current->ttsmask, "say", vargs);
			break;
		default:
			break;
		}
		nlcount = 0;
	}
}

void ParseThread::startDocument(const char **attrib)
{
	const char *vargs[3];
	const char *voicelib = NULL;
	char *p;

	memset(vargs, 0, sizeof(vargs));

	while(attrib && *attrib)
	{
		if(!stricmp(*attrib, "voice"))
			voicelib = img->dupString(*(++attrib));
		else
			++attrib;
		++attrib;
	}

	if(voicelib)
	{
		voicelib = img->dupString(voicelib);
		voice = img->dupString(voicelib);
		p = strchr(voicelib, '/');
		if(p)
		{
			*p = 0;
			lang = voicelib;
		}
		else
			voicelib = NULL;
	}

	if(!voicelib)
	{
		lang = "none";
		voice = "none/prompts";
	}

	img->getCompile(&main);
	main.logname = interp->getLogname();
	current = &main;
	vargs[0] = lang;
	vargs[1] = voice;
	if(voice)
		img->addCompile(current, 0, "-voice", vargs);
	textstate[0] = TEXT_NONE;
	textstack = 0;
	document = true;
}

void ParseThread::startElement(const unsigned char *name, const unsigned char **attrib)
{
	if(!document)
	{
		if(!stricmp((const char *)name, "bayonnexml"))
			startDocument((const char **)attrib);
		else
			return;
	}

	if(!stricmp((const char *)name, "echo"))
		doLog((const char **)attrib, TEXT_ECHO);
	else if(!stricmp((const char *)name, "answer"))
		doAnswer();
	else if(!stricmp((const char *)name, "text"))
		doText((const char **)attrib);
	else if(!stricmp((const char *)name, "goto"))
		doGoto((const char **)attrib);
	else if(!stricmp((const char *)name, "recordAudio"))
		record((const char **)attrib);
	else if(!stricmp((const char *)name, "playAudio"))
		playAudio((const char **)attrib);
	else if(!stricmp((const char *)name, "playNumber"))
		playNumber((const char **)attrib);	
	else if(!stricmp((const char *)name, "log"))
		doLog((const char **)attrib, TEXT_ERROR);
	else if(!stricmp((const char *)name, "debug"))
		doLog((const char **)attrib, TEXT_DEBUG);
	else if(!stricmp((const char *)name, "error"))
		doLog((const char **)attrib, TEXT_ERROR);
	else if(!stricmp((const char *)name, "notice"))
		doLog((const char **)attrib, TEXT_NOTICE);
	else if(!stricmp((const char *)name, "assign"))
		doAssign((const char **)attrib);
	else if(!stricmp((const char *)name, "clear"))
		doAssign((const char **)attrib);
	else if(!stricmp((const char *)name, "trap"))
		startTrap((const char **)attrib, SIGNAL_ERROR + 1);
	else if(!stricmp((const char *)name, "onTermDigit"))
		startTerm((const char **)attrib);
	else if(!stricmp((const char *)name, "onHangup"))
		startTrap((const char **)attrib, SIGNAL_EXIT + 1);
	else if(!stricmp((const char *)name, "onError"))
		startTrap((const char **)attrib, SIGNAL_ERROR + 1);
	else if(!stricmp((const char *)name, "block"))
		startBlock((const char **)attrib);
	else if(!stricmp((const char *)name, "delay"))
		doDelay((const char **)attrib);
	else if(!stricmp((const char *)name, "wait"))
		doDelay((const char **)attrib);
	else if(!stricmp((const char *)name, "onMaxTime"))
		startTrap((const char **)attrib, SIGNAL_TIMEOUT + 1);
	else if(!stricmp((const char *)name, "onTimeout"))
		startTrap((const char **)attrib, SIGNAL_TIMEOUT + 1);
}

void ParseThread::endDocument(void)
{
	if(current != &main)
		img->postCompile(current);
	img->postCompile(&main);
	document = false;
}

void ParseThread::endElement(const unsigned char *name)
{
	if(!document)
		return;

	if(!stricmp((const char *)name, "bayonnexml"))
		endDocument();
	else if(!stricmp((const char *)name, "reorder"))
		doReorder();
	else if(!stricmp((const char *)name, "hangup"))
		doHangup();
	else if(!stricmp((const char *)name, "disconnect"))
		doHangup();
	else if(!stricmp((const char *)name, "echo"))
		--textstack;
	else if(!stricmp((const char *)name, "debug"))
		--textstack;
	else if(!stricmp((const char *)name, "text"))
		--textstack;
	else if(!stricmp((const char *)name, "error"))
		--textstack;
	else if(!stricmp((const char *)name, "notice"))
		--textstack;
	else if(!stricmp((const char *)name, "log"))
		--textstack;
	else if(!stricmp((const char *)name, "trap"))
		endTrap();
	else if(!stricmp((const char *)name, "onHangup"))
		endTrap();
	else if(!stricmp((const char *)name, "onError"))
		endTrap();
	else if(!stricmp((const char *)name, "onTimeout"))
		endTrap();
	else if(!stricmp((const char *)name, "onMaxTime"))
		endTrap();
	else if(!stricmp((const char *)name, "onTermDigit"))
		endTerm();
	else if(!stricmp((const char *)name, "block"))
		endBlock();
}

void ParseThread::playNumber(const char **attrib)
{
	const char *value = NULL;
	const char *term = NULL;
	const char *format = NULL;
	const char *clear = NULL;
	unsigned long tmask = 0;
	const char *args[5];

	while(attrib && *attrib)
	{
		if(!stricmp(*attrib, "value"))
			value = img->dupString(*(++attrib));
		else if(!stricmp(*attrib, "format"))
			format = *(++attrib);
		else if(!stricmp(*attrib, "termDigits"))
			term = *(++attrib);
		else if(!stricmp(*attrib, "clearDigits"))
			clear = *(++attrib);
		else
			++attrib;
		++attrib;
	}

	if(!value)
		return;

	if(!stricmp(format, "number"))
		format = "&number";
	else
		format = "&spell";

        while(term && *term)
                tmask |= (1 << (getDigit(*(term++)) + 5));

        if(tmask)
                tmask |= 0x03;

        if(clear && !stricmp(clear, "true"))
                img->addCompile(current, 0, "cleardigits", NULL);

	args[0] = format;
	args[1] = value;
	args[2] = NULL;
	if(current->form)
		img->addCompile(current, 0, "prompt", args);
	else
		img->addCompile(current, 0, "play", args);
}

void ParseThread::playTone(const char **attrib)
{
	const char *value = NULL;
	const char *term = NULL;
	const char *clear = NULL;
	const char *duration = NULL;
	const char *format = NULL;
	const char *args[12];
	unsigned argc;
	unsigned long tmask = 0;

	memset(args, 0, sizeof(args));
	while(attrib && *attrib)
	{
		if(!stricmp(*attrib, "value"))
			value = img->dupString(*(++attrib));
		else if(!stricmp(*attrib, "maxTime"))
			duration = img->dupString(*(++attrib));
		else if(!stricmp(*attrib, "clearDigits"))
			clear = *(++attrib);
		else if(!stricmp(*attrib, "termDigits"))
			term = *(++attrib);
		else if(!stricmp(*attrib, "format"))
			format = img->dupString(*(++attrib));
		else
			++attrib;
		++attrib;
	}

	if(!value)
		return;

        if(clear && !stricmp(clear, "true"))
                img->addCompile(current, 0, "cleardigits", NULL);

        while(term && *term)
                tmask |= (1 << (getDigit(*(term++)) + 5));

        if(tmask)
                tmask |= 0x07;

	args[0] = "=name";
	args[1] = value;
	argc = 2;
	if(duration)
	{
		args[argc++] = "=duration";
		args[argc++] = duration;
	}
	if(format)
	{
		args[argc++] = "=location";
		args[argc++] = format;
	}
	img->addCompile(current, tmask, "teltone", args);
}

void ParseThread::getDigits(const char **attrib)
{
	char vname[65];

	const char *var = NULL;
	const char *count = NULL;
	const char *term = NULL;
	const char *exit = NULL;
	const char *clear = NULL;
	const char *timeout = "30s";
	const char *interdigit = "5s";
	const char *format = NULL;
	const char *args[16];
	unsigned argc = 0;

	memset(args, 0, sizeof(args));

	while(attrib && *attrib)
	{
		if(!stricmp(*attrib, "var"))
		{
			var = *(++attrib);
			if(*var == '%' || *var == '&')
				var = img->dupString(var);
			else
			{
				vname[0] = '%';
				setString(vname + 1, sizeof(vname) -1, var);
				var = img->dupString(vname);
			}
		}
		else if(!stricmp(*attrib, "maxTime") || !stricmp(*attrib, "timeout"))
			timeout = img->dupString(*(++attrib));
		else if(!stricmp(*attrib, "maxSilence") || !stricmp(*attrib, "interdigit"))
			interdigit = img->dupString(*(++attrib));
		else if(!stricmp(*attrib, "termDigits"))
			term = *(++attrib);
		else if(!stricmp(*attrib, "maxDigits"))
			count = *(++attrib);
		else if(!stricmp(*attrib, "clearDigits"))
			clear = *(++attrib);
		else
			++attrib;
		++attrib;
	}

	if(term)
		exit = img->dupString(term);

	if(clear && !stricmp(clear, "true"))
		img->addCompile(current, 0, "cleardigits", NULL);

	if(var)
		args[argc++] = var;
	if(timeout)
	{
		args[argc++] = "=timeout";
		args[argc++] = timeout;
	}
	if(interdigit)
	{
		args[argc++] = "=interdigit";
		args[argc++] = interdigit;
	}
	if(exit)
	{
		args[argc++] = "=exit";
		args[argc++] = exit;
	}
	if(format)
	{
		args[argc++] = "=format";
		args[argc++] = format;
	}
	if(count)
	{
		args[argc++] = "=count";
		args[argc++] = count;
	}
	img->addCompile(current, 0, "collect", args);
}
					
void ParseThread::record(const char **attrib)
{
	const char *value = NULL;
	const char *term = NULL;
	const char *clear = NULL;
	const char *timeout = "30s";
	const char *silence = "5s";
	const char *beep = "true";
	const char *encoding = NULL;
	const char *args[12];
	unsigned argc;
	unsigned long tmask = 0;

	memset(args, 0, sizeof(args));
	while(attrib && *attrib)
	{
		if(!stricmp(*attrib, "value"))
			value = img->dupString(*(++attrib));
		else if(!stricmp(*attrib, "maxTime"))
			timeout = img->dupString(*(++attrib));
		else if(!stricmp(*attrib, "maxSilence"))
			silence = img->dupString(*(++attrib));
		else if(!stricmp(*attrib, "clearDigits"))
			clear = *(++attrib);
		else if(!stricmp(*attrib, "termDigits"))
			term = *(++attrib);
		else if(!stricmp(*attrib, "beep"))
			beep = *(++attrib);
		else if(!stricmp(*attrib, "format"))
		{
			encoding = img->dupString(*(++attrib));
			if(!stricmp(encoding, "audio/vox"))
				encoding = "vox";
			else if(!stricmp(encoding, "audio/basic"))
				encoding = "ulaw";
			else if(!stricmp(encoding, "audio/adpcm"))
				encoding = "adpcm";
		}
		else
			++attrib;
		++attrib;
	}

	if(!value)
		return;

        if(clear && !stricmp(clear, "true"))
                img->addCompile(current, 0, "cleardigits", NULL);

        while(term && *term)
                tmask |= (1 << (getDigit(*(term++)) + 5));

        if(tmask)
                tmask |= 0x07;

	if(beep && !stricmp(beep, "true"))
		img->addCompile(current, tmask, "beep", NULL);

	args[0] = value;
	argc = 1;
	if(timeout)
	{
		args[argc++] = "=timeout";
		args[argc++] = timeout;
	}
	if(encoding)
	{
		args[argc++] = "=encoding";
		args[argc++] = encoding;
	}
	if(silence)
	{
		args[argc++] = "=silence";
		args[argc++] = silence;
	}
	img->addCompile(current, tmask, "record", args);
}


void ParseThread::playAudio(const char **attrib)
{
	const char *value = NULL;
	const char *term = NULL;
	const char *clear = NULL;
	unsigned long tmask = 0;
	
	const char *args[10];

	while(attrib && *attrib)
	{
		if(!stricmp(*attrib, "value"))
			value = img->dupString(*(++attrib));
		else if(!stricmp(*attrib, "termDigits"))
			term = *(++attrib);
		else if(!stricmp(*attrib, "clearDigits"))
			clear = *(++attrib);
		else
			++attrib;
		++attrib;
	}

	if(!value)
		return;

	if(clear && !stricmp(clear, "true"))
		img->addCompile(current, 0, "cleardigits", NULL);
	
        while(term && *term)
                tmask |= (1 << (getDigit(*(term++)) + 5));

        if(tmask)
                tmask |= 0x03;

	if(*value != '/' && !strchr(value, ':'))	
	{
		args[0] = value;
		args[1] = NULL;
		if(current->form)
			img->addCompile(current, 0, "prompt", args);
		else
			img->addCompile(current, tmask, "play", args);
		return;
	}

	if(current->form)
		args[0] = "definitions::url-prompt";
	else
		args[0] = "definitions::url-play";
	args[1] = "=from";
	args[2] = value;
	args[3] = NULL;	
	img->addCompile(current, tmask, "call", args);
}

void ParseThread::doGoto(const char **attrib)
{
	const char *submit = NULL;
	const char *value = NULL;
	const char *label = NULL;
	const char *args[10];
	unsigned argc = 0;
	char *p, *q;
	char *tok;
	const char *cp;
	char vname[65];

	while(attrib && *attrib)
	{
		if(!stricmp(*attrib, "value"))
			value = img->dupString(*(++attrib));
		else if(!stricmp(*attrib, "submit"))
			submit = img->dupString(*(++attrib));
		else
			++attrib;
		++attrib;
	}

	if(!value)
		return;

	if(!strchr(value, '/'))
	{
		if(*value == '#')
			++value;
		args[0] = value;
		args[1] = NULL;
		img->addCompile(current, 0, "startxml", args);
		return;
	}

	p = strchr(value, '#');		
	if(p)
	{
		*p = 0;
		label = img->dupString((const char *)(p + 1));
		q = strchr(label, '?');
		if(q)
		{
			*(q++) = 0;
			strcpy(p, q);
		}				
	}

	if(submit)
	{
		args[0] = "%session.xmlquery";
		if(strchr(value, '?'))
		{
			args[1] = NULL;
			img->addCompile(current, 0, "clear", args);
		}
		else
		{
			args[1] = "{?";
			args[2] = NULL;
			img->addCompile(current, 0, "set", args);
		}
		cp = strtok_r((char *)submit, ",:;%&", &tok);
		while(cp)
		{
			if(argc++)
				snprintf(vname, sizeof(vname), "{&%s=", cp);
			else
				snprintf(vname, sizeof(vname), "{%s=", cp);
			p = strchr(vname, '.');
			if(p)
				*p = '_';
			args[1] = img->dupString(vname);
			vname[0] = '%';
			setString(vname + 1, sizeof(vname) - 1, cp);
			args[2] = img->dupString(vname);
			args[3] = 0;
			img->addCompile(current, 0, "add", args);
			cp = strtok_r(NULL, ",:;%&", &tok);
		}
	}	

	args[0] = "definitions::xml-parse";
	args[1] = "=from";
	args[2] = value;
	argc = 3;
	if(label)
	{
		args[argc++] = "=goto";
		args[argc++] = label;
	}
	if(submit)
	{
		args[argc++] = "=query";
		args[argc++] = "%session.xmlquery";
	}
	args[argc] = NULL;
	img->addCompile(current, 0, "call", args);
};

void ParseThread::startBlock(const char **attrib)
{
	const char *name = NULL;
	const char *repeat = NULL;
	const char *clear = NULL;
	const char *args[3];
	const char *vargs[3];
	char nbuf[10];

	memset(vargs, 0, sizeof(vargs));

	bnext = NULL;
	while(attrib && *attrib)
	{
		if(!stricmp(*attrib, "label") || !stricmp(*attrib, "name"))
			name = *(++attrib);
		else if(!stricmp(*attrib, "repeat"))
			repeat = *(++attrib);
		else if(!stricmp(*attrib, "cleardigits"))
			clear = *(++attrib);
		else if(!stricmp(*attrib, "next"))
			bnext = *(++attrib);
		else
			++attrib;
		++attrib;
	}

	if(name && *name == '#')
		++name;

	if(!name && bcount++)
	{
		snprintf(nbuf, sizeof(nbuf), "%ud", bcount);
		name = nbuf;
	}

	if(name)
	{
		img->getCompile(&block, img->dupString(name));
		block.logname = interp->getLogname();
		current = &block;
		vargs[0] = lang;
		vargs[1] = voice;
		if(voice)
			img->addCompile(current, 0, "-voice", vargs);
	}
		
	if(repeat)
	{
		args[0] = img->dupString(repeat);
		img->addCompile(current, 0, "repeat", args);
		current->repeated = true;
	}

	if(clear && !stricmp(clear, "true"))
		img->addCompile(current, 0, "cleardigits", NULL);
}
	
void ParseThread::endBlock(void)
{
	char *p, *q;
	const char *label;
	const char *args[10];

	current->trap = 0;
	if(current->repeated)
		img->addCompile(current, 0, "loop", NULL);
	current->repeated = false;
	if(bnext)
	{
		if(!strchr(bnext, '/'))
		{
			if(*bnext == '#')
				++bnext;
			args[0] = bnext;
			args[1] = NULL;
			img->addCompile(current, 0, "startxml", args);
		}
		else
		{
			label = NULL;
			p = strchr(bnext, '#');
			if(p)
			{
				*p = 0;
				label = img->dupString((const char *)(p + 1));
				q = strchr(label, '?');
				if(q)
				{
					*(q++) = 0;
					strcpy(p, q);
				}
			}
			args[0] = "definitions::xml-parse";
			args[1] = "=from";
			args[2] = bnext;
			args[3] = NULL;
			if(label)
			{
				args[3] = "=goto";
				args[4] = label;
				args[5] = NULL;
			}
			img->addCompile(current, 0, "call", args);
		}
		bnext = NULL;
	}
	if(current != &main)
	{
		img->postCompile(current);
		current = &main;
	}
}

void ParseThread::startTerm(const char **attrib)
{
	const char *value = NULL;
	const char *term;
	unsigned dig;

	current->addterm |= 0x08;
	current->trap = 4;
	while(attrib && *attrib)
	{
		if(!stricmp(*attrib, "value"))
			value = *(++attrib);
		else
			++attrib;
		++attrib;
	}

	term = value;
	current->trapmask = 0;

	if(!value)
		return;

	while(term && *term)
		current->trapmask |= (1 << (getDigit(*(term++)) + 5));

	if(isdigit(*value))
	{
		dig = getDigit(*value);
		current->trap = dig + 5;
		current->addterm |= (1 << (dig + 5));
		return;
	}

	if(!stricmp(value, "star"))
		value = "*";

	if(!stricmp(value, "hash") || !stricmp(value, "pound"))
		value = "#";

	if(*value == '*')
	{
		current->trap = 15;
		current->addterm |= (1 << 15);
		return;
	}

	if(*value == '#')
	{
		current->trap =	16;
		current->addterm |= (1 << 16);
	}
}

void ParseThread::startTrap(const char **attrib, unsigned trap)
{
	const char *id;
	while(attrib && *attrib)
	{
		if(!stricmp(*attrib, "id"))
		{
			id = *(++attrib);
			if(!stricmp(id, "hangup"))
				trap = SIGNAL_HANGUP + 1;
			else if(!stricmp(id, "error"))
				trap = SIGNAL_ERROR + 1;
			else if(!stricmp(id, "timeout"))
				trap = SIGNAL_TIMEOUT + 1;
			else if(!stricmp(id, "dtmf"))
				trap = SIGNAL_DTMF + 1;
			else if(isdigit(*id) && !id[1])
				trap = atoi(id) + 5;
			else if(!stricmp(id, "star"))
				trap = 15;
			else if(!stricmp(id, "pound"))
				trap = 16;
			else if(!stricmp(id, "tone") || !stricmp(id, "busy"))
				trap = SIGNAL_TONE + 1;
			else if(!stricmp(id, "part"))
				trap = SIGNAL_PART + 1;
			else if(!stricmp(id, "fail"))
				trap = SIGNAL_FAIL + 1;
			else if(!stricmp(id, "pickup"))
				trap = SIGNAL_PICKUP + 1;
			else if(!stricmp(id, "invalid"))
				trap = SIGNAL_INVALID + 1;
		}
		else
			++attrib;
		++attrib;
	}
	current->trap = trap;
}

void ParseThread::endTrap(void)
{
	current->trap = 0;
}

void ParseThread::endTerm(void)
{
	unsigned long mask = 1;
	unsigned count = 0;

	Line *tl = current->script->trap[current->trap];
	current->trap = 0;
	
	while(count < 32)
	{
		if(mask & current->trapmask)
			current->script->trap[count] = tl;
		mask = (mask << 1);
		++count;
	}
}

void ParseThread::doDelay(const char **attrib)
{
	const char *value = "0";
	const char *args[2];
	const char *term = NULL;
	unsigned long tmask = 0;

	args[0] = value;
	args[1] = NULL;

	while(attrib && *attrib)
	{
		if(!stricmp(*attrib, "value"))
			value = *(++attrib);
		else if(!stricmp(*attrib, "termDigits"))
			term = *(++attrib);
		else
			++attrib;
		++attrib;
	}
	while(term && *term)
		tmask |= (1 << (getDigit(*(term++)) + 5)); 

	if(tmask)
		tmask |= 0x07;

	if(!stricmp(value, "nolimit"))
		value="100000s";
	args[0] = dupString(value);
	img->addCompile(current, tmask, "sleep", args);
}
	
void ParseThread::doReorder(void)
{
	img->addCompile(current, 1, "reorder", NULL);
}

void ParseThread::doAnswer(void)
{
	img->addCompile(current, 1, "answer", NULL);
}

void ParseThread::doHangup(void)
{
	img->addCompile(current, 0, "-hangup", NULL);
}

void ParseThread::doAssign(const char **attrib)
{
	const char *size = "0";
	const char *var = "";
	const char *value = "";
	unsigned sz = 0;

	char szbuf[5];
	const char *args[7];
	while(attrib && *attrib)
	{
		if(!stricmp(*attrib, "size"))
			size = *(++attrib);
		else if(!stricmp(*attrib, "var"))
			var = *(++attrib);
		else if(!stricmp(*attrib, "name"))
			var = *(++attrib);
		else if(!stricmp(*attrib, "value"))
			value = *(++attrib);
		else
			++attrib;
		++attrib;
	}
	if(!stricmp(size, "char"))
		sz = 1;
	else if(!stricmp(size, "time"))
		sz = 8;
	else if(!stricmp(size, "date"))
		sz = 10;
	else if(!stricmp(size, "number"))
		sz = 11;

	if(!*var)
		return;

	var = img->dupString(var);
	value = img->dupString(value);
	args[0] = "=var";
	args[1] = var;
	args[2] = "=value";
	args[3] = value;

	if(sz)
	{
		snprintf(szbuf, sizeof(szbuf), "%d", sz);
		args[4] = "=size";
		args[5] = img->dupString(szbuf);
		args[6] = NULL;
	}
	else
		args[4] = NULL;

	img->addCompile(current, 0, "-assign", args);
}	

void ParseThread::doText(const char **attrib)
{
	current->ttsvoice = NULL;
	current->ttsmask = 0;

	const char *term = NULL;

	textstate[++textstack] = TEXT_TTS;
	while(attrib && *attrib)
	{
		if(!stricmp(*attrib, "voice"))
			current->ttsvoice = img->dupString(*(++attrib));
		else if(!stricmp(*attrib, "termDigits"))
			term = *(++attrib);
		else
			++attrib;
		++attrib;
	}

	while(term && *term)
		current->ttsmask |= (1 << (getDigit(*(term++) + 5)));
}

void ParseThread::doLog(const char **attrib, textstate_t mode)
{
	const char *text = NULL;
	const char *args[65];
	const char *level;

	while(attrib && *attrib)
	{
		if(!stricmp(*attrib, "text"))
			text = *(++attrib);
		else if(!stricmp(*attrib, "level"))
		{
			level = *(++attrib);
			if(!stricmp(level, "debug"))
				mode = TEXT_DEBUG;
			else if(!stricmp(level, "error"))
				mode = TEXT_ERROR;
			else if(!stricmp(level, "notice"))
				mode = TEXT_NOTICE;
			else
				mode = TEXT_ERROR;
		}
		else
			++attrib;
		++attrib;
	}
	textstate[++textstack] = mode;

	if(text)
	{
		img->getList(args, text, 0, 32);
		switch(mode)
		{
		case TEXT_ECHO:
			img->addCompile(current, 0, "echo", args);
			break;
		case TEXT_DEBUG:
			img->addCompile(current, 0, "-debug", args);
			break;
		case TEXT_NOTICE:
			img->addCompile(current, 0, "-notice", args);
		default:
			img->addCompile(current, 0, "-error", args);
			break;
		}
	}
}

void ParseThread::run(void)
{
	Methods *m = (Methods *)interp;
	ScriptImage *sp;
#ifdef	WIN32
	file.handle = CreateFile(buffer, GENERIC_READ | GENERIC_WRITE, 0,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(file.handle == INVALID_HANDLE_VALUE)
		exit("xml-failed");
#else
	file.fd = ::open(buffer, O_RDONLY);
	if(file.fd < 0)
		exit("xml-failed");
#endif
	img = new ParseImage();
	if(!parse())
		exit("xml-invalid");

	if(*ip)
	{
		Thread::yield();
		sp = *ip;
		*ip = NULL;
		delete sp;
	}

	Thread::yield();
	*ip = img;
	img = NULL;
	
	interp->setSymbol("script.error", "none");
	m->scrStart();
	exit(NULL);
}

} // end namespace

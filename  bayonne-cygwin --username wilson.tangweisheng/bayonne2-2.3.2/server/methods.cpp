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

#include "server.h"
#ifndef	WIN32
#include <sys/wait.h>
#endif

namespace server {
using namespace ost;
using namespace std;

bool Methods::scrId(void)
{
	Symbol *sym;
	const char *cp = getKeyword("size");
	const char *m = getMember();
	time_t now;
	const char *pre, *host;
	char *dp;
	const char *idprefix = getKeyword("idprefix");
	const char *prefix = getKeyword("prefix");
	const char *ext = getKeyword("extension");
	const char *id = getKeyword("id");
	unsigned size = 0;
	unsigned short uniq = 0;
	char buffer[128];
	unsigned char code;

	if(!idprefix)
		idprefix = "";

	if(prefix && (*prefix == '.' || *prefix == '/'))
	{
		error("invalid-path");
		goto clearall;
	}

	if(prefix && (strstr(prefix, "/.") || strchr(prefix, ':')))
	{
		error("invalid-path");
		goto clearall;
	}
	
	if(cp)
		size = atoi(cp);
	else if(prefix && id)
		size = 128;
	else if(m && !stricmp(m, "uniq") && prefix)
		size = 128;
	else if(m && !stricmp(m, "uniq"))
		size = 16;
	else if(id)
		size = strlen(id);

	if(!ext)
		ext = ".au";

	time(&now);
	while(NULL != (cp = getOption(NULL)))
	{
		if(*cp == '%' || *cp == '&')
			++cp;
		sym = mapSymbol(cp, size);
		if(!sym)
			continue;
		if(sym->type != symNORMAL && sym->type != symCONST && sym->type != symINITIAL)
		{
			error("cannot-access");
			return true;
		}
		if(prefix && sym->type == symCONST)
		{
			error("cannot-access");
			return true;
		}
		if(!sym->data[0] && id)
			commit(sym, id);
		if(sym->type == symINITIAL)
			sym->type = symNORMAL;
		if(m && !stricmp(m, "uniq"))
		{
			if(sym->type == symCONST)
				continue;
			snprintf(sym->data, sym->size + 1, "%08lx-%02d%04d", 
				now, ++uniq, timeslot);				
			goto modify;
		}
		if(m && !stricmp(m, "contact"))
		{
			pre = strchr(sym->data, ':');
			host = strchr(sym->data, '@');
			dp = sym->data;
			if(!pre || (host && pre > host))
				pre = sym->data;
			else
				++pre;
			if(*pre == '+')
				++pre;
			while(*pre && *pre != ':')
				*(dp++) = *(pre++);
			*dp = 0;
			goto modify;
		}	
		if(m && !strnicmp(m, "base", 4))
		{
			pre = strrchr(sym->data, '/');
			if(!pre)
				goto modify;			
			++pre;
			goto update;
		}
		if(m && !strnicmp(m, "host", 4))
		{
            host = strchr(sym->data, '@');
            dp = sym->data;
            if(!host)
                host = sym->data;
            else
                ++host;
            while(*host && *host != ':')
                *(dp++) = *(host++);
            *dp = 0;
            goto modify;
		}				
		if(m && !stricmp(m, "dtmf"))
		{
			dp = sym->data;
			pre = sym->data;
			while(*pre)
			{
				code = (unsigned char)*(pre++);
				if(dtmf_keymap[code])
					*(dp++) = dtmf_keymap[code];
			}
			*dp = 0;
			goto modify;
		}				
		pre = getRegistryId(sym->data);
		if(*pre == '+')
			++pre;
update:
		dp = sym->data;
		while(pre && *pre && *pre != ':' && *pre != '@')
			*(dp++) = *(pre++);
		*dp = 0;
modify:
		if(!prefix)
			continue;

		if(strchr(sym->data, ':') || strchr(sym->data, '/') || sym->data[0] == '.')
		{
			clear(sym);
			continue;
		}
		snprintf(buffer, sizeof(buffer), "%s/%s%s%s", 
			prefix, idprefix, sym->data, ext);
		commit(sym, buffer);
	}
	advance();
	return true;
clearall:
	while(NULL != (cp = getOption(NULL)))
	{
		sym = mapSymbol(cp, 0);
		if(sym)
			clear(sym);
	}
	return true;
}

#ifdef	WIN32
bool Methods::scrDetach(void)
{
	error("no-detach");
	return true;
}
#else
bool Methods::scrDetach(void)
{
	Line *line = getLine();
	char *argv[128];
	int argc = 0;
	const char *cp = getValue(NULL);
	const char *v;
	char *up;
	int pid, fd;
	int max_fd = 256;
	struct rlimit rlim;
	unsigned idx = 0;
	char name[65];
	char execpath[128];

	v = strrchr(cp, '/');
	if(v)
	{
		error("cannot use path");
		return true;
	}
	else
		v = cp;

	snprintf(execpath, sizeof(execpath), "%s/%s", LIBEXEC_FILES, cp);
	while(NULL != (cp = getValue(NULL)))
		argv[argc++] = (char *)cp;

	argv[argc] = NULL;

	pid = fork();
	if(pid < 0)
	{
		error("detach-failed");
		return true;
	}
	if(pid > 0)
	{
		waitpid(pid, NULL, 0);
		setSymbol("_detach_", v, 16);
		advance();
		return true;
	}
	if(!getrlimit(RLIMIT_NOFILE, &rlim))
		max_fd = rlim.rlim_max;
	for(fd = 3; fd < max_fd; ++fd)
		::close(fd);
	::signal(SIGQUIT, SIG_DFL);
	::signal(SIGINT, SIG_DFL);
	::signal(SIGCHLD, SIG_DFL);
	fd = ::open("/dev/null", O_RDWR);
	if(fd > 2)
	{
		::dup2(fd, 0);
		::dup2(fd, 1);
		::close(fd);
	}
	pid = fork();
	if(pid)
		::exit(0);

	idx = 0;
	while(idx < line->argc)
	{
		cp = line->args[idx++];
		if(*cp != '=')
			continue;

		v = getContent(line->args[idx++]);
		if(!v)
			continue;
	
		snprintf(name, sizeof(name), "param_%s", ++cp);
		up = name;
		while(*up)
		{
			*up = toupper(*up);
			++up;
		}
		Process::setEnv(name, v, true);
	}
		
	::execv(execpath, argv);
	::exit(-1);	
}
#endif
	
bool Methods::scrReconnect(void)
{
	const char *encoding = getKeyword("encoding");
	timeout_t framing = getTimeoutKeyword("framing");

	if(!framing || framing == TIMEOUT_INF)
		framing = 0;

	if(setReconnect(encoding, framing))
		return false;

	error("reconnect-unsupported");
	return true;
}
		
bool Methods::scrParam(void)
{
	Line *line = getLine();
	const char *id, *value;
	unsigned pos = 0;
	char buf[80];
	
	while(pos < line->argc)
	{
		id = line->args[pos++];
		if(!id)
			break;

		if(*id != '=')
			continue;
		++id;
		value = getContent(line->args[pos++]);
		if(!value)
			continue;

		snprintf(buf, sizeof(buf), "param.%s", id);
		setConst(buf, value);
	}
	advance();
	return true;
}

bool Methods::scrExit(void)
{
	const char *reason = getValue(NULL);
	ScriptMethods *methods = (ScriptMethods *)this;

	if(reason)
		setConst("session.exit_reason", reason);

	return methods->scrExit();
}

bool Methods::scrTrap(void)
{
	const char *id = getKeyword("id");
	const char *text = getKeyword("text");

	if(!id)
		id = "6";

	Bayonne::snmptrap(atoi(id), text);
	advance();
	return true;
}

bool Methods::scrSay(void)
{
	char buf[256];
	Name *tts;
	Name *scr = getName();
	Line *line = getLine();
	const char *ext = getKeyword("extension");
	const char *prefix = getKeyword("prefix");
	const char *voice = getKeyword("voice");
	const char *file = getKeyword("file");
	const char *opt;
	unsigned len = 0;
	char keybuf[65];

	snprintf(buf, sizeof(buf), "definitions::tts-%s", 
		keylibexec.getString("tts", keybuf, sizeof(keybuf)));

	tts = getScript(buf);
	if(!tts)
	{
		error("tts-missing");
		return true;
	}

	if(!voice)
		voice = keylibexec.getString("voice", keybuf, sizeof(keybuf));

	while((NULL != (opt = getValue(NULL))) && (len < (sizeof(buf) - 2)))
	{
		setString(buf + len, sizeof(buf) - len, opt);
		len += strlen(opt);
	}		

	if(!push())
	{
		error("tts-overflow");
		return true;
	}

	frame[stack].tranflag = true;
	frame[stack].index = 0;
	frame[stack].caseflag = false;
	frame[stack].local = new ScriptSymbols();
	frame[stack].script = tts;
	frame[stack].line = frame[stack].first = tts->first;
	frame[stack].mask = scr->mask & line->mask;
	frame[stack].mask |= tts->mask;
	updated = true;


	setConst("voice", voice);
	setConst("text", buf);
	if(prefix)
		setConst("prefix", prefix);
	if(ext)
		setConst("extension", ext);
	if(file)
		setConst("file", file);
	setConst("limit", keylibexec.getString("ttslimit", keybuf, sizeof(keybuf)));
	return true;
}

bool Methods::scrTransfer(void)
{
	char nbuf[MAX_PATHNAME];
	unsigned len = 0;
	const char *cp;
	const char *pre = getKeyword("prefix");
	const char *svr = getKeyword("server");

	if(!svr)
		svr = driver->getLast("outbound");

	if(!svr)
		svr = getSymbol("session.uri_server");

	if(!pre)
		pre = driver->getLast("transfer");

	while(NULL != (cp = getValue(NULL)))
	{
		snprintf(nbuf + len, sizeof(nbuf) - len, "%s", cp);
		len = strlen(nbuf);
	}

	switch(iface)
	{
	case IF_INET:
		pre = driver->getLast("urlprefix");
		if(!pre)
			pre = "";

		if(strchr(nbuf, '@'))
			svr = NULL;

                len = strlen(pre);
                if(svr && len && !strnicmp(pre, svr, len))
                        svr += len;
                else if(!svr && len && !strnicmp(pre, nbuf, len))
                        pre = "";

		if(svr)
			snprintf(state.url.buf, sizeof(state.url.buf), "%s%s@%s", pre, nbuf, svr);
		else
			snprintf(state.url.buf, sizeof(state.url.buf), "%s%s", pre, nbuf);

		state.url.ref = state.url.buf;
		setState(STATE_XFER);
		return false;
	case IF_PSTN:	
		if(!pre)
			pre = "!";
		state.tone.exiting = true;
		if(audio.tone)
			delete audio.tone;

		snprintf(state.tone.digits, sizeof(state.tone.digits), "%s%s", pre, nbuf);
		audio.tone = new DTMFTones(state.tone.digits, 20000, getToneFraming(), driver->getInterdigit());
		setState(STATE_TONE);
		return false;
	default:
		error("not-supported");
		return true;
	}
}


bool Methods::scrDTMF(void)
{
	const char *cp = getKeyword("level");
	Audio::Level level = 26000;
	Audio::timeout_t interdigit;
	unsigned len = 0;

	if(cp)
		level = atoi(cp);

	// lets not detect ourself...
	disableDTMF();

	interdigit = getTimeoutKeyword("interdigit");
	state.timeout = TIMEOUT_INF;
	if(interdigit == TIMEOUT_INF || !interdigit)
		interdigit = driver->getInterdigit();
	if(interdigit > 1000)
		interdigit /= 1000;
	
	if(audio.tone)
		delete audio.tone;

	state.tone.syncdigit = state.tone.digits;
	state.tone.synctimer = interdigit * 2;
	state.tone.exiting = false;
	state.tone.digits[0] = 0;
	while(len < 64 && NULL != (cp = getValue(NULL)))
	{
		snprintf(state.tone.digits + len, sizeof(state.tone.digits) - len, "%s", cp);		
		len = strlen(state.tone.digits);
	}

	audio.tone = new DTMFTones(state.tone.digits, level, getToneFraming(), interdigit);	
	setState(STATE_DTMF);
	return false;
}

bool Methods::scrMF(void)
{
	const char *cp = getKeyword("level");
	Audio::Level level = 26000;
	Audio::timeout_t interdigit;
	unsigned len = 0;

	if(cp)
		level = atoi(cp);

	disableDTMF();

	interdigit = getTimeoutKeyword("interdigit");
	state.timeout = TIMEOUT_INF;
	if(interdigit == TIMEOUT_INF || !interdigit)
		interdigit = driver->getInterdigit();
	if(interdigit > 1000)
		interdigit /= 1000;
	
	if(audio.tone)
		delete audio.tone;

	state.tone.exiting = false;
	state.tone.digits[0] = 0;
	while(len < 64 && NULL != (cp = getValue(NULL)))
	{
		snprintf(state.tone.digits + len, sizeof(state.tone.digits) - len, "%s", cp);		
		len = strlen(state.tone.digits);
	}

	audio.tone = new MFTones(state.tone.digits, level, getToneFraming(), interdigit);	
	setState(STATE_TONE);
	return false;
}


bool Methods::scrTonegen(void)
{
	Audio::Level level = 26000;
	const char *cp = getKeyword("level");
	const char *f1, *f2;

	state.timeout = getTimeoutKeyword("duration");
	state.tone.exiting = false;

	if(cp)
		level = atoi(cp);

	if(audio.tone)
	{
		delete audio.tone;
		audio.tone = NULL;
	}

	cp = getKeyword("frequency");
	f1 = getKeyword("freq1");
	f2 = getKeyword("freq2");

	if(!f1)
		f1 = "0";

	if(!f2)
		f2 = "0";

	if(cp)
		audio.tone = new AudioTone(atoi(cp), level, getToneFraming());
	else
		audio.tone = new AudioTone(atoi(f1), atoi(f2), level, level, getToneFraming());

	setState(STATE_TONE);	
	return false;
}

bool Methods::scrTone(void)
{
	Line *line = getLine();
	const char *name = getKeyword("name");
	const char *loc = getKeyword("location");
	const char *cp = getKeyword("level");
	TelTone::tonekey_t *key;
	Audio::Level level = 26000;

	if(!name)
		name = line->cmd;

	if(cp)
		level = atoi(cp);

	state.timeout = getTimeoutKeyword("timeout");
	state.tone.exiting = false;

	if(!loc)
		loc = runtime.getLast("location");

	if(audio.tone)
	{
		delete audio.tone;
		audio.tone = NULL;
	}

	if(!stricmp(name, "dialtone"))
		name = "dial";
	else if(!stricmp(name, "ringback"))
		name = "ring";
	else if(!stricmp(name, "ringtone"))
		name = "ring";
	else if(!stricmp(name, "busytone"))
		name = "busy";
	else if(!stricmp(name, "beep"))
		name = "record";
	else if(!stricmp(name, "callwait"))
		name = "waiting";
	else if(!stricmp(name, "callback"))
		name = "recall";

	key = TelTone::find(name, loc);
	if(!key)
	{
		error("invalid-tone");
		return true;
	}

	audio.tone = new TelTone(key, level, getToneFraming());
	setState(STATE_TONE);
	return false;
}

bool Methods::scrTimeslot(void)
{
	const char *cp;
	ScriptProperty *p = &typeTimeslot;
	Symbol *sym;

	while(NULL != (cp = getOption(NULL)))
	{
		sym = mapSymbol(cp, 16 + sizeof(p));
		if(!sym)
			continue;

		if(sym->type != ScriptSymbols::symINITIAL)
			continue;

		sym->type = ScriptSymbols::symPROPERTY;
		memcpy(sym->data, &p, sizeof(p));
		sym->data[sizeof(p)] = 0;
	}

	advance();
	return true;
}

bool Methods::scrPosition(void)
{
	const char *cp;
	ScriptProperty *p = &typePosition;
	Symbol *sym;

	while(NULL != (cp = getOption(NULL)))
	{
		sym = mapSymbol(cp, 13 + sizeof(p));
		if(!sym)
			continue;

		if(sym->type != ScriptSymbols::symINITIAL)
			continue;

		sym->type = ScriptSymbols::symPROPERTY;
		memcpy(sym->data, &p, sizeof(p));
		strcpy(sym->data + sizeof(p), "00:00:00.000");
	}

	advance();
	return true;
}

#ifdef	HAVE_LIBEXEC

bool Methods::scrLibexec(void)
{
	const char *cp = Process::getEnv("LIBEXEC");
	Symbol *sym = NULL;

	if(cp && !strnicmp(cp, "no", 2))
	{
		error("libexec-unsupported");
		return true;
	}

	cp = getKeyword("results");
	if(cp)
		sym = mapSymbol(cp);

	if(cp && !sym)
	{
		error("libexec-no-results");
		return true;
	}

	if(sym)
		Script::clear(sym);

	state.wait.interval = 250;
	cp = getKeyword("limit");
	if(!cp)
		cp = "0";
	state.wait.count = atoi(cp);
	state.wait.interval = getTimeoutKeyword("interval");

	if(!state.wait.interval || state.wait.interval == TIMEOUT_INF)
		state.wait.interval = 250;

	setState(STATE_LIBWAIT);
	return false;
}

#else

bool Methods::scrLibexec(void)
{
	error("libexec-unsupported");
	return true;
}

#endif

bool Methods::scrCleardigits(void)
{
	state.timeout = getTimeoutValue("0");

	if(state.timeout && !requiresDTMF())
	{
		*dtmf_digits = 0;
		return true;
	}

	if(!state.timeout)
	{
		*dtmf_digits = 0;
		advance();
		return true;
	}

	setState(STATE_CLEAR);
	return false;
}

bool Methods::scrVoicelib(void)
{
	const char *lib = getValue(NULL);
	BayonneTranslator *t;
	Line *line = getLine();

	t = BayonneTranslator::get(line->cmd);
	if(!t)
	{
		slog.error("language %s not loaded", line->cmd);
		error("voicelib-invalid");
		return true;
	}

	// we depend on compile-time check to validate voicelib command

	translator = t;
	voicelib = lib;
	advance();
	return true;
}

bool Methods::scrRoute(void)
{
	const char *cp = getMember();
	const char *opt;
	char evt[65];
	
	if(!cp)
		cp = "digits";

	setSymbol("script.error", "none");

	while(NULL != (opt = getValue(NULL)))
	{
		snprintf(evt, sizeof(evt), "%s:%s", cp, opt);
		if(digitEvent(evt))
		{
			snprintf(dtmf_digits, MAX_DTMF + 1, "%s", opt);			
			return false;
		}
	}

	snprintf(evt, sizeof(evt), "%s:invalid", cp);
	if(scriptEvent(evt))
		return false;
	
	error("route-invalid");
	return true;
}

bool Methods::scrSRoute(void)
{
	const char *cp = getMember();
	const char *opt;
	char evt[65];
	
	if(!cp)
		cp = "string";

	setSymbol("script.error", "none");

	while(NULL != (opt = getValue(NULL)))
	{
		snprintf(evt, sizeof(evt), "%s:%s", cp, opt);
		if(scriptEvent(evt))
			return false;
	}
	
	error("route-invalid");
	return true;
}
	
bool Methods::scrCollect(void)
{
        const char *cp = getMember();     

        if(!requiresDTMF())    
                return true;

	if(!cp)
		cp = "digits";

	state.input.route = cp;
	state.input.var = getOption(NULL);
        state.timeout = getTimeoutKeyword("timeout");
        state.input.interdigit = getTimeoutKeyword("interdigit");
	state.input.lastdigit = getTimeoutKeyword("lastdigit");
        state.input.format = getKeyword("format");
	state.input.ignore = getKeyword("ignore");

        if(state.timeout == TIMEOUT_INF)
                state.timeout = 6000;

        if(state.input.interdigit == TIMEOUT_INF)
                state.input.interdigit = state.timeout;

        if(state.input.lastdigit == TIMEOUT_INF)
                state.input.lastdigit = state.input.interdigit;   

        state.input.exit = getExitKeyword("#");

        cp = getKeyword("count");
        if(!cp)
                cp = "0";

      	state.input.size = state.input.count = atoi(cp);
        if(state.input.format)
                state.input.size = strlen(state.input.format);

        if(state.input.format && !state.input.count)
        {
                cp = state.input.format;
                while(*cp)
                {
                        if(*(cp++) == '?')
                                ++state.input.count;
                }
        }

	if(!state.input.count)
	{
		error("input-no-count");
		return true;
	}

        if(state.input.count > MAX_DTMF)
                state.input.count = MAX_DTMF;

	setState(STATE_COLLECT);
	return false;
}

bool Methods::scrInput(void)
{
	const char *cp;
	Symbol *sym;

	if(!requiresDTMF())
		return true;

	state.input.var = getOption(NULL);
	state.timeout = getTimeoutKeyword("timeout");
	state.input.interdigit = getTimeoutKeyword("interdigit");
	state.input.lastdigit = getTimeoutKeyword("lastdigit");
	state.input.format = getKeyword("format");

	if(state.timeout == TIMEOUT_INF)
		state.timeout = 6000;

	if(state.input.interdigit == TIMEOUT_INF)
		state.input.interdigit = state.timeout;

	state.input.exit = getExitKeyword("#");

	cp = getKeyword("count");
	if(!cp)
		cp = "0";

	state.input.size = state.input.count = atoi(cp);

	if(state.input.format)
		state.input.size = strlen(state.input.format);

	state.input.required = 0;
	if(state.input.format)
	{
		cp = state.input.format;
		while(*cp)
		{
			if(*(cp++) == '?')
				++state.input.required;
		}
		if(!state.input.count)
			state.input.count = state.input.required;
	}
		
	sym = mapSymbol(state.input.var, state.input.size);
	if(!sym)
	{
		error("input-no-symbol");
		return true;
	}
	if(!state.input.size)
		state.input.count = ScriptSymbols::storage(sym);

	if(!state.input.count)
		state.input.count = state.input.size;

	if(!state.input.count)
	{
		error("cannot-save-input");
		return true;
	}
	if(state.input.count > MAX_DTMF)
		state.input.count = MAX_DTMF;

	if(state.input.required > MAX_DTMF)
		state.input.required = MAX_DTMF;

	if(!state.input.required)
		state.input.required = state.input.count;

        if(state.input.lastdigit == TIMEOUT_INF)     
        {
                if(state.input.exit && *state.input.exit)
                        state.input.lastdigit = 800; 
                else
                        state.input.lastdigit = state.input.interdigit;
        }

	setState(STATE_INPUT);
	return false;	
}

bool Methods::scrRead(void)
{
	const char *cp;
	Symbol *sym;

	if(!requiresDTMF())
		return true;

	state.input.var = getOption(NULL);
	state.timeout = getTimeoutKeyword("timeout");
	state.input.interdigit = getTimeoutKeyword("interdigit");
	state.input.format = getKeyword("format");

	if(state.timeout == TIMEOUT_INF)
		state.timeout = 6000;

	if(state.input.interdigit == TIMEOUT_INF)
		state.input.interdigit = state.timeout;

	state.input.exit = getExitKeyword(NULL);

	cp = getKeyword("count");
	if(!cp)
		cp = "0";

	state.input.size = state.input.count = atoi(cp);

	if(state.input.format)
		state.input.size = strlen(state.input.format);

	state.input.required = 0;
	if(state.input.format)
	{
		cp = state.input.format;
		while(*cp)
		{
			if(*(cp++) == '?')
				++state.input.required;
		}
		if(!state.input.count)
			state.input.count = state.input.required;
	}
		
	sym = mapSymbol(state.input.var, state.input.size);
	if(!sym)
	{
		error("input-no-symbol");
		return true;
	}
	if(!state.input.size)
		state.input.count = ScriptSymbols::storage(sym);

	if(!state.input.count)
		state.input.count = state.input.size;

	if(!state.input.count)
	{
		error("cannot-save-input");
		return true;
	}
	if(state.input.count > MAX_DTMF)
		state.input.count = MAX_DTMF;

	if(state.input.required > MAX_DTMF)
		state.input.required = MAX_DTMF;

	if(!state.input.required)
		state.input.required = state.input.count;

	setState(STATE_READ);
	return false;	
}

bool Methods::scrKeyinput(void)
{

	if(!requiresDTMF())
		return true;

	state.inkey.menu = getKeyword("menu");
	state.inkey.var = getOption(NULL);
	state.timeout = getTimeoutKeyword("timeout");

	if(state.timeout == TIMEOUT_INF)
		state.timeout = 0;
		
	setState(STATE_INKEY);
	return false;
}

bool Methods::scrPickup(void)
{
	if(!offhook)
	{
		setState(STATE_PICKUP);
		return false;
	}
	advance();
	return true;
}

bool Methods::scrSync(void)
{
	time_t now;

	state.timeout = getTimeoutValue();

	time(&now);
	now -= starttime;
	if(now >= (time_t)(state.timeout / 1000))
		state.timeout = 0;
	else
		state.timeout -= (timeout_t)(now * 1000l);

	if(!state.timeout)
	{
		advance();
		return true;
	}
	setState(STATE_SLEEP);
	return false;
}	

bool Methods::scrWaitkey(void)
{
	state.timeout = getTimeoutValue();
        if(!state.timeout || *dtmf_digits)
        {
                advance();
                return true;
        } 
	setState(STATE_WAITKEY);
	return false;
}

bool Methods::scrSleep(void)
{
	state.timeout = getTimeoutValue();

	if(!state.timeout)
	{
		advance();
		return true;
	}

	setState(STATE_SLEEP);
	return false;
}

bool Methods::scrAnswer(void)
{
	setState(STATE_ANSWER);
	return false;
}

bool Methods::scrHangup(void)
{
	setState(STATE_HANGUP);
	return false;
}	

bool Methods::scrErase(void)
{
	const char *cp = getAudio(false);
	
	if(cp)
	{
		slog.error("%s: %s", logname, cp);
		error("invalid-erase");
		return true;
	}

	cp = getWritepath();
	if(!cp)
	{
		error("erase-invalid-file");
		return true;
	}
	remove(cp);
	advance();
	return true;
}	

#ifndef	WIN32

bool Methods::scrLink(void)
{
	const char *cp = getAudio(false);
	const char *dp;
	const char *se, *de;
	char buf1[MAX_PATHNAME], buf2[MAX_PATHNAME];
	
	if(cp)
	{
		slog.error("%s: %s", logname, cp);
		error("invalid-link");
		return true;
	}

	cp = getWritepath(buf1, sizeof(buf1));
	if(!cp)
	{
		error("invalid-source");
		return true;
	}

	dp = getWritepath(buf2, sizeof(buf2));	
	if(!cp)
	{
		error("invalid-target");
		return true;
	}

	se = strrchr(cp, '.');
	de = strrchr(dp, '.');

	if(!se || !de)
	{
		error("invalid-names");
		return true;
	}

	if(stricmp(se, de))
	{
		error("filename-mismatch");
		return true;
	}

	if(link(cp, dp))
	{
		error("link-failed");
		return true;
	}

	advance();
	return true;
}	

#endif

bool Methods::scrMove(void)
{
	const char *cp = getAudio(false);
	const char *dp;
	const char *se, *de;
	char buf1[MAX_PATHNAME], buf2[MAX_PATHNAME];
	
	if(cp)
	{
		slog.error("%s: %s", logname, cp);
		error("invalid-move");
		return true;
	}

	cp = getWritepath(buf1, sizeof(buf1));
	if(!cp)
	{
		error("invalid-source");
		return true;
	}

	dp = getWritepath(buf2, sizeof(buf2));	
	if(!cp)
	{
		error("invalid-target");
		return true;
	}

	se = strrchr(cp, '.');
	de = strrchr(dp, '.');

	if(!se || !de)
	{
		error("invalid-names");
		return true;
	}

	if(stricmp(se, de))
	{
		error("filename-mismatch");
		return true;
	}

	if(rename(cp, dp))
	{
		error("move-failed");
		return true;
	}

	advance();
	return true;
}	

bool Methods::scrWrite(void)
{
	char buf[MAX_PATHNAME];
	const char *cp = getWritepath(buf, sizeof(buf));

	if(!cp)
	{
		error("invalid-file");
		return true;
	}
	
	new WriteThread(dynamic_cast<ScriptInterp *>(this), cp);
	return false;
}	

bool Methods::scrCopy(void)
{
	const char *cp = getAudio(false);
	const char *dp;
	const char *se, *de;
	char buf1[MAX_PATHNAME], buf2[MAX_PATHNAME];
	
	if(cp)
	{
		slog.error("%s: %s", logname, cp);
		error("invalid-move");
		return true;
	}

	cp = getWritepath(buf1, sizeof(buf1));
	if(!cp)
	{
		error("invalid-source");
		return true;
	}

	dp = getWritepath(buf2, sizeof(buf2));	
	if(!cp)
	{
		error("invalid-target");
		return true;
	}

	se = strrchr(cp, '.');
	de = strrchr(dp, '.');

	if(!se || !de)
	{
		error("invalid-names");
		return true;
	}

	if(stricmp(se, de))
	{
		error("filename-mismatch");
		return true;
	}
	new CopyThread(dynamic_cast<ScriptInterp*>(this), cp, dp);
	return false;
}	

bool Methods::scrBuild(void)
{
	const char *cp, *dp;
	Audio::Info info;
	char buf[MAX_PATHNAME];

	cp = getAudio();
        if(cp)
        {
                slog.error("%s: %s", logname, cp);
                if(!signalScript(SIGNAL_FAIL))
                        error("invalid-audio");      
                return true;
        }

	dp = getWritepath(buf, sizeof(buf));
	if(!dp)
	{
		error("missing target");
		return true;
	}
        cp = audio.translator->speak(this);
        if(cp)
        {
                slog.error("%s: %s", logname, cp);
                error("invalid-translation");
                return true;
        }

	info.framing = audio.framing;
	info.encoding = audio.encoding;
	info.annotation = (char *)getKeyword("note");
	info.rate = Audio::getRate(audio.encoding);

	if(info.rate == Audio::rateUnknown)
		info.rate = Audio::rate8khz;

	new BuildThread(dynamic_cast<ScriptInterp *>(this), &audio, &info, dp, state.audio.list);
	return false;
}

bool Methods::scrPrompt(void)
{
	const char *cp = getMember();

	if(cp && stricmp(translator->getId(), cp))
	{
		advance();
		return true;
	}

	if(state.menu && *dtmf_digits)
	{
		advance();
		return true;
	}

        memset(&state.audio, 0, sizeof(state.audio));
	audio.offset = NULL;

	if(state.menu && !requiresDTMF())
		return true;

	if(state.menu)
		state.audio.exitkey = true;
	else
		state.audio.exitkey = false;

	state.audio.mode = Audio::modeReadAny;

	cp = getAudio();
        if(cp)
        {
                slog.error("%s: %s", logname, cp);
                if(!signalScript(SIGNAL_FAIL))
                        error("invalid-audio");
                return true;
        }

        cp = audio.translator->speak(this);
        if(cp)
        {
                slog.error("%s: %s", logname, cp);
                error("invalid-translation");
                return true;
        }

	setState(STATE_PLAY);
	return false;
}

bool Methods::scrReplay(void)
{
	const char *cp;
        memset(&state.audio, 0, sizeof(state.audio));

        state.audio.exitkey = false;
        if(state.menu)
                state.audio.exitkey = true;

        state.audio.exit = getExitKeyword(NULL);
	state.audio.menu = getMenuKeyword(NULL);

        cp = getAudio();
        if(cp)
        {
                slog.error("%s: %s", logname, cp);
                if(!signalScript(SIGNAL_FAIL))
                        error("invalid-audio");
                return true;
        }

        state.audio.list[0] = getValue(NULL);
	state.audio.list[1] = NULL;
	state.audio.mode = Audio::modeRead;
	setState(STATE_PLAY);
	return false;
}

bool Methods::scrRecord(void)
{
	const char *cp = getMember();

	memset(&state.audio, 0, sizeof(state.audio));

	state.audio.exitkey = false;
	if(state.menu)
		state.audio.exitkey = true;

	if(cp)
		state.audio.compress = true;

	state.audio.exit = getExitKeyword(NULL);
	state.audio.menu = getMenuKeyword(NULL);
	state.audio.total = getTimeoutKeyword("timeout");
	state.audio.silence = getTimeoutKeyword("silence");
	state.audio.intersilence = getTimeoutKeyword("intersilence");
	state.audio.note = getKeyword("note");

	if(state.audio.total == TIMEOUT_INF)
		state.audio.total = 60000;

	if(state.audio.silence == TIMEOUT_INF)
		state.audio.silence = 0;

	if(state.audio.intersilence == TIMEOUT_INF)
		state.audio.intersilence = state.audio.silence;

	cp = getAudio();
	if(cp)
	{
                slog.error("%s: %s", logname, cp);
                if(!signalScript(SIGNAL_FAIL))
                        error("invalid-audio");
                return true;
        }

	state.audio.list[0] = getValue(NULL);
	state.audio.list[1] = getValue(NULL);
	state.audio.list[2] = NULL;

	state.audio.mode = Audio::modeCreate;
	if(audio.offset)
		state.audio.mode = Audio::modeWrite;

	setState(STATE_RECORD);
	return false;
}

bool Methods::scrAppend(void)
{
	const char *cp = getMember();

	memset(&state.audio, 0, sizeof(state.audio));

	state.audio.exitkey = false;
	if(state.menu)
		state.audio.exitkey = true;

	if(cp)
		state.audio.compress = true;

	audio.offset = NULL;
	state.audio.exit = getExitKeyword(NULL);
	state.audio.menu = getMenuKeyword(NULL);
	state.audio.total = getTimeoutKeyword("timeout");
	state.audio.silence = getTimeoutKeyword("silence");
	state.audio.intersilence = getTimeoutKeyword("intersilence");

	if(state.audio.total == TIMEOUT_INF)
		state.audio.total = 60000;

	if(state.audio.silence == TIMEOUT_INF)
		state.audio.silence = 0;

	if(state.audio.intersilence == TIMEOUT_INF)
		state.audio.intersilence = state.audio.silence;

	cp = getAudio();
	if(cp)
	{
                slog.error("%s: %s", logname, cp);
                if(!signalScript(SIGNAL_FAIL))
                        error("invalid-audio");
                return true;
        }

	state.audio.list[0] = getValue(NULL);
	state.audio.list[1] = getValue(NULL);
	state.audio.list[2] = NULL;
	state.audio.mode = Audio::modeAppend;

	setState(STATE_RECORD);
	return false;
}

bool Methods::scrPlay(void)
{
	const char *cp = getMember();

        if(cp && stricmp(translator->getId(), cp))
        {
                advance();
                return true;
        } 

        memset(&state.audio, 0, sizeof(state.audio));
	audio.offset = NULL;
	state.audio.exitkey = false;
	state.audio.mode = Audio::modeReadAny;

	cp = getAudio();
        if(cp)
        {
                slog.error("%s: %s", logname, cp);
                if(!signalScript(SIGNAL_FAIL))
                        error("invalid-audio");
                return true;
        }

        cp = audio.translator->speak(this);
        if(cp)
        {
                slog.error("%s: %s", logname, cp);
                error("invalid-translation");
                return true;
        }

	setState(STATE_PLAY);
	return false;
}

bool Methods::scrPath(void)
{
	char fbuf[64];
	const char *ep;
	const char *prefix = getKeyword("prefix");
	const char *cp = getAudio();
	const char *ext;
	Symbol *sym;

	if(prefix && !*prefix)
		prefix = NULL;

	if(cp)
	{
                slog.error("%s: %s", logname, cp);
                if(!signalScript(SIGNAL_FAIL))
                        error("invalid-audio");
                return true;
        } 

	cp = getOption(NULL);
	if(!cp)
	{
nosym:
		error("no-symbol");
		return true;
	}
	sym = mapSymbol(cp, sizeof(fbuf));
	if(!sym)
		goto nosym;

	cp = getValue(NULL);
	if(!cp)
	{
		error("no-path");
		return true;
	}
	ext = strrchr(cp, '/');
	if(ext)
		ext = strrchr(ext, '.');
	else
		ext = strchr(cp, '.');
	if(ext)
		ext = "";
	else
	{
		ext = getKeyword("extension");
		if(!ext || !*ext)
			ext = audio.libext;
	}

	if(!strchr(cp, '/') && !strchr(cp, ':') && prefix)
	{
		ep = prefix + strlen(prefix) - 1;
		if(*ep == '/' || *ep == ':')
			snprintf(fbuf, sizeof(fbuf), "%s%s%s", prefix, cp, ext);
		else
			snprintf(fbuf, sizeof(fbuf), "%s/%s%s", prefix, cp, ext);
		goto finish;
	}

	snprintf(fbuf, sizeof(fbuf), "%s%s", cp, ext);
finish:
	Bayonne::commit(sym, fbuf);
	advance();
	return true;
}

bool Methods::scrReadpath(void)
{
	char fbuf[MAX_PATHNAME];
	const char *cp;
	Symbol *sym;

	cp = getOption(NULL);
	if(!cp)
		goto nosym;

	sym = mapSymbol(cp, sizeof(fbuf));

	if(!sym)
	{
nosym:
		error("no-symbol");
		return true;
	}

	Bayonne::commit(sym, "");
	cp = getAudio();
	if(cp)
	{
                slog.error("%s: %s", logname, cp);
                if(!signalScript(SIGNAL_FAIL))
                        error("invalid-audio");
		return true;
	}
	
	
	cp = getValue(NULL);
	if(!cp || !*cp)
		goto error;
	cp = audio.getFilename(cp);
	if(!cp)
	{
error:
		error("invalid-path");
		return true;
	}
	if(*cp != '/' && cp[1] != ':')
	{
		snprintf(fbuf, sizeof(fbuf), "%s/%s", keypaths.getLast("datafiles"), cp);
		cp = fbuf;
	}
	Bayonne::commit(sym, cp);
	advance();
	return true;
}

bool Methods::scrList(void)
{
	const char *cp;
	char buf[MAX_PATHNAME];
	Symbol *sym = mapSymbol(getOption(NULL), sizeof(buf));
	unsigned len = 0;
		
	if(!sym)
	{
		error("no-symbol");
		return true;
	}

	setString(buf, sizeof(buf), keypaths.getLast("datafiles"));
	len = strlen(buf);
	cp = getKeyword("prefix");
	if(!cp)
		cp = getValue(NULL);
	while(cp && len < sizeof(buf) - 2)
	{
		buf[len++] = '/';
		setString(buf + len, sizeof(buf) - len, cp);
		len = strlen(buf);
		cp = getValue(NULL);
	}

	Bayonne::clear(sym);

	new ListThread(dynamic_cast<ScriptInterp*>(this), buf, sym);
	return false;
}

bool Methods::scrWritepath(void)
{
	char buf[MAX_PATHNAME];
	char fbuf[MAX_PATHNAME];
	const char *cp;
	Symbol *sym;

	cp = getOption(NULL);
	if(!cp)
		goto nosym;
	sym = mapSymbol(cp, sizeof(buf));

	if(!sym)
	{
nosym:
		error("no-symbol");
		return true;
	}

	Bayonne::commit(sym, "");
	cp = getAudio();
	if(cp)
	{
                slog.error("%s: %s", logname, cp);
                if(!signalScript(SIGNAL_FAIL))
                        error("invalid-audio");
		return true;
	}
	
	cp = getWritepath(buf, sizeof(buf));
	if(!cp)
	{
		error("invalid-path");
		return true;
	}
	if(*cp != '/' && cp[1] != ':')
	{
		snprintf(fbuf, sizeof(fbuf), "%s/%s", keypaths.getLast("datafiles"), cp);
		cp = fbuf;
	}
	Bayonne::commit(sym, cp);
	advance();
	return true;
}

	
bool Methods::scrPathname(void)
{
	unsigned count = 0;
	const char *cp = getAudio();
	const char *path;
	FILE *fp;

	if(cp)
	{
		slog.error("%s: %s", logname, cp);
		if(!signalScript(SIGNAL_FAIL))
			error("invalid-audio");
		return true;
	}

	cp = audio.translator->speak(this);
	if(cp)
	{
		slog.error("%s: %s", logname, cp);
		error("invalid-translation");
		return true;
	}	

	fp = fopen(server->getLast("output"), "a");
	serialize.enter();
	while(state.audio.list[count])
	{
		cp = state.audio.list[count++];
		path = audio.getFilename(cp, false);
		if(path)
		{
			printf("%s = %s\n", cp, path);
			fprintf(fp, "%s = %s\n", cp, path);
		}
		else
		{
			printf("%s = invalid\n", cp);
			fprintf(fp, "%s = invalid\n", cp);
		}
	}
	serialize.leave();
	fclose(fp);
	advance();
	return true;
}
		
bool Methods::scrEcho(void)
{
	const char *val;
	FILE *fp = fopen(server->getLast("output"), "a");
	serialize.enter();

	if(fp)
	{
		while(NULL != (val = getValue(NULL)))
		{
			fputs(val, fp);
			cout << val;
		}
	
		fputc('\n', fp);
	}
	cout << endl;
	serialize.leave();
	if(fp)
		fclose(fp);
	advance();
	return true;
}

} // end namespace

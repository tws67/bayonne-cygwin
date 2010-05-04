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

#define	BUILD_LIBEXEC
#include "engine.h"

#ifdef HAVE_LIBEXEC
#include <cc++/process.h>
#include <cc++/slog.h>
#include <cc++/export.h>
#include "libexec.h"

using namespace ost;
using namespace std;

void BayonneTSession::sysHangup(const char *tsid)
{
	Event event;

	memset(&event, 0, sizeof(event));
	event.id = DROP_LIBEXEC;
	event.libexec.tid = tsid;
	if(!postEvent(&event))
		slog.error("libexec hangup failed; tsid=%s", tsid);
}	

void BayonneTSession::sysPost(const char *sid, char *id, const char *value)
{
        Event event;

        char buf[128];
        char *up;

        enter();
        if(stricmp(var_sid, sid))
        {
                slog.error("libexec call id %s invalid", sid);
                leave();
                return;
        }
        snprintf(buf, sizeof(buf), "posted:%s", id);
        up = buf;  
        while(*up)
        {
                if(*up == '_')
                        *up = '.';
                ++up;
        }
        setSymbol(buf + 7, value);
        memset(&event, 0, sizeof(event));
        event.id = POST_LIBEXEC;
        event.name = buf;  
	postEvent(&event);
	leave();
}

void BayonneTSession::sysVar(const char *tsid, char *id, const char *value, int size)
{
        char buf[512];
        const char *cp;
        char *up;  

        enter();
        if(isLibexec(tsid))
        {  
		libWrite("400 QUERY\n");
		while(NULL != (up = strchr(id, '_')))
			*up = '.';
		if(value && size < 0)
			catSymbol(id, value);
		else if(value)
			setSymbol(id, value, size);
		cp = getSymbol(id);
		snprintf(buf, sizeof(buf), "%s: ", id);
		up = buf;
		while(*up)
		{
			*up = toupper(*up);
			if(*up == '.')
				*up = '_';
			++up;
		}
		libWrite(buf);
		if(cp)
		{
			state.result = RESULT_SUCCESS;
			libWrite(cp);
		}
		else
			state.result = RESULT_INVALID;
		snprintf(buf, sizeof(buf), "\nRESULT: %d\n\n", 
			state.result);
		state.result = RESULT_SUCCESS;
		libWrite(buf);
	}
	leave();
}

void BayonneTSession::sysInput(const char *tsid, char *tok)
{
	Event event;
	const char *exitkeys = "#";
	unsigned count = 1;
	timeout_t timeout;
	const char *cp;

	cp = strtok_r(NULL, " \r\n\t", &tok);
	if(!cp)
		cp = "6";

	timeout = atol(cp);
	if(timeout < 250)
		timeout *= 1000;

	cp = strtok_r(NULL, " \t\r\n", &tok);
	if(cp)
		count = atoi(cp);
	else
		exitkeys = NULL;

	enter();
	if(isLibexec(tsid))
	{
		memset(&event, 0, sizeof(event));
		event.id = READ_LIBEXEC;
		event.libexec.tid = tsid;
		memset(&state.input, 0, sizeof(state.input));
		state.timeout = timeout;
		state.input.interdigit = timeout;
		if(exitkeys && *exitkeys)
			state.input.lastdigit = 800;
		else
			state.input.lastdigit = timeout;
		state.input.count = count;
		state.input.exit = exitkeys;		
		postEvent(&event);
	}
	leave();
}

void BayonneTSession::sysXfer(const char *tsid, const char *id)
{
	Event event;
	const char *pre;
	const char *svr;

	memset(&event, 0, sizeof(event));
	event.id = XFER_LIBEXEC;
	event.libexec.tid = tsid;
	enter();

	if(isLibexec(tsid))
	{
		switch(iface)
		{
		case IF_INET:
			pre = driver->getLast("urlprefix");
			if(!pre)
				pre = "";
			svr = driver->getLast("outbound");
			if(strchr(id, '@'))
				svr = NULL;
			state.url.ref = id;
			if(svr)
			{
				snprintf(state.url.buf, sizeof(state.url.buf), "%s%s@%s", pre, svr, id);
				state.url.ref = state.url.buf;
			}
			break;
		case IF_PSTN:
			pre = driver->getLast("prefix");
			if(!pre)
				pre = "!";

			state.tone.exiting = true;
			if(audio.tone)
				delete audio.tone;
			snprintf(state.tone.digits, sizeof(state.tone.digits), "%s%s", pre, id);
			audio.tone = new DTMFTones(state.tone.digits, 20000, getToneFraming());
			event.id = TONE_LIBEXEC;
			break;
		default:
			state.result = RESULT_INVALID;
			event.id = RESTART_LIBEXEC;
		}
		postEvent(&event);
	}
	leave();
}

void BayonneTSession::sysReplay(const char *tsid, char *tok)
{
	Event event;
	const char *file, *offset;

	memset(&event, 0, sizeof(event));
	event.id = REPLAY_LIBEXEC;
	event.libexec.tid = tsid;
	file = strtok_r(NULL, " \t\r\n", &tok);
	offset = strtok_r(NULL, " \t\r\n", &tok);
	enter();
	if(isLibexec(tsid))
	{
                *dtmf_digits = 0;
                digit_count = 0;
                memset(&state.audio, 0, sizeof(state.audio));
                if(getAudio())
                {    
                        state.result = RESULT_INVALID;
			event.id = RESTART_LIBEXEC;
                        goto post;
                } 

		if(offset)
		{
                	setString(state.libaudio->text, 16, offset);
                        audio.offset = state.libaudio->text;
                }
                else
                        audio.offset = NULL;

                state.audio.exitkey = true;
                state.audio.exit = NULL;
                state.audio.mode = Audio::modeRead;
                snprintf(state.libaudio->text + 16, MAX_LIBINPUT - 16, file);
                state.audio.list[0] = state.libaudio->text + 16;
post:
		postEvent(&event);
	}
	leave();
}

void BayonneTSession::sysPrompt(const char *tsid, const char *voice, const char *text)
{
	Event event;
	const char *cp;
	char bts[8];
	char *p, *tok;
	unsigned count = 0;
	char buf[80];

	memset(&event, 0, sizeof(event));
	event.id = PROMPT_LIBEXEC;
	event.libexec.tid = tsid;
	
	if(!strchr(voice, '/'))
		voice = NULL;

	enter();
	if(isLibexec(tsid))
	{
		if(*dtmf_digits)
		{
			snprintf(buf, sizeof(buf), "100 TRANSACTION\nRESULT: %d\n\n", RESULT_PENDING);
			libWrite(buf);
			goto exit;
		}
		memset(&state.audio, 0, sizeof(state.audio));
		if(getAudio())
		{
			slog.error("%s: %s", logname, "format invalid");
			state.result = RESULT_INVALID;
			event.id = RESTART_LIBEXEC;
			goto post;
		}
                state.audio.exitkey = true;
                state.audio.exit = NULL;
                state.audio.mode = Audio::modeRead;
		audio.offset = NULL;
		if(voice)
		{
			snprintf(bts, sizeof(bts), "%s", voice);
			p = strchr(bts, '/');
			if(p)
				*p = 0;
			audio.translator = BayonneTranslator::get(bts);
			if(!audio.translator)
			{
invalid:
				event.id = RESTART_LIBEXEC;
				state.result = RESULT_INVALID;
				goto post;
			}
			cp = audio.getVoicelib(voice);
			if(!cp)
				goto invalid;			
		}
		setString(state.libaudio->text, MAX_LIBINPUT, text);
		cp = strtok_r(state.libaudio->text, " \t\r\n", &tok);
		while(cp && count < (MAX_LIBINPUT / 2 - 1))
		{
			state.libaudio->list[count++] = cp;
			cp = strtok_r(NULL, " \t\r\n", &tok);
		}
		state.libaudio->list[count] = NULL;
                state.libaudio->line.argc = count;
                state.libaudio->line.args = state.libaudio->list;
                state.libaudio->line.cmd = "prompt";   
		cp = audio.translator->speak(this, &state.libaudio->line);
                if(cp)
                	goto invalid;  
post:
		postEvent(&event);
	}
exit:
	leave();
}

void BayonneTSession::sysTone(const char *tsid, char *tok)
{
	Event event;
	const char *loc, *tone, *cp;
	char *p;
	timeout_t timeout = TIMEOUT_INF;
	Audio::Level level = 26000;
	TelTone::tonekey_t *key;

	memset(&event, 0, sizeof(event));
	event.id = TONE_LIBEXEC;
	event.libexec.tid = tsid;
	tone = strtok_r(NULL, " \t\r\n", &tok);
	if(!tone)
		return;
	p = strchr(tone, '/');	
	if(p)
	{
		*(p++) = 0;
		loc = tone;
		tone = p;
	}
	else
		loc = server->getLast("location");
	cp = strtok_r(NULL, " \t\r\n", &tok);
	if(cp)
		timeout = atol(cp);
	cp = strtok_r(NULL, " \t\r\n", &tok);
	if(cp && atoi(cp) > 0)
		level = atoi(cp);
	if(!timeout)
		timeout = TIMEOUT_INF;
	else if(timeout < 100)
		timeout *= 1000;
	if(!stricmp(tone, "dialtone"))
		tone = "dial";
	else if(!stricmp(tone, "ringback"))
		tone = "ring";
	else if(!stricmp(tone, "ringtone"))
		tone = "ring";
	else if(!stricmp(tone, "busytone"))
		tone = "busy";
	else if(!stricmp(tone, "beep"))
		tone = "record";
	else if(!stricmp(tone, "callwait"))
		tone = "waiting";
	else if(!stricmp(tone, "callback"))
		tone = "recall";
	
	key = TelTone::find(tone, loc);
	enter();
	if(isLibexec(tsid))
	{
		if(audio.tone)
		{
			delete audio.tone;
			audio.tone = NULL;
		}
		if(!key)
		{
			state.result = RESULT_INVALID;
			event.id = RESTART_LIBEXEC;
			goto post;
		}
		audio.tone = new TelTone(key, level, getToneFraming());
		state.timeout = timeout;
		state.tone.exiting = false;
		
post:
		postEvent(&event);
	}
	leave();
}	

void BayonneTSession::sysSTone(const char *tsid, char *tok)
{
	Event event;
	unsigned f1 = 0;
	const char *cp;
	timeout_t timeout = TIMEOUT_INF;
	Audio::Level level = 26000;

	memset(&event, 0, sizeof(event));
	event.id = TONE_LIBEXEC;
	event.libexec.tid = tsid;

	cp = strtok_r(NULL, " \t\r\n", &tok);
	if(cp)
		f1 = atoi(cp);
	cp = strtok_r(NULL, " \t\r\n", &tok);
	if(cp)
		timeout = atol(cp);
	cp = strtok_r(NULL, " \t\r\n", &tok);
	if(cp && atoi(cp) > 0)
		level = atoi(cp);
	if(!timeout)
		timeout = TIMEOUT_INF;
	else if(timeout < 10)
		timeout *= 1000;
	
	enter();
	if(isLibexec(tsid))
	{
		if(audio.tone)
		{
			delete audio.tone;
			audio.tone = NULL;
		}
		audio.tone = new AudioTone(f1, level, getToneFraming());
		state.timeout = timeout;
		state.tone.exiting = false;
		postEvent(&event);
	}
	leave();
}	

void BayonneTSession::sysDTone(const char *tsid, char *tok)
{
	Event event;
	unsigned f1 = 0, f2 = 0;
	const char *cp;
	timeout_t timeout = TIMEOUT_INF;
	Audio::Level level = 26000;

	memset(&event, 0, sizeof(event));
	event.id = TONE_LIBEXEC;
	event.libexec.tid = tsid;

	cp = strtok_r(NULL, " \t\r\n", &tok);
	if(cp)
		f1 = atoi(cp);
	cp = strtok_r(NULL, " \t\r\n", &tok);
	if(cp)
		f2 = atoi(cp);
	cp = strtok_r(NULL, " \t\r\n", &tok);
	if(cp)
		timeout = atol(cp);
	cp = strtok_r(NULL, " \t\r\n", &tok);
	if(cp && atoi(cp) > 0)
		level = atoi(cp);
	if(!timeout)
		timeout = TIMEOUT_INF;
	else if(timeout < 10)
		timeout *= 1000;
	
	enter();
	if(isLibexec(tsid))
	{
		if(audio.tone)
		{
			delete audio.tone;
			audio.tone = NULL;
		}
		audio.tone = new AudioTone(f1, f2, level, level, getToneFraming());
		state.timeout = timeout;
		state.tone.exiting = false;
		postEvent(&event);
	}
	leave();
}	

void BayonneTSession::sysRecord(const char *tsid, char *tok)
{
	Event event;
	const char *file, *offset, *cp;
	timeout_t timeout, silence;

	memset(&event, 0, sizeof(event));
        event.id = RECORD_LIBEXEC;
        event.libexec.tid = tsid;
        file = strtok_r(NULL, " \t\r\n", &tok);
        cp = strtok_r(NULL, " \t\r\n", &tok);
        if(!cp)
	        cp = "30";

	timeout = atol(cp);
	if(timeout < 1000)
		timeout *= 1000;

        cp = strtok_r(NULL, " \t\r\n", &tok);
        if(!cp)
                 cp = "0";
        silence = atol(cp); 
	offset = strtok_r(NULL, " \t\r\n", &tok);

	enter();
	if(isLibexec(tsid))
	{
                *dtmf_digits = 0;
                digit_count = 0;
		memset(&event, 0, sizeof(event));
                memset(&state.audio, 0, sizeof(state.audio));
                if(getAudio())
                {   
                        slog.error("%s: %s", logname, cp);
                        state.result = RESULT_INVALID;       
                        event.id = ENTER_LIBEXEC;
			goto post;
                }  
                if(offset)
                {
        		setString(state.libaudio->text, 16, offset);
                        audio.offset = state.libaudio->text;
                        state.audio.mode = Audio::modeWrite;
                }
                else
                {
                        audio.offset = NULL;
                        state.audio.mode = Audio::modeCreate;
                } 

                state.audio.exitkey = true;
                state.audio.compress = false;
                state.audio.exit = NULL;
                state.audio.total = timeout;
                state.audio.silence = silence;
                state.audio.intersilence = silence;
                state.audio.note = NULL; 

		setString(state.libaudio->text + 16, MAX_LIBINPUT - 16, file);
		state.audio.list[0] = state.libaudio->text + 16;
		state.audio.list[1] = NULL;
post:
		postEvent(&event);
	}
	leave();
}

void BayonneTSession::sysArgs(const char *tsid)
{
	char buf[64];
	const char *cp;
	char *up;
	unsigned opt = 0;
	Line *line = getLine();
	const char *id, *value;

	enter();
	if(isLibexec(tsid))
	{  
		libWrite("300 ARGUMENTS\n");
		frame[stack].index = 0;
		if(strnicmp(line->cmd, "exec", 4))
			cp = getOption(NULL);
		while(NULL != (cp = getOption(NULL)))
		{
			if(*cp == '%' || *cp == '&')
				snprintf(buf, sizeof(buf), "%s: ", cp + 1);
			else
				snprintf(buf, sizeof(buf), "ARG_%d: ", ++opt);
			up = buf;
			while(*up)
			{
				*up = toupper(*up);
				if(*up == '.')
					*up = '_';
				++up;
			}
			if(*cp == '&')
				cp = getSymbol(++cp);
			else
				cp = getContent(cp);
			if(cp)
			{
				libWrite(buf);
				if(*cp)
					libWrite(cp);
				libWrite("\n");
			}
		}
		while(line->next && !stricmp(line->next->cmd, "param"))
		{
			skip();
			line = getLine();
			opt = 0;
			while(opt < line->argc)
			{
				id = line->args[opt++];
				if(*id != '=')
					continue;
				++id;
				value = getContent(line->args[opt++]);
				if(!*id || !value)
					continue;
				snprintf(buf, sizeof(buf), "PARAM_%s: %s\n", id, value);
				up = buf;
				while(*up && *up != ':')
				{
					*up = toupper(*up);
					++up;
				}
				libWrite(buf);
			}
		}
		libWrite("\n");
	}	
	leave();
}

void BayonneTSession::sysWait(const char *tsid, char *tok)
{
	Event event;
	const char *cp;
	timeout_t timeout;
	char buf[80];

	cp = strtok_r(NULL, " \t\r\n", &tok);
	if(!cp)
		cp = "6";
	timeout = atol(cp);
	if(timeout < 250)
		timeout *= 1000;	

	enter();
	if(isLibexec(tsid))
	{
		if(*dtmf_digits)
		{
			snprintf(buf, sizeof(buf), 
				"100 TRANSACTION\nRESULT: %d\n\n", RESULT_PENDING);
			libWrite(buf);
			goto exit;
		}
		if(!timeout)
		{
			libWrite("100 TRANSACTION\nRESULT: 0\n\n");
			goto exit;
		}
		memset(&event, 0, sizeof(event));
		event.id = WAIT_LIBEXEC;
		event.libexec.tid = tsid;
		state.timeout = timeout;
		postEvent(&event);
	}
exit:
	leave();
}

void BayonneTSession::sysError(const char *tsid, char *tok)
{
	Event event;
	const char *errmsg = strtok_r(NULL, "\r\n", &tok);
	event.id = ERROR_LIBEXEC;
	event.liberror.tid = tsid;
	event.liberror.errmsg = errmsg;
	postEvent(&event);
}		

void BayonneTSession::sysExit(const char *tsid, char *tok)
{
	Event event;
	char *code = strtok_r(NULL, " \t\r\n", &tok);
	if(!code)
		code = "255";
	
	event.id = EXIT_LIBEXEC;
	event.libexec.tid = tsid;
	event.libexec.result = atoi(code);
	if(event.libexec.result < -255)
		event.libexec.result = 255;
	else if(event.libexec.result < 0)
		event.libexec.result += 256;
	else if(event.libexec.result > 255)
		event.libexec.result = 255;
	if(!postEvent(&event))
		slog.error("libexec exit failed; tsid=%s", tsid);
}
		
void BayonneTSession::sysFlush(const char *tsid)
{
	enter();
	if(isLibexec(tsid))
	{
		*dtmf_digits = 0;
		digit_count = 0;
		state.result = RESULT_SUCCESS;
		libWrite("100 TRANSACTION\nRESULT: 0\n\n");
	}
	leave();
}

void BayonneTSession::sysReturn(const char *tsid, const char *text)
{
	Symbol *sym = NULL;
	const char *cp;
	char pack[2] = ",";
	char buf[100];

	enter();
	if(isLibexec(tsid))
	{
		cp = getKeyword("token");
		if(cp)
			pack[0] = *cp;
		cp = getKeyword("results");
		if(cp)	
			sym = mapSymbol(cp);
		if(sym)
		{
			switch(sym->type)
			{
			case symPROPERTY:
			case symNUMBER:
			case symCOUNTER:
				Script::commit(sym, text);
				break;
			case symNORMAL:
			case symINITIAL:
				if(sym->data[0])
					Script::append(sym, pack);
				sym->type = symNORMAL;
			default:
				Script::append(sym, text);			
			}	
			state.result = RESULT_SUCCESS;
		}
		else
			state.result = RESULT_FAILED;
		snprintf(buf, sizeof(buf), "100 TRANSACTION\nRESULT: %d\n\n", state.result);
		libWrite(buf);		
	}
	leave();
}

void BayonneTSession::sysHeader(const char *tsid)
{
	BayonneTranslator *t;
	char buf[512];
	const char *cp;
	unsigned len;

	enter();
	if(isLibexec(tsid))
	{
		getAudio();
		libWrite("200 HEADER\n");
		snprintf(buf, sizeof(buf), "LANGUAGES: NONE");
		len = strlen(buf);
		t = BayonneTranslator::getFirst();
		while(t && len < 500)
		{
			cp = t->getId();
			if(stricmp(cp, "none"))
			{
				snprintf(buf + len, sizeof(buf) - len, " %s", cp); 
				len += strlen(cp) + 1;
			}
			t = t->getNext();
		}		
		buf[len++] = '\n';
		buf[len] = 0;
		libWrite(buf);
		snprintf(buf, sizeof(buf), "SESSION: %s\n", var_sid);
		libWrite(buf);
		snprintf(buf, sizeof(buf), "TIMEOUT: %ld\n", 
			(long)getLibexecTimeout() / 1000);
		libWrite(buf);
		if(*dtmf_digits)
		{
			snprintf(buf, sizeof(buf), "DIGITS: %s\n", dtmf_digits);
			libWrite(buf);
			*dtmf_digits = 0;
			digit_count = 0;
		}
		cp = getKeyword("token");
		if(!cp)
			cp = ",";
		snprintf(buf, sizeof(buf), "PACK: %s\n", cp);
		libWrite(buf);
                cp = getSymbol("session.caller");
                if(cp)
                {
                        snprintf(buf, sizeof(buf), "CALLER: %s\n", cp);
			libWrite(buf);
                }  
                cp = getSymbol("session.dialed");
                if(cp)
                {
                        snprintf(buf, sizeof(buf), "DIALED: %s\n", cp);
			libWrite(buf);
                }
                cp = getSymbol("session.display");
                if(cp)
                {
                        snprintf(buf, sizeof(buf), "DISPLAY: %s\n", cp);
			libWrite(buf);
                } 
		snprintf(buf, sizeof(buf), "EXTENSION: %s\n", audio.extension);
		libWrite(buf);
		snprintf(buf, sizeof(buf), "ENCODING: %s\n",
			Audio::getName(audio.encoding));
		libWrite(buf);
		snprintf(buf, sizeof(buf), "FRAMING: %ld\n", 
			(long)audio.framing);
		libWrite(buf);
		cp = getKeyword("prefix");
		if(cp)
		{
			snprintf(buf, sizeof(buf), "PREFIX: %s\n", cp);
			libWrite(buf);
		}
                cp = getKeyword("voice");
		if(!cp)
			cp = voicelib;
                if(!cp)
                        cp = "none/prompts";
                snprintf(buf, sizeof(buf), "VOICE: %s\n", cp); 
		libWrite(buf);
                snprintf(buf, sizeof(buf), "START: %s %s\n", 
                        var_date, var_time);
		libWrite(buf);
                snprintf(buf, sizeof(buf), "IFACE: %s\n", 
                        getExternal("session.interface"));
		libWrite(buf);
                snprintf(buf, sizeof(buf), "CTYPE: %s\n", 
                        getExternal("session.type"));
		libWrite(buf);
		cp = getSymbol("session.info");
		if(!cp)
			cp = "none";
		snprintf(buf, sizeof(buf), "CINFO: %s\n", cp);
		libWrite(buf);
		cp = getSymbol("session.callref");
		snprintf(buf, sizeof(buf), "CREF: %s\n", cp);
		libWrite(buf);
		libWrite("\n");
	}
	leave();
}	

#endif

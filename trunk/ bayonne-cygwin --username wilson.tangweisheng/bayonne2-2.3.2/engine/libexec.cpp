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

#ifdef	HAVE_LIBEXEC
#include <cc++/process.h>
#include <cc++/slog.h>
#include <cc++/export.h>
#include "libexec.h"

#include <cstdarg>
#include <cstdio>

using namespace ost;
using namespace std;

Libexec::Libexec()
{
	const char *cp;
	char *p;
	char lbuf[256];
	unsigned num;

#ifndef	WIN32
	Process::setPosixSignal(SIGPIPE, SIG_IGN);
#endif

	query[0] = 0;
	digits[0] = 0;
	setString(position, sizeof(position), "00:00:00.000");	
	exitcode = 0;
	reply = 0;
	level = 0;
	result = RESULT_SUCCESS;

	voice = NULL;
	tsid = Process::getEnv("PORT_TSESSION");
	if(!tsid)
		return;

	cout << tsid << " head" << endl;
	while(tsid && !exitcode)
	{
		cin.getline(lbuf, sizeof(lbuf));
		if(cin.eof())
		{
			tsid = NULL;
			break;
		}
		cp = lbuf;
		p = strchr(lbuf, '\n');
		if(p)
			*p = 0;
		p = strchr(lbuf, '\r');
		if(p)
			*p = 0;
		if(!*cp)
			break;

		num = atoi(cp);
		if(num)
			reply = num;
		if(num >= 900)
		{
			exitcode = num - 900;
			tsid = NULL;
			break;
		}
		if(!isalpha(*cp))
			continue;
		p = strchr(lbuf, ':');
		if(!p)
			continue;
		*p = 0;
		head.setValue(lbuf, p + 2);		
	}

	if(!tsid || exitcode)
		return;

	cout << tsid << " args" << endl;
	while(tsid && !exitcode)
	{
		cin.getline(lbuf, sizeof(lbuf));
		if(cin.eof())
		{
			tsid = NULL;
			break;
		}
		cp = lbuf;
		p = strchr(lbuf, '\n');
		if(p)
			*p = 0;
		p = strchr(lbuf, '\r');
		if(p)
			*p = 0;
		if(!*cp)
			break;
		num = atoi(cp);
		if(num)
			reply = num;
		if(num >= 900)
		{
			tsid = NULL;
			exitcode = num - 900;
			break;
		}
		if(!isalpha(*cp))
			continue;
		p = strchr(lbuf, ':');
		if(!p)
			continue;
		*p = 0;
		args.setValue(lbuf, p + 2);		
	}
}

void Libexec::postSym(const char *id, const char *value)
{
	const char *sid = head.getLast("SESSION");
	if(!sid)
		return;

	cout << sid << " POST " << id << " " << value << endl;
}

const char *Libexec::getArg(const char *id)
{
	return args.getLast(id);
}

const char *Libexec::getEnv(const char *id)
{
	const char *cp;
	char bid[48];
	char *up;

	cp = head.getLast(id);
	if(cp)
		return cp;

	snprintf(bid, sizeof(bid), "%s", id);
	up = bid;
	while(*up)
	{
		*up = toupper(*up);
		++up;
	}
	return Process::getEnv(bid);
}

const char *Libexec::getPath(const char *file, char *buffer, unsigned size)
{
	const char *pre = head.getLast("PREFIX");
	const char *ext = head.getLast("EXTENSION");
	const char *var = Process::getEnv("SERVER_PREFIX");
	const char *ram = Process::getEnv("SERVER_TMPFS");
	const char *tmp = Process::getEnv("SERVER_TMP");
	const char *spos, *epos;

	if(!file || !*file)
		return NULL;

	if(*file == '/' || *file == '.' || file[1] == ':')
		return NULL;

	if(strstr(file, ".."))
		return NULL;

	if(strstr(file, "/."))
		return NULL;

	spos = strrchr(file, '/');
	if(!spos)
		spos = strrchr(file, '\\');

	epos = strrchr(file, '.');
	if(epos < spos)
		epos = NULL;

	if(epos)
		ext = "";

	if(!strnicmp(file, "tmp:", 4))
	{
		snprintf(buffer, size, "%s/%s%s", tmp, file, ext);
		return buffer;
	}

        if(!strnicmp(file, "ram:", 4))
        {
                snprintf(buffer, size, "%s/%s%s", ram, file, ext);
                return buffer;
        }

	if(strchr(file, ':'))
		return "";

	if(!spos)
	{
		if(!pre)
			return NULL;

		snprintf(buffer, size, "%s/%s/%s%s", var, pre, file, ext);
		return buffer;
	}

	snprintf(buffer, size, "%s/%s%s", var, file, ext);
	return buffer;
}

const char *Libexec::getFile(const char *file)
{
	if(!file || !*file)
		return NULL;

	if(*file == '/' || *file == '.' || file[1] == ':')
		return NULL;

	if(strstr(file, ".."))
		return NULL;

	if(strstr(file, "/."))
		return NULL;

	if(!strnicmp(file, "tmp:", 4))
		return file;

	if(!strnicmp(file, "ram:", 4))
		return file;

	if(strchr(file, ':'))
		return NULL;

	if(strchr(file, '/'))
		return file;

	if(head.getLast("PREFIX"))
		return file;

	return NULL;
}

void Libexec::hangupSession(void)
{
	if(!tsid)
		return;

	cout << tsid << " HANGUP" << endl;
	tsid = NULL;
}

void Libexec::detachSession(unsigned code)
{
	if(code > 255)
		code = 255;

	if(!tsid)
		return;

	cout << tsid << " EXIT " << code << endl;
	tsid = NULL;
}	

void Libexec::sendError(const char *msg)
{
        if(!tsid)
                return;

        cout << tsid << " ERROR " << msg << endl;
        tsid = NULL;
}

Bayonne::result_t Libexec::replayFile(const char *file)
{
	char buffer[512];

	file = getFile(file);
	if(!file)
		return RESULT_BADPATH;

	snprintf(buffer, sizeof(buffer), "REPLAY %s", file);
	return sendCommand(buffer);
}

Bayonne::result_t Libexec::replayOffset(const char *file, const char *offset) 
{
        char buffer[512]; 

        file = getFile(file);
        if(!file)
                return RESULT_BADPATH;

        snprintf(buffer, sizeof(buffer), "REPLAY %s %s", file, offset);
        return sendCommand(buffer);
}

Bayonne::result_t Libexec::recordFile(const char *file, timeout_t timeout, timeout_t silence)
{
        char buffer[512];

        file = getFile(file);
        if(!file)
                return RESULT_BADPATH;

        snprintf(buffer, sizeof(buffer), "RECORD %s %ld %ld", 
		file, (long)timeout, (long)silence);
        return sendCommand(buffer);
}  

Bayonne::result_t Libexec::recordOffset(const char *file, const char *offset, timeout_t timeout, timeout_t silence)
{
        char buffer[512];

        file = getFile(file);
        if(!file)
                return RESULT_BADPATH;

        snprintf(buffer, sizeof(buffer), "REPLAY %s %ld %ld %s", 
		file, (long)timeout, (long)silence, offset);
        return sendCommand(buffer);
}  

Bayonne::result_t Libexec::eraseFile(const char *file)
{
	char path[256];

	file = getPath(file, path, sizeof(path));
	if(!file)
		return RESULT_BADPATH;

	if(remove(path))
		return RESULT_FAILED;

	return RESULT_SUCCESS;
}

Bayonne::result_t Libexec::moveFile(const char *file1, const char *file2)
{
        char path1[256];
	char path2[256]; 

        file1 = getPath(file1, path1, sizeof(path1));
	file2 = getPath(file2, path2, sizeof(path2));
        if(!file1 || !file2)
                return RESULT_BADPATH;

        if(rename(path1, path2))
                return RESULT_FAILED;

        return RESULT_SUCCESS;
}  

Bayonne::result_t Libexec::clearInput(void)
{
	return sendCommand("FLUSH");
}

Bayonne::result_t Libexec::xferCall(const char *dest)
{
	char buf[512];

	snprintf(buf, sizeof(buf), "xfer %s", dest);
	return sendCommand(buf);
}

bool Libexec::waitInput(timeout_t timeout)
{
        char buffer[512];    
        snprintf(buffer, sizeof(buffer), "WAIT %ld", (long)timeout);
	if(sendCommand(buffer) == RESULT_PENDING)
		return true;
	return false;
}   

char Libexec::readKey(timeout_t timeout)
{
	char buffer[32];
	char inbuf[2];
	snprintf(buffer, sizeof(buffer), "READ %ld", (long)timeout);
	sendCommand(buffer, inbuf, 2);
	return inbuf[0];
}

Bayonne::result_t Libexec::readInput(char *ib, unsigned size, timeout_t timeout)
{
	char buffer[32];
	snprintf(buffer, sizeof(buffer), "READ %ld %d", 
		(long)timeout, size - 1);
	return sendCommand(buffer, ib, size);
}

Bayonne::result_t Libexec::getSym(const char *id, char *ib, unsigned size)
{
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "GET %s", id);
        return sendCommand(buffer, ib, size);
}    

Bayonne::result_t Libexec::setSym(const char *id, const char *value)
{
	char buffer[512];
	snprintf(buffer, sizeof(buffer), "SET %s %s", id, value);
	return sendCommand(buffer);
}

Bayonne::result_t Libexec::addSym(const char *id, const char *value)
{
        char buffer[512];
        snprintf(buffer, sizeof(buffer), "ADD %s %s", id, value);
        return sendCommand(buffer);
}

Bayonne::result_t Libexec::sizeSym(const char *id, unsigned size)
{
        char buffer[512];
        snprintf(buffer, sizeof(buffer), "NEW %s %d", id, size);
        return sendCommand(buffer);
}
   	
Bayonne::result_t Libexec::sendResult(const char *code)
{
	char buffer[512];
	snprintf(buffer, sizeof(buffer), "RESULT %s", code);
	return sendCommand(buffer);
}

Bayonne::result_t Libexec::playSingleTone(short f1, timeout_t duration, unsigned l)
{
	char buffer[512];
	if(!l)
		l = level;
	snprintf(buffer, sizeof(buffer), "STONE %d %ld %d", 
		f1, (long)duration, l);
	return sendCommand(buffer);
}

Bayonne::result_t Libexec::playDualTone(short f1, short f2, timeout_t duration, unsigned l)
{
        char buffer[512];
        if(!l)
                l = level;
        snprintf(buffer, sizeof(buffer), "DTONE %d %d %ld %d", 
		f1, f2, (long)duration, l);
	return sendCommand(buffer);
}

Bayonne::result_t Libexec::playTone(const char *tone, timeout_t timeout, unsigned _level)
{
	char buffer[512];
	if(!_level)
		_level = level;
	snprintf(buffer, sizeof(buffer), "TONE %s %ld %d", 
		tone, (long)timeout, _level);
	return sendCommand(buffer);
}

Bayonne::result_t Libexec::speak(const char *format, ...)
{
	va_list args;
	va_start(args, format);

	result_t result;
	char buffer[512];
	const char *v = voice;
	unsigned len;
	if(!v)
		v = "PROMPT";

	snprintf(buffer, sizeof(buffer), "%s ", v);
	len = strlen(buffer);

	vsnprintf(buffer + len, sizeof(buffer) - len, format, args);
	result = sendCommand(buffer);
	va_end(args);
	return result;
}
	
Bayonne::result_t Libexec::sendCommand(const char *text, char *buffer, unsigned size)
{
        const char *cp;
        char *p;
        char lbuf[256];
	unsigned num = 0;

	result = RESULT_OFFLINE;
	if(buffer)
		*buffer = 0;

	if(!tsid || exitcode > 0)
		return RESULT_OFFLINE;

	cout << tsid << " " << text << endl;
	while(!exitcode)
	{
		cin.getline(lbuf, sizeof(lbuf));
		if(cin.eof())
		{
			tsid = NULL;
			break;
		}
                cp = lbuf;
                p = strchr(lbuf, '\n');
                if(p)
                        *p = 0;
                p = strchr(lbuf, '\r');      
                if(p)
                        *p = 0;
                if(!*cp)
                        break;   
		num = atoi(cp); 
                if(num)
                        reply = num;
                if(num >= 900) 
                {       
                        exitcode = num - 900;
                        tsid = NULL;
                        break;
                }
                if(!isalpha(*cp))
                        continue;
                p = strchr(lbuf, ':');
                if(!p)
                        continue;
                *p = 0;                               
		p += 2;
		switch(reply)
		{
		case 200:
			result = RESULT_SUCCESS;
			setString(query, sizeof(query), p);
			if(buffer)
				setString(buffer, size, p);
			break;
		case 100:
			if(!stricmp(lbuf, "result"))
				result = (result_t)atoi(p);
			else if(!stricmp(lbuf, "digits"))
			{
				setString(digits, sizeof(digits), p);
				if(buffer)
					setString(buffer, size, p);
			}
			else if(!stricmp(lbuf, "position"))
				setString(position, sizeof(position), p);
			break;
		}
	}
	return result;
}

#endif


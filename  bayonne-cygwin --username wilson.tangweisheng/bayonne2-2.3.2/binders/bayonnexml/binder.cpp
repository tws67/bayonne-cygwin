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
#ifndef	WIN32
#include "private.h"
#endif

namespace binder {
using namespace ost;
using namespace std;

Binder	Binder::ivrscript;

Binder::Binder() :
BayonneBinder("ivrscript1")
{
        static ScriptInterp::Define interp[] = { 
                {"endinput", false, (Method)&Methods::scrEndinput,
                        (Check)&Checks::chkEndinput}, 
		{"endform", false, (Method)&Methods::scrEndform,
			(Check)&Checks::chkConditional},
		{"form", false, (Method)&Methods::scrForm,
			(Check)&Checks::chkIgnore},
                {"cdr", false, (Method)&ScriptMethods::scrNop,   
                        (Check)&Checks::chkCDR},   
                {"_keydata_", false, (Method)&Methods::scrKeydata,
                        (Check)&Checks::chkKeydata}, 
                {"key", true, (Method)&Methods::scrKey,
                        (Check)&Checks::chkKey},  
                {"select", false, (Method)&ScriptMethods::scrNop,  
                        (Check)&Checks::chkSelect}, 
		{"startxml", false, (Method)&Methods::scrStart,
			(Check)&Checks::chkStart},
		{"parsexml", false, (Method)&Methods::scrParse,
			(Check)&Checks::chkParse},
		{"-assign", true, (Method)&Methods::xmlAssign,
			(Check)&Checks::chkIgnore},
                {"-debug", true, (Method)&Methods::xmlDebug,
                        (Check)&Checks::chkIgnore},
                {"-error", true, (Method)&Methods::xmlError,
                        (Check)&Checks::chkIgnore},
                {"-notice", true, (Method)&Methods::xmlNotice,
                        (Check)&Checks::chkIgnore},
		{"-hangup", false, (Method)&Methods::xmlHangup,
			(Check)&Checks::chkIgnore},
		{"-voice", false, (Method)&Methods::xmlVoice,
			(Check)&Checks::chkIgnore},
                {NULL, false, NULL, NULL}}; 

	bind(interp);
	slog.info("binding bayonnexml...");
	PersistProperty::load();

	addConfig("timeslot.conf");
	addConfig("connect.conf");
	addConfig("dialed.conf");
	addConfig("runlevel.conf");

	ScriptInterp::addConditional("key", &testKey); 
	Bayonne::use_prefix = true;
	Bayonne::use_macros = true;
	Bayonne::use_merge = false;
	allocateLocal();
};

Script::Name *Binder::getIncoming(ScriptImage *img, BayonneSession *s, Event *event)
{
    const char *did;
    char buf[80];
    Name *scr = NULL;

    if(sla[0])
    {
        scr = img->getScript(sla);
        if(scr)
            return scr;
    }

    did = s->getSymbol("session.hostname");
    if(did && !*did)
        did = NULL;

    if(did)
    {
        snprintf(buf, sizeof(buf), "dialed::%s", did);
        scr = img->getScript(buf);
        if(scr)
            return scr;
    }

	if(event->start.scr)
		return event->start.scr;

	did = getRegistryId(s->getSymbol("session.dialed"));
    if(did)
    {
        snprintf(buf, sizeof(buf), "dialed::%s", did);
        scr = img->getScript(buf);
        if(scr)
            return scr;
    }
	
    return BayonneBinder::getIncoming(img, s, event);
}

bool Binder::select(ScriptInterp *interp)
{
        char buf[65]; 

	const char *cinfo = interp->getSymbol("session.info");
	const char *ctype = interp->getExternal("session.type");
	const char *rings = interp->getExternal("session.rings");

	if(!stricmp(ctype, "incoming") && cinfo && *cinfo)
	{
		snprintf(buf, sizeof(buf), "incoming:%s", cinfo);
		if(scriptEvent(interp, buf))
			return true;

		if(rings && atoi(rings) > 0)
		{
			if(scriptEvent(interp, "incoming:ringing"))
				return true;
		}
		else if(scriptEvent(interp, "incoming:immediate"))
			return true;
	}
	else if(!stricmp(ctype, "forward"))
	{
		if(scriptEvent(interp, "incoming:forward"))
			return true;
	}
	else if(!stricmp(ctype, "recall"))
	{
		if(scriptEvent(interp, "incoming:recall"))
			return true;
	}
	else if(!stricmp(ctype, "pickup"))
	{
		if(scriptEvent(interp, "incoming:pickup"))
			return true;
	}
	return false;		
}

void Binder::attach(ScriptInterp *interp)
{
	interp->setSymbol("session.xmlquery", "", 512);
};

void Binder::detach(ScriptInterp *interp)
{
	Line *cdr;
	char buffer[256];
	char var[65];
	char *p;
	unsigned idx = 0;
	const char *cp;
	size_t len;

	BayonneSession *s = (BayonneSession *)interp;
	ScriptImage **ip = getLocalImage(s->getSlot());
	ScriptImage *img = interp->getImage();
	Name *scr = interp->getName();

	if(ip && *ip)
	{
		delete *ip;
		*ip = NULL;
	}

	snprintf(var, sizeof(var), "cdr.%s", scr->name);
	p = strchr(var, ':');
	if(p)
		*p = 0; 

	cdr = (Line *)img->getPointer(var);	
	if(!cdr)
		return;

	snprintf(buffer, sizeof(buffer), "%s:", var + 4);
	len = strlen(buffer);
	
	while(len < sizeof(buffer) - 2 && NULL != (cp = getOption(cdr, &idx)))
	{
		if(isalpha(*cp))
		{
			var[0] = '%';
			snprintf(var + 1, sizeof(var) - 1, "session.%s", cp);
			cp = interp->getContent(var);
		}
		else
			cp = interp->getContent(cp);
		if(!cp || !*cp)
			cp = "-";
		snprintf(buffer + len, sizeof(buffer) - len, " %s", cp);
		len = strlen(buffer);
	}
#ifndef	WIN32
	if(getppid() > 1)
		fprintf(stderr, "%s\n", buffer);
#endif
	buffer[len++] = '\n';
	buffer[len] = 0;

#ifdef	WIN32
	HANDLE fd;
	DWORD res;
	fd = CreateFile(Bayonne::server->getLast("calls"), 
		GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if(fd == INVALID_HANDLE_VALUE)
			return;

	Thread::yield();
	SetFilePointer(fd, 0, 0, FILE_END);
	WriteFile(fd, buffer, (DWORD)len, &res, NULL);
	CloseHandle(fd);
#else
	int fd;
	fd = ::open(server->getLast("calls"), O_WRONLY | O_APPEND | O_CREAT, 0640);
	if(fd < 0)
		return;

	::write(fd, buffer, len);
	::close(fd);
#endif
}

void Binder::down(void)
{
	slog.info("saving persistent keys...");
	PersistProperty::save();
}

bool Binder::reload(ScriptCompiler *img)
{
	ScriptCommand *server = img->getCommand();
	const char *prefix;
	unsigned count = server->Keydata::getCount();
	char **keys = new char *[count + 1];
	const char *ext;

#ifdef  SCRIPT_DEFINE_TOKENS
	img->compileDefinitions("url.def");
	img->compileDefinitions("xml.def");
	img->compileDefinitions("tts.def");
#endif

	count = server->Keydata::getIndex(keys, count);
	while(count)
	{
		prefix = *(keys++);
		if(!prefix)
			break;

		ext = strrchr(prefix, '.');
		if(ext && strstr(Bayonne::exec_extensions, ext))
			goto compile;

		if(ext && !stricmp(ext, ".conf"))
			goto conf;

		if(!ext || stricmp(ext, ".scr"))
			continue;

compile:
		++Bayonne::compile_count;
		prefix = server->getLast(prefix);
		if(canAccess(prefix))
			img->compile(prefix);
		continue;

conf:
		++Bayonne::compile_count;
		prefix = server->getLast(prefix);
		if(canAccess(prefix))
			img->compile(prefix);
	}
	return true;
}

bool Binder::testKey(ScriptInterp *interp, const char *v)      
{
        return PersistProperty::test(v);
}
  

} // end namespace

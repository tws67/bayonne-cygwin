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
#else
#define	CONFIG_FILES	"C:\\Program Files\\GNU Telephony\\Bayonne Config"
#endif

namespace binder {
using namespace ost;
using namespace std;

Binder	Binder::ivrscript;

Binder::Binder() :
BayonneBinder("ivrscript")
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
                {"dial", false, (Method)&Methods::scrDial,
                        (Check)&Checks::chkDial},
                {"select", false, (Method)&ScriptMethods::scrNop,  
                        (Check)&Checks::chkSelect}, 
                {"incoming", false, (Method)&ScriptMethods::scrNop,
                        (Check)&Checks::chkAssign},
                {"caller", false, (Method)&ScriptMethods::scrNop,   
                        (Check)&Checks::chkAssign},  
                {"assign", false, (Method)&ScriptMethods::scrNop,
                        (Check)&Checks::chkAssign},    
                {"_keydata_", false, (Method)&Methods::scrKeydata,
                        (Check)&Checks::chkKeydata},
                {"register", false, (Method)&ScriptMethods::scrNop,
                        (Check)&Checks::chkRegister},
                {"extern", false, (Method)&ScriptMethods::scrNop,
                        (Check)&Checks::chkRegister}, 
		{"extension", false, (Method)&ScriptMethods::scrNop,
			(Check)&Checks::chkRegister},
                {"gateway", false, (Method)&ScriptMethods::scrNop,
                        (Check)&Checks::chkRegister},
                {"key", true, (Method)&Methods::scrKey,
                        (Check)&Checks::chkKey},  
		{"stop", false, (Method)&Methods::scrStop,
			(Check)&Checks::chkStop},
                {"cancel", false, (Method)&Methods::scrStop,
                        (Check)&Checks::chkStop},
		{"start", false, (Method)&Methods::scrStart,
			(Check)&Checks::chkStart},
		{"connect", false, (Method)&Methods::scrConnect,
			(Check)&Checks::chkConnect},
		{"join", false, (Method)&Methods::scrJoin,
			(Check)&Checks::chkJoin},
		{"recall", false, (Method)&Methods::scrRecall,
			(Check)&Checks::chkRecall},
		{"redial", false, (Method)&Methods::scrRedial,
			(Check)&Checks::chkRedial},
        {"reroute", false, (Method)&Methods::scrRedial,
            (Check)&Checks::chkRedial},
		{"feature", false, (Method)&Methods::scrFeature,
			(Check)&Checks::chkFeature},
		{"release", true, (Method)&Methods::scrRelease,
			(Check)&Checks::chkRecall},
                {NULL, false, NULL, NULL}}; 

	bind(interp);
	slog.info("binding ivrscript...");
	PersistProperty::load();

	ScriptInterp::addConditional("key", &testKey);
	ScriptInterp::addConditional("registered", &testRegistered);
	ScriptInterp::addConditional("available", &testAvailable);
	ScriptInterp::addConditional("extension", &testExternal);
	ScriptInterp::addConditional("external", &testExternal);
	ScriptInterp::addConditional("reachable", &testReachable);
	ScriptInterp::addConditional("destination", &testDestination);

	Bayonne::use_prefix = true;
	Bayonne::use_macros = true;
	Bayonne::use_merge = true;

	addConfig("timeslot.conf");
	addConfig("connect.conf");
	addConfig("feature.conf");
	addConfig("caller.conf");
	addConfig("dialed.conf");
	addConfig("local.conf");
	addConfig("runlevel.conf");
};

Script::Name *Binder::getIncoming(ScriptImage *img, BayonneSession *s, Event *event)
{
	const char *cid, *did, *uid;
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

	uid = getRegistryId(s->getSymbol("session.identity"));
	cid = getRegistryId(s->getSymbol("session.caller"));

	if(cid && !uid)
	{
		snprintf(buf, sizeof(buf), "caller::%s", cid);
		scr = img->getScript(buf);
		if(scr)
			return scr;
	}

	if(uid)
	{
		snprintf(buf, sizeof(buf), "local::%s", uid);
		scr = img->getScript(buf);
		if(scr)
			return scr;
	}

	did = getRegistryId(s->getSymbol("session.dialed"));
	if(did && uid && !stricmp(did, uid))
	{
		scr = img->getScript("local::_self_");
		if(scr)
			return scr;
	}

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

	ScriptImage *img = interp->getImage();
	Name *scr = interp->getName();

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
	const char *prefix, *cp;
	unsigned count = server->Keydata::getCount();
	char **keys = new char *[count + 1];
	char buffer[65];
	const char *ext;

#ifdef  SCRIPT_DEFINE_TOKENS
        const char *const *list = server->getList("definitions");

        img->compileDefinitions("site.def");
        while(list && *list)
        {
               	img->compileDefinitions(*list);
               	++list;
        }
	cp = server->getLast("location");
	if(cp)
	{
		snprintf(buffer, sizeof(buffer), "macros-%s.def", cp);
		img->compileDefinitions(buffer);
	}
	img->compileDefinitions("macros.def");

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

#ifdef	WIN32
	prefix = "C:\\Program Files\\GNU Telephony\\Bayonne IVRScript";
#else
	prefix = BAYONNE_FILES "/ivrscript";
#endif
	if(prefix)
	{
		if(isDir(prefix))
			compileDir(prefix, img);
		else
			slog.error("%s: directory missing", prefix);
	}

	prefix = server->getLast("scripts");
	if(prefix)
	{
		if(isDir(prefix))
			compileDir(prefix, img);
		else
			slog.error("%s: directory missing", prefix);
	}

	return true;
}

void Binder::compileDir(const char *prefix, ScriptCompiler *img)
{
        Dir dir(prefix);
        char path[256];
        const char *ext;
        const char *file;
        unsigned len;
        char *pp; 

        slog.info("compiler scanning %s", prefix);
        setString(path, sizeof(path), prefix);
        len = strlen(path);
        pp = path + len;
        len = sizeof(path) - len;  

        *pp++ = '/';
        --len;
                   
        while(NULL != (file = dir.getName()))
        {
                if(*file == '.')
                        continue;

                ext = strrchr(file, '.');
                if(ext && strstr(Bayonne::exec_extensions, ext))
                        goto compile;

                if(ext && !stricmp(ext, ".scr"))
                        goto compile;

                continue;
compile:  
                ++Bayonne::compile_count;
                setString(pp, len, file);
		if(canAccess(path))
	                img->compile(path);
        }
}  

bool Binder::testKey(ScriptInterp *interp, const char *v)      
{
        return PersistProperty::test(v);
}

bool Binder::testRegistered(ScriptInterp *inter, const char *v)
{
	BayonneDriver *d = NULL;
	char vbuf[8];
	char *p;

	setString(vbuf, sizeof(vbuf), v);
	p = strchr(v, ':');
	if(p)
	{
		*p = 0;
		v = (const char *)++p;
		d = BayonneDriver::get(vbuf);
	}
	if(!d)
		d = BayonneDriver::getProtocol();
	if(!d)
		return false;

	return d->isRegistered(v);
}

bool Binder::testExternal(ScriptInterp *interp, const char *v)
{
	BayonneDriver *d = NULL;
	char vbuf[8];
	char *p;

	setString(vbuf, sizeof(vbuf), v);
	p = strchr(v, ':');
	if(p)
	{
		*p = 0;
		v = (const char *)++p;
		d = BayonneDriver::get(vbuf);
	}
	if(!d)
		d = BayonneDriver::getProtocol();
	if(!d)
		return false;

	return d->isExternal(v);
}

bool Binder::testAvailable(ScriptInterp *inter, const char *v)
{
	BayonneDriver *d = NULL;
	char vbuf[8];
	char *p;

	setString(vbuf, sizeof(vbuf), v);
	p = strchr(v, ':');
	if(p)
	{
		*p = 0;
		v = (const char *)++p;
		d = BayonneDriver::get(vbuf);
	}
	if(!d)
		d = BayonneDriver::getProtocol();
	if(!d)
		return false;

	return d->isAvailable(v);
}  
	
bool Binder::testDestination(ScriptInterp *interp, const char *v)
{
	char name[80];
	ScriptImage *img = interp->getImage();
	snprintf(name, sizeof(name), "dialed::%s", v);
	
	if(img->getScript(name))
		return true;

	return false;
}

bool Binder::testReachable(ScriptInterp *inter, const char *v)
{
	BayonneDriver *d = NULL;
	char vbuf[8];
	char *p;

	setString(vbuf, sizeof(vbuf), v);
	p = strchr(v, ':');
	if(p)
	{
		*p = 0;
		v = (const char *)++p;
		d = BayonneDriver::get(vbuf);
	}
	if(!d)
		d = BayonneDriver::getProtocol();
	if(!d)
		return false;

	return d->isReachable(v);
}  

} // end namespace

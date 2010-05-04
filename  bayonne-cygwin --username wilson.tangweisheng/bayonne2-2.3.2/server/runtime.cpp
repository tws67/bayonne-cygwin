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
#include <cstdarg>

namespace server {
using namespace ost;
using namespace std;

Runtime::Runtime() :
ScriptRuntime()
{
	static ScriptInterp::Define interp[] = {
		{"dialtone", false, (Method)&Methods::scrTone,
			(Check)&Checks::chkTone},
                {"busytone", false, (Method)&Methods::scrTone,
                        (Check)&Checks::chkTone},
                {"ringback", false, (Method)&Methods::scrTone,
                        (Check)&Checks::chkTone},
                {"reorder", false, (Method)&Methods::scrTone,
                        (Check)&Checks::chkTone},
                {"beep", false, (Method)&Methods::scrTone,
                        (Check)&Checks::chkTone},
                {"sit", false, (Method)&Methods::scrTone,
                        (Check)&Checks::chkTone},
		{"tone", false, (Method)&Methods::scrTonegen,
			(Check)&Checks::chkTonegen},
		{"teltone", false, (Method)&Methods::scrTone,
			(Check)&Checks::chkTone},
                {"callwait", false, (Method)&Methods::scrTone,
                        (Check)&Checks::chkTone},
                {"callback", false, (Method)&Methods::scrTone,
                        (Check)&Checks::chkTone},
        {"service", true, (Method)&ScriptMethods::scrNop,
            (Check)&Checks::chkService},
		{"lang", true, (Method)&ScriptMethods::scrNop,
			(Check)&Checks::chkLang},
		{"languages", true, (Method)&ScriptMethods::scrNop,
			(Check)&Checks::chkLang},
		{"load", true, (Method)&ScriptMethods::scrNop,
			(Check)&Checks::chkLoad},
		{"dir", true, (Method)&ScriptMethods::scrNop,
			(Check)&Checks::chkDir},
		{"list", true, (Method)&Methods::scrList,
			(Check)&Checks::chkList},
		{"pathname", false, (Method)&Methods::scrPathname,
			(Check)&Checks::chkPathname},
		{"readpath", true, (Method)&Methods::scrReadpath,
			(Check)&Checks::chkPaths},
		{"writepath", true, (Method)&Methods::scrWritepath,
			(Check)&Checks::chkPaths},
		{"path", true, (Method)&Methods::scrPath,
			(Check)&Checks::chkPaths},
		{"id", true, (Method)&Methods::scrId,
			(Check)&Checks::chkId},
        {"idpath", true, (Method)&Methods::scrId,
            (Check)&Checks::chkId},
		{"param", true, (Method)&Methods::scrParam,
			(Check)&Checks::chkParam},
		{"voicelib", false, (Method)&Methods::scrVoicelib,
			(Check)&Checks::chkVoicelib},
		{"timeslot", true, (Method)&Methods::scrTimeslot,
			(Check)&ScriptChecks::chkType},
		{"reconnect", false, (Method)&Methods::scrReconnect,
			(Check)&Checks::chkReconnect},
        {"detach", false, (Method)&Methods::scrDetach,
            (Check)&Checks::chkDetach},
		{"position", true, (Method)&Methods::scrPosition,
			(Check)&ScriptChecks::chkType},
        {"echo", false, (Method)&Methods::scrEcho,
			(Check)&ScriptChecks::chkHasArgs},
		{"pickup", false, (Method)&Methods::scrPickup,
			(Check)&ScriptChecks::chkOnlyCommand},
                {"hangup", false, (Method)&Methods::scrHangup, 
                        (Check)&ScriptChecks::chkOnlyCommand},
                {"answer", false, (Method)&Methods::scrAnswer,
                        (Check)&ScriptChecks::chkOnlyCommand},
		{"collect", false, (Method)&Methods::scrCollect,
			(Check)&Checks::chkCollect},
		{"keyinput", false, (Method)&Methods::scrKeyinput,
			(Check)&Checks::chkKeyinput},
		{"read", false, (Method)&Methods::scrRead,
			(Check)&Checks::chkInput},
		{"input", false, (Method)&Methods::scrInput,
			(Check)&Checks::chkInput},
		{"keywait", false, (Method)&Methods::scrWaitkey,
			(Check)&Checks::chkSleep},
		{"sync", false, (Method)&Methods::scrSync,
			(Check)&Checks::chkSleep},
		{"sleep", false, (Method)&Methods::scrSleep,
			(Check)&Checks::chkSleep},
		{"route", false, (Method)&Methods::scrRoute,
			(Check)&Checks::chkRoute},
		{"droute", true, (Method)&Methods::scrRoute,
			(Check)&Checks::chkRoute},
		{"sroute", true, (Method)&Methods::scrSRoute,
			(Check)&Checks::chkRoute},
#ifdef  WIN32
                {"link", false, (Method)&Methods::scrCopy,
                        (Check)&Checks::chkMove},
#else
                {"link", false, (Method)&Methods::scrLink,
                        (Check)&Checks::chkMove},
#endif
		{"write", false, (Method)&Methods::scrWrite,
			(Check)&Checks::chkWrite},
		{"copy", false, (Method)&Methods::scrCopy,
			(Check)&Checks::chkMove},
		{"move", false, (Method)&Methods::scrMove,
			(Check)&Checks::chkMove},
		{"erase", false, (Method)&Methods::scrErase,
			(Check)&Checks::chkErase},
		{"build", false, (Method)&Methods::scrBuild,
			(Check)&Checks::chkBuild},
		{"record", false, (Method)&Methods::scrRecord,
			(Check)&Checks::chkRecord},
		{"append", false, (Method)&Methods::scrAppend,
			(Check)&Checks::chkAppend},
		{"mf", false, (Method)&Methods::scrMF,
			(Check)&Checks::chkDialer},
		{"dtmf", false, (Method)&Methods::scrDTMF,
			(Check)&Checks::chkDialer},
		{"transfer", false, (Method)&Methods::scrTransfer,
			(Check)&Checks::chkTransfer},
		{"libexec", false, (Method)&Methods::scrLibexec,
			(Check)&Checks::chkLibexec},
		{"exec", false, (Method)&Methods::scrLibexec,
			(Check)&Checks::chkLibexec},
		{"exit", false, (Method)&Methods::scrExit,
			(Check)&Checks::chkExit},
		{"cleardigits", false, (Method)&Methods::scrCleardigits,
			(Check)&Checks::chkCleardigits},
		{"replay", false, (Method)&Methods::scrReplay,
			(Check)&Checks::chkReplay},
		{"prompt", false, (Method)&Methods::scrPrompt,
			(Check)&Checks::chkPathname},
		{"speak", false, (Method)&Methods::scrPrompt,
			(Check)&Checks::chkPathname},
		{"play", false, (Method)&Methods::scrPlay,
			(Check)&Checks::chkPathname},
		{"say", false, (Method)&Methods::scrSay,
			(Check)&Checks::chkSay},
		{"trap", false, (Method)&Methods::scrTrap,
			(Check)&Checks::chkTrap},
                {"snmptrap", false, (Method)&Methods::scrTrap,
                        (Check)&Checks::chkTrap},
        	{NULL, false, NULL, NULL}};

        trap("timeout", false);
        trap("dtmf");

        trap("0");      /* 0x10 */
        trap("1");
        trap("2");
        trap("3");

        trap("4");      /* 0x100 */
        trap("5");
        trap("6");
        trap("7");

        trap("8");      /* 0x1000 */
        trap("9");
        trap("star");
        trap("pound");

        trap("a");      /* 0x00010000 */
        trap("b");
        trap("c");
        trap("d");

	trap("ring");	/* 0x00100000 */
	trap("tone");
	trap("event");
	trap("wink");

	trap("child");	/* 0x01000000 */
	trap("fail");
	trap("pickup");
	trap("part");

	trap("invalid");
	trap("cancel");
	trap("join");

	load(interp);

	ScriptInterp::addConditional("runlevel", &testLevel);
	ScriptInterp::addConditional("safe", &testSafe);
	ScriptInterp::addConditional("file", &testFile);
	ScriptInterp::addConditional("dir", &testDir);
	ScriptInterp::addConditional("span", &testSpan);
	ScriptInterp::addConditional("driver", &testDriver);
	ScriptInterp::addConditional("timeslot", &testTimeslot);
	ScriptInterp::addConditional("voice", &testVoice);
	ScriptInterp::addConditional("lang", &testLang);
	ScriptInterp::addConditional("language", &testLang);
	ScriptInterp::addConditional("pattern", &testPattern);
	ScriptInterp::addConditional("dialed", &testDialed);
    ScriptInterp::addConditional("caller", &testCaller);
    ScriptInterp::addConditional("detach", &testDetach);
}

unsigned long Runtime::getTrapMask(const char *trapname)
{
        unsigned long mask;

        if(!stricmp(trapname, "hangup"))
                return 0x00000001;

        if(!strcmp(trapname, "!"))
                return 0x0000fff8;

        if(!stricmp(trapname, "override"))
                return 0x00010000;

        if(!stricmp(trapname, "flash"))
                return 0x00020000;

        if(!stricmp(trapname, "immediate"))
                return 0x00040000;

        if(!stricmp(trapname, "priority"))
                return 0x00080000;

        mask = ScriptRuntime::getTrapMask(trapname);
        if(mask == 0x00000008)
                return 0x0000fff8;

        if(mask & 0x0000fff0)
                mask |= 0x00000008;

        return mask;
}

const char *Runtime::getExternal(const char *opt)
{
	if(strnicmp(opt, "server.", 7))
		return NULL;

	opt += 7;
	if(!stricmp(opt, "software"))
		return "bayonne2";
	else if(!stricmp(opt, "version"))
		return VERSION;
	else if(!stricmp(opt, "node") || !stricmp(opt, "name"))
		return keyserver.getLast("node");
	else if(!stricmp(opt, "platform"))
		return keyserver.getLast("platform");
	else if(!stricmp(opt, "state"))
		return keyserver.getLast("state");
	else if(!stricmp(opt, "timeslots"))
		return keyserver.getLast("timeslots");
	else if(!stricmp(opt, "location"))
		return keyserver.getLast("location");
	else if(!stricmp(opt, "voice"))
		return keyserver.getLast("voice");
	return NULL;
}

void Runtime::errlog(const char *level, const char *msg)
{
        const char *path = getLast("errlog");
	char buffer[256];
	char *m;
        struct tm *dt, tbuf;
        time_t now;
        int year;

        static char *months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

	if(!path)
		return;

        time(&now); 
        dt = localtime_r(&now, &tbuf);  

        year = dt->tm_year;
        if(year < 1000)
                year += 1900;

        m = months[dt->tm_mon];
        snprintf(buffer, sizeof(buffer), "[%s %02d %02d:%02d:%02d %d] [%s] %s\n",
        	m, dt->tm_mday, dt->tm_hour, dt->tm_min, dt->tm_sec, year, level, msg);

	m = strchr(buffer, '\n');
	if(m)
		*(++m) = 0;

#ifdef  WIN32
        DWORD count;
        enter();
        HANDLE fd = CreateFile(path, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if(fd != INVALID_HANDLE_VALUE)
        {
                SetFilePointer(fd, 0, NULL, FILE_END);
                WriteFile(fd, buffer, (DWORD)strlen(buffer), &count, NULL);
                CloseHandle(fd);
        }
        leave();

#else
        int fd = ::open(path, O_WRONLY | O_APPEND | O_CREAT, 0660);
        if(fd > -1)
        {
                ::write(fd, buffer, strlen(buffer));
                ::close(fd);
        }
#endif
}

bool Runtime::isInput(Line *line)
{
	if(line->scr.method == (Method)&Methods::scrKeyinput)
		return true;

	if(line->scr.method == (Method)&Methods::scrInput)
		return true;

	if(line->scr.method == (Method)&Methods::scrRead)
		return true;

	if(line->scr.method == (Method)&Methods::scrCleardigits)
		return true;

	if(line->scr.method == (Method)&Methods::scrCollect)
		return true;

	return false;
}

bool Runtime::testDriver(ScriptInterp *interp, const char *v)
{
	if(BayonneDriver::get(v))
		return true;
	
	return false;
}

bool Runtime::testDetach(ScriptInterp *interp, const char *v)
{
	const char *dj = interp->getSymbol("_detach_");
	if(!dj || !v)
		return false;

	if(!stricmp(dj, v))
		return true;
	return false;
}

bool Runtime::testSafe(ScriptInterp *interp, const char *v)
{
	if(*v == '.' || *v == '/')
		return false;

	if(strchr(v, ':'))
		return false;

	if(strstr(v, "/."))
		return false;

	return true;
}

bool Runtime::testFile(ScriptInterp *interp, const char *v)
{
	BayonneSession *s = (BayonneSession *)(interp);
	char buf[MAX_PATHNAME];
	const char *ext;

	if(!v || !*v)
		return false;

	// lets accept any valid path, including those from read/writepath
	if(*v == '/' || v[1] == ':')
		goto cont;

	ext = strrchr(v, '/');
	if(ext)
		ext = strrchr(ext, '.');
	else
		ext = strrchr(v, '.');

	if(ext)
		ext = "";
	else
		ext = ".au";

	if(!strnicmp(buf, "tmp:", 4))
	{
		snprintf(buf, sizeof(buf), "%s/%s%s", path_tmp, v + 4, ext);
		v = buf;
		goto cont;
	}

        if(!strnicmp(buf, "ram:", 4) || !strnicmp(buf, "mem:", 4))
        {
                snprintf(buf, sizeof(buf), "%s/%s%s", path_tmpfs, v + 4, ext);
                v = buf;
                goto cont;
        }

	if(!isalnum(*v))
		return false;

	if(strchr(v, ':'))
		return false;

	if(strchr(v, '/'))
		snprintf(buf, sizeof(buf), "%s/%s%s", 
			keypaths.getLast("datafiles"), v, ext);
	else
		snprintf(buf, sizeof(buf), "%s/%s/%s%s",
			path_prompts, s->defVoicelib(), v, ext);

	v = buf;

cont:
	if(strstr(v, ".."))
		return false;

	if(strstr(v, "/."))
		return false;

	return isFile(v);
}

bool Runtime::testDir(ScriptInterp *interp, const char *v)
{
	char buf[MAX_PATHNAME];

	if(!v || !*v)
		return false;

	if(*v == '/' || v[1] == ':')
		goto cont;

	if(!strnicmp(buf, "tmp:", 4))
	{
		snprintf(buf, sizeof(buf), "%s/%s", path_tmp, v + 4);
		v = buf;
		goto cont;
	}

        if(!strnicmp(buf, "ram:", 4) || !strnicmp(buf, "mem:", 4))
        {
                snprintf(buf, sizeof(buf), "%s/%s", path_tmpfs, v + 4);
                v = buf;
                goto cont;
        }

	if(!isalnum(*v))
		return false;

	if(strchr(v, ':'))
		return false;

	snprintf(buf, sizeof(buf), "%s/%s", keypaths.getLast("datafiles"), v);
	v = buf;

cont:
	if(strstr(v, ".."))
		return false;

	if(strstr(v, "/."))
		return false;

	return isDir(v);
}
	

bool Runtime::testSpan(ScriptInterp *interp, const char *v)
{
	if(BayonneSpan::get(atoi(v)))
		return true;

	return false;
}

bool Runtime::testTimeslot(ScriptInterp *interp, const char *v)
{
	if(getSid(v))
		return true;

	return false;
}

bool Runtime::testDialed(ScriptInterp *interp, const char *v)
{
	const char *did = getRegistryId(interp->getSymbol("session.dialed"));

	if(!did)
		return false;

	return matchDigits(did, v, false);
}

bool Runtime::testCaller(ScriptInterp *interp, const char *v)
{
    const char *cid = getRegistryId(interp->getSymbol("session.caller"));

    if(!cid)
        return false;

    return matchDigits(cid, v, false);
}

bool Runtime::testPattern(ScriptInterp *interp, const char *v)
{
	BayonneSession *s = (BayonneSession *)(interp);

	return matchDigits(s->getDigits(), v, false);
} 

bool Runtime::testVoice(ScriptInterp *interp, const char *v)
{
	BayonneSession *s = (BayonneSession *)(interp);
	return (s->audio.getVoicelib(v) != NULL);
}

bool Runtime::testLevel(ScriptInterp *interp, const char *v)
{
	if(!stricmp(v, Bayonne::getRunLevel()))
		return true;

	return false;
}

bool Runtime::testLang(ScriptInterp *interp, const char *v)
{
	BayonneSession *s = (BayonneSession *)(interp);
	BayonneTranslator *t = s->getTranslator();
	const char *id = t->getId();

	if(!id[2] || id[2] == '_')
	{
		if(!strnicmp(id, v, 2))
			return true;
	}
	else if(!stricmp(id, v))
		return true;

	return false;
}	

#ifdef	WIN32

void Runtime::process(void)
{
	Thread::sleep(TIMEOUT_INF);
}

#else

static int fifo = -1;

static void readline(char *buf, unsigned max)
{
        unsigned count = 0;
        *buf = 0;

        --max;

        while(count < max)
        {
                if(read(fifo, buf + count, 1) < 1)
                        break;

                if(buf[count] == '\n')
                        break;

                ++count;
        }
        buf[count] = 0;
}

bool Runtime::command(char *cmd)
{
	Event event;
	char *tok;
	const char *sym, *val;
	static fstream *logout = NULL;
	BayonneSession *s;
	const char *cp;
	bool rtn;
	const char *dial, *caller, *display;
	char buf[65];
	const char *args[65];
	int argc = 0;

	cmd = strtok_r(cmd, " \t\r\n", &tok);
	if(!cmd || ispunct(*cmd) || !*cmd)
		return true;
	
	if(!stricmp(cmd, "down"))
	{
		Bayonne::down();
		return true;
	}

	if(!stricmp(cmd, "start"))
	{
		dial = strtok_r(NULL, " \t\r\n", &tok);

		cmd = strtok_r(NULL, " \t\r\n", &tok);
		if(!cmd || !dial)
			return false;

		caller = strtok_r(NULL, " \t\r\n", &tok);
		display = strtok_r(NULL, " \t\r\n", &tok);
		s = startDialing(dial, cmd, caller, display, NULL, "server");
		if(!s)
			return false;
		s->leave();
		return true;
	}	

	if(!strnicmp(cmd, "lang", 4))
	{
		cmd = strtok_r(NULL, " \t\r\n", &tok);
		while(cmd)
		{
			BayonneTranslator::loadTranslator(cmd);
			cmd = strtok_r(NULL, " \t\r\n", &tok);
		}
		return true;
	}
		
        if(!stricmp(cmd, "define"))
        {
                cmd = strtok_r(NULL, " \t\r\n", &tok);
                if(!cmd)
                        return false;

                snprintf(buf, sizeof(buf), "%s.def", cmd);
                Bayonne::server->setValue("definitions", buf);
                return true;
        }

	if(!stricmp(cmd, "global") || !stricmp(cmd, "symbol"))
	{
		sym = strtok_r(NULL, " \t\r\n", &tok);
		cmd = strtok_r(NULL, " \t\r\n", &tok);
		val = strtok_r(NULL, " \t\r\n", &tok);

		if(!sym || !cmd)
			return false;

		if(!val && stricmp(cmd, "clear"))
			return false;
		
		if(!stricmp(cmd, "set"))
			return BayonneSession::setGlobal(sym, val);
		else if(!stricmp(cmd, "size"))
			return BayonneSession::sizeGlobal(sym, atoi(val));
		else if(!stricmp(cmd, "add"))
			return BayonneSession::addGlobal(sym, val);
		else if(!stricmp(cmd, "clear"))
			return BayonneSession::clearGlobal(sym);
		else
			return false;
	}

	if(!stricmp(cmd, "load"))
	{
		cmd = strtok_r(NULL, " \t\r\n", &tok);
		if(!cmd)
			return false;
		rtn = Bayonne::loadPlugin(cmd);
		if(rtn)
			BayonneService::start();
		return rtn;
	}

	if(!strnicmp(cmd, "log", 3))
	{
		cmd = strtok_r(NULL, " \t\r\n", &tok);
		if(!cmd)
			return false;
		
		memset(&event, 0, sizeof(event));
		if(!stricmp(cmd, "off") || !stricmp(cmd, "none"))
			event.id = DISABLE_LOGGING;
		else
		{
			event.id = ENABLE_LOGGING;
			event.debug.logstate = cmd;
#ifndef	WIN32
			if(getppid() > 1)
			{
				event.debug.output = &cerr;
				goto skipcerr1;
			}
#endif
			if(!logout)
			{
				cp = runtime->getLast("evtlog");
				logout = new fstream(cp, fstream::app);
			}

			event.debug.output = logout;
skipcerr1:
			while(NULL !=(cmd = strtok_r(NULL, " \t\r\n", &tok)))
			{
				s = getSid(cmd);
				if(s)
					s->postEvent(&event);	
			}
		}
	}

	if(!stricmp(cmd, "service"))
	{
		cmd = strtok_r(NULL, " \t\r\n", &tok);
		if(!cmd)
			return false;
		return service(cmd);
	}

	if(!stricmp(cmd, "submit"))
	{
		cmd = strtok_r(NULL, " \t\r\n", &tok);
		if(!cmd)
			return false;

		args[argc++] = cmd;
		while(NULL != (cmd = strtok_r(NULL, " \t\r\n", &tok)) && argc < 60)
			args[argc++] = cmd;

		args[argc] = NULL;
		if(BayonneBinder::submitRequest(args))
			return true;
		return false;
	}
		
	if(!stricmp(cmd, "reload"))
	{
		Bayonne::reload();
		return true;
	}

	slog.error("fifo: unknown command %s", cmd);
	return false;
}
	
void Runtime::process(void)
{
	const char *argv0 = keyserver.getLast("argv0");
	FILE *fp;
	char buf[256];
	char *cp;
	int pid;

	snprintf(buf, sizeof(buf), "%s/provision.conf", keypaths.getLast("config"));
	if(!isFile(buf))
		snprintf(buf, sizeof(buf), "%s/startup.conf", keypaths.getLast("config"));
	
	fp = fopen(buf, "r");
	if(fp)
	{
		for(;;)
		{
			if(!fgets(buf, sizeof(buf), fp) || feof(fp))
				break;
			cp = buf;
                	while(isspace(*cp))
                        	++cp;

			if(*cp == '[')
				break;

			if(!isalpha(*cp))
				continue;

			command(cp);                               
		}
		fclose(fp);
	}

	snprintf(buf, sizeof(buf), "%s/%s.ctrl", keypaths.getLast("runfiles"), argv0);
	remove(buf);
	mkfifo(buf, 0770);
	fifo = ::open(buf, O_RDWR);
	keypaths.setValue("runfifo", buf);
	if(fifo < 0)
	{
		Bayonne::errlog("error", "cannot open control fifo %s", buf);
		Thread::sleep(TIMEOUT_INF);
	}
	for(;;)
	{
		readline(buf, sizeof(buf));
		cp = buf;
		while(isspace(*cp))
			++cp;

		pid = 0;
		if(isdigit(*cp))
		{
			pid = atoi(cp);
			while(isdigit(*cp))
				++cp;
		}
		if(!pid)
		{
			command(cp);
			continue;
		}

		if(command(cp))
			kill(pid, SIGUSR1);
		else
			kill(pid, SIGUSR2);
	}
}

#endif

bool Runtime::loadBinder(const char *path)
{
        char pathbuf[256];
        const char *cp, *kv;
        const char *prefix = NULL;
        DSO *dso;  

#ifdef  WIN32
        if(!prefix)
	 	prefix = "C:\\Program Files\\Common Files\\GNU Telephony\\Bayonne Binders";
#else
        if(!prefix)
                prefix = LIBDIR_FILES;
#endif 

#ifdef  WIN32
        snprintf(pathbuf, sizeof(pathbuf), "%s/%s%s", prefix, path, RLL_SUFFIX);
#else
        snprintf(pathbuf, sizeof(pathbuf), "%s/%s.bin", prefix, path);
#endif  

        cp = path;
        path = pathbuf;

        kv = getLast(path); 

        if(kv)
        {
                if(!stricmp(kv, "loaded"))
                        return true;
                return false;
        }

        if(!canAccess(path))
        {
                Bayonne::errlog("access", "cannot load  %s", path);
                return false;
        }  

        dso = new DSO(path);
        if(!dso->isValid())
        {
                kv = dso->getError();
                setValue(path, kv);
                Bayonne::errlog("error", "cannot initialize %s", path);
                return false;
        }
        setValue(path, "loaded");
	ripple = false;
        return true;
}                     
		
Runtime runtime;

} // namespace

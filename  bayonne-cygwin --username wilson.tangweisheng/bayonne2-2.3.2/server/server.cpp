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

using namespace server;
using namespace ost;
using namespace std;

#ifdef	WIN32

static bool test = true;

static SERVICE_STATUS_HANDLE hStatus = 0;
static SERVICE_STATUS sStatus;
static HANDLE hDown;

static BOOL WINAPI stop(DWORD code)
{
        if(code == CTRL_LOGOFF_EVENT)
                return TRUE;

	BayonneService::stop();
	BayonneDriver::stop();
	ScriptBinder::shutdown();

	if(code)
		Bayonne::errlog("failed", "server exiting; reason=%d", code);
	else
		Bayonne::errlog("notice", "server exiting; normal termination");

	return TRUE;
}

#else
static int mainpid;

static RETSIGTYPE stop(int signo)
{
	const char *cp;

	if(signo > 0 && getpid() != mainpid)
	{
		kill(mainpid, signo);
		return;
	}

        signal(SIGINT, SIG_IGN);
        signal(SIGABRT, SIG_IGN);
        signal(SIGTERM, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);

	cp = keypaths.getLast("runfifo");
	if(cp)
		remove(cp);

	BayonneService::stop();
	BayonneDriver::stop();
	ScriptBinder::shutdown();

	purgedir(keypaths.getLast("tmp"));
	purgedir(keypaths.getLast("tmpfs"));

        if(signo == -2)
                Bayonne::errlog("failed", "server exiting; no timeslots allocated");
        else if(signo)
                Bayonne::errlog("failed", "server exiting; reason=%d", signo);
        else
                Bayonne::errlog("notice", "server exiting; normal termination");

#ifdef	HAVE_LIBEXEC
	cp = Process::getEnv("LIBEXEC");
	if(!cp || strnicmp(cp, "no", 2))
		BayonneSysexec::cleanup();
#endif

	Thread::sleep(100);

	::exit(signo);
}
#endif

static void version(void)
{
        cout << VERSION << endl;
        exit(0);
}

static void plugins(void)
{
	char *tok, *cp, *sp = (char *)keyserver.getLast("plugins");
	if(sp && !strnicmp(sp, "no", 2))
		sp = NULL;
		
	if(sp && !*sp)
		sp = NULL;
		
	if(sp)
		cp = strtok_r(sp, " ,;\t", &tok);
	else
		cp = NULL;

	while(cp)
	{
		Bayonne::loadPlugin(cp);
		cp = strtok_r(NULL, " ,;:\t", &tok);
	}	
}	

static void setTimeslots(BayonneDriver *driver)
{
	const char *type = driver->getLast("type");
	if(!type || stricmp(type, "proto"))
		return;
	
	type = driver->getLast("proto");
	if(!type)
		return;

	type = keyengine.getLast(type);
	if(!type || atoi(type) == 0)
		return;

	driver->setInitial("timeslots", type);
}
	
static void loading(void)
{
	char buffer[256];
	char lang[32];
	char ** keys;
	const char *cp;
	char *sp, *tok;
	unsigned count = keyoptions.getCount("modules");
	BayonneDriver *driver, *proto;
	BayonneTranslator *translator = NULL;
	static char vlib[64];
#ifdef	HAVE_LIBEXEC
	size_t bs = 0;
	int pri = 0;
#endif
	unsigned driver_count = 0;

	if(!Process::getEnv("SERVER_SYSEXEC"))
		Process::setEnv("SERVER_SYSEXEC", keypaths.getLast("scripts"), true);
	
	cp = keypaths.getLast("btsexec");
	if(cp && *cp)
		Process::setEnv("SHELL_BTSEXEC", cp, true);

	Process::setEnv("SERVER_PREFIX", keypaths.getLast("datafiles"), true);
       	Process::setEnv("SERVER_PROMPTS", keypaths.getLast("prompts"), true);
       	Process::setEnv("SERVER_SCRIPTS", keypaths.getLast("scripts"), true);
       	Process::setEnv("SERVER_LIBEXEC", keypaths.getLast("libexec"), true);     
       	Process::setEnv("SERVER_SOFTWARE", "bayonne", true);
       	Process::setEnv("SERVER_VERSION", VERSION, true);
       	Process::setEnv("SERVER_TOKEN", " ", true);

#ifdef	HAVE_LIBEXEC
	cp = Process::getEnv("LIBEXEC");
	if(cp && !strnicmp(cp, "no", 2))
		goto noexec;

	cp = keyengine.getLast("gateways");
	if(cp)
		pri = atoi(cp);

	cp = keyengine.getLast("buffers");
	if(cp)
		bs = atoi(cp) * 1024;

	if(getcwd(buffer, sizeof(buffer)) == NULL)
	{
		slog.critical("cannot get current directory; exiting");
		exit(-1);
	}

	BayonneSysexec::allocate(keypaths.getLast("libexec"), bs, pri,
		keypaths.getLast("modexec"));

noexec:
#endif
        cp = keyserver.getLast("language");
        if(cp && (strlen(cp) == 2 || cp[2] == '_'))  
		BayonneTranslator::loadTranslator(cp);  

        cp = keyserver.getLast("voice");
        if(cp && strchr(cp, '/'))
        {
                setString(lang, sizeof(lang), cp);
                sp = strchr(lang, '/');
                if(sp)
                        *sp = 0;
                if(strlen(lang) == 2 || sp[2] == '_')
                        translator = BayonneTranslator::loadTranslator(lang);

	}

	if(cp && translator)
	{
		snprintf(buffer, sizeof(buffer), "%s/%s", keypaths.getLast("prompts"), cp);
		if(isDir(buffer))
		{
			slog.info("using %s as default voice library", cp);
			Bayonne::init_translator = translator;
			Bayonne::init_voicelib = cp;
		}
		else if(cp[2] == '_')
		{
			vlib[0] = cp[0];
			vlib[1] = cp[1];
			cp = strchr(cp, '/');
			if(!cp)
				cp = "/default";
			snprintf(vlib + 2, sizeof(vlib) - 2, "%s", cp);
			snprintf(buffer, sizeof(buffer), "%s/%s",
				keypaths.getLast("prompts"), vlib);
			if(isDir(buffer))
			{
                        	slog.info("using %s as default voice library", cp);
                        	Bayonne::init_translator = translator;
                        	Bayonne::init_voicelib = vlib; 
			}
		}
        } 

	driver = NULL;	
	cp = keyengine.getLast("driver");
#ifndef	WIN32
	if(cp && !stricmp(cp, "autodetect"))
	{
		cp = Process::getEnv("DRIVER_AUTODETECT");
		if(!cp || !*cp)
			cp = "default";
	}
	if(cp && !stricmp(cp, "default"))
	{
		cp = Process::getEnv("DRIVER");
		if(!cp || !*cp)
			cp = "soundcard";
	}
#endif
	if(cp && *cp)
		driver = BayonneDriver::loadDriver(cp);

	if(driver)
	{
		setTimeslots(driver);
		++driver_count;
	}

	proto = NULL;
	cp = keyengine.getLast("protocol");
	if(!cp)
		cp = keyengine.getLast("protocols");

#ifndef	WIN32
	if(cp && !stricmp(cp, "default"))
	{
		cp = Process::getEnv("PROTOCOL");
		if(!cp || !*cp)
			cp = "none";
	}
#endif
	if(cp && !stricmp(cp, "none"))
		cp = NULL;

	if(cp)
	{
		setString(buffer, sizeof(buffer), cp);
		sp = strtok_r(buffer, ",;:\t \r\n", &tok);
		while(sp)
		{
			proto = BayonneDriver::loadDriver(cp);
			if(proto)
			{
				++driver_count;
				setTimeslots(proto);
			}
			sp = strtok_r(NULL, ",;:\t \r\n", &tok);
		}
	}

	if(!driver_count)
	{
		slog.critical("no drivers loaded; exiting...");
		exit(-1);
	}
		
	sp = (char *)keyserver.getLast("monitoring");
	if(sp && !strnicmp(sp, "no", 2))
		sp = NULL;

	if(sp && !*sp)
		sp = NULL;

	if(sp)
		cp = strtok_r(sp, " ,;:\t", &tok);
	else
		cp = NULL;

	while(cp)
	{
		Bayonne::loadMonitor(cp);
		cp = strtok_r(NULL, " ,;:\t", &tok);
	}

	if(!count)
		goto start;

	keys = (char **)keyoptions.getList("modules");

	while(*keys)
		Bayonne::loadPlugin(*keys++);

start:
	if(!BayonneDriver::getPrimary())
	{
		slog.critical("bayonne: no drivers loaded");
		stop(-1);
	}

	BayonneDriver::start();
	count = Bayonne::getTimeslotsUsed();
	if(!count)
		stop(-2);

	count = Bayonne::getTimeslotsUsed();

#ifdef	HAVE_LIBEXEC
	cp = Process::getEnv("LIBEXEC");
	if(!cp || strnicmp(cp, "no", 2))
		BayonneSysexec::startup();
#endif
// reload
	cp = keyserver.getLast("binding");
#ifdef	WIN32
	if(cp && !stricmp(cp, "default"))
		cp = "ivrscript";
#else
	if(cp && !stricmp(cp, "default"))
	{
		cp = Process::getEnv("BINDING");
		if(!cp || !*cp)
			cp = "ivrscript";
	}
#endif
	plugins();
	runtime.loadBinder(cp);
	Bayonne::reload();
	if(!Bayonne::compile_count)
	{
		Bayonne::errlog("critical", "no applications defined");
		stop(-1);
	}
	Bayonne::errlog("notice", "%d scripts compiled", Bayonne::compile_count);
	BayonneService::start();
	Bayonne::errlog("notice", "driver(s) started; %d timeslot(s) used", count);
#ifndef	WIN32
        Process::setPosixSignal(SIGPIPE, SIG_IGN);
        Process::setPosixSignal(SIGINT, &stop);
        Process::setPosixSignal(SIGHUP, &stop);
        Process::setPosixSignal(SIGTERM, &stop);
        Process::setPosixSignal(SIGABRT, &stop);
#endif
}

static void logging(void)
{
	slog.open("bayonne", Slog::classDaemon);
	const char *level = keyserver.getLast("logging");

	if(!stricmp(level, "notice"))
		slog.level(Slog::levelNotice);
	else if(!stricmp(level, "info"))
		slog.level(Slog::levelInfo);
	else if(!stricmp(level, "error"))
		slog.level(Slog::levelError);
	else if(!stricmp(level, "debug"))
		slog.level(Slog::levelDebug);
}

static void checking(void)
{
	const char *cp = keyserver.getLast("binding");

	keyserver.setValue("logging", "debug");
	logging();
	if(!Bayonne::getTimeslotCount())
		Bayonne::allocate(1, dynamic_cast<ScriptCommand *>(&runtime), 0);
	if(cp && !stricmp(cp, "default"))
		cp = "ivrscript";
	plugins();
	runtime.loadBinder(cp);
	Bayonne::reload();
        if(!Bayonne::compile_count)
        {
                Bayonne::errlog("critical", "no applications defined");
                stop(-1);
        }                                                                     
	stop(0);
}

#ifdef	WIN32

static void control(DWORD request)
{
	switch(request)
	{
	case SERVICE_CONTROL_SHUTDOWN:
	case SERVICE_CONTROL_STOP:
		BayonneService::stop();
		BayonneDriver::stop();
		Bayonne::errlog("notice", "server exiting; service shutdown");
		sStatus.dwWin32ExitCode = 0;
		sStatus.dwCurrentState = SERVICE_STOPPED;
		SetServiceStatus(hStatus, &sStatus);
		SetEvent(hDown);
		return;
	}
	SetServiceStatus(hStatus, &sStatus);
}

static VOID service(DWORD argc, LPTSTR *argv)
{
	hStatus = RegisterServiceCtrlHandler("Bayonne", (LPHANDLER_FUNCTION)control);
	hDown = CreateEvent(0, TRUE, FALSE, 0);

	if(!hStatus || !hDown)
		return;

	sStatus.dwServiceType = SERVICE_WIN32;
	sStatus.dwCurrentState = SERVICE_START_PENDING;
	sStatus.dwControlsAccepted = 0;
	sStatus.dwWin32ExitCode = 0;
	sStatus.dwServiceSpecificExitCode = 0;
	sStatus.dwCheckPoint = 0;
	sStatus.dwWaitHint = 500;

	SetServiceStatus(hStatus, &sStatus);

	sStatus.dwCurrentState = SERVICE_RUNNING;
	sStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
	sStatus.dwWin32ExitCode = 0;
	sStatus.dwWaitHint = 0;
	SetServiceStatus(hStatus, &sStatus);

	WaitForSingleObject(hDown, INFINITE);
	return;
};

static SERVICE_TABLE_ENTRY services[] =
{
	{"Bayonne", (LPSERVICE_MAIN_FUNCTION)service},
	{NULL, NULL}
};

#endif

int main(int argc, char **argv)
{
	const char *argv0;
	const char *cp;

	Script::use_macros = true;
	Script::use_prefix = true;
	Script::use_merge = true;

#ifdef	WIN32
	argv0 = "bayonne";
#else
	argv0 = strrchr(argv[0], '/');

	if(argv0)
		++argv0;
	else
		argv0 = argv[0];
#endif

	keyserver.setValue("argv0", argv0);

	if(argc == 2)
	{
		cp = argv[1];
		if(!strncmp(cp, "--", 2))
			++cp;
		
		if(!stricmp(cp, "-version"))
			version();
		else if(!stricmp(cp, "-check"))
		{
                	loadConfig();
                	liveConfig();
                	runtime.setValue("scripts", keypaths.getLast("scripts"));
                	if(keypaths.getLast("addons"))
                    		runtime.setValue("addons", keypaths.getLast("addons"));    
                	runtime.setValue("include", keypaths.getLast("macros")); 
			checking();
		}
	}

#ifdef	WIN32
	SetConsoleTitle("Bayonne");
        SetConsoleCtrlHandler((PHANDLER_ROUTINE)stop, TRUE);
#else
	mainpid = getpid();
	Process::setPosixSignal(SIGPIPE, SIG_IGN);
        Process::setPosixSignal(SIGINT, &stop);
        Process::setPosixSignal(SIGHUP, &stop);
        Process::setPosixSignal(SIGTERM, &stop);
        Process::setPosixSignal(SIGABRT, &stop);
#endif

	flag_daemon = true;

	loadConfig();
	parseConfig(argv);
	liveConfig();
	if(flag_check)
		checking();

	Process::setScheduler(keyengine.getLast("scheduler"));
        Process::setPriority(atoi(keyengine.getLast("priority")));
	cp = keyserver.getLast("community");
	if(cp)
		Bayonne::trap_community = cp;
	cp = keyserver.getLast("traps");
	if(!cp)
		cp = keyserver.getLast("trap4");
	if(cp)
		Bayonne::addTrap4(cp);
#ifdef	CCXX_IPV6
	cp = keyserver.getLast("trap6");
	if(cp)
		Bayonne::addTrap6(cp);
#endif   
#ifndef WIN32
        if(!getuid())
        {
                if(!Process::setUser(keyserver.getLast("user")))
                {
                        Bayonne::errlog("fatal", "%s: cannot set user",
                                keyserver.getLast("user"));
                        stop(-1);
                }
				Process::setEnv("USER", keyserver.getLast("user"), true);
				Process::setEnv("HOME", keypaths.getLast("datafiles"), true);
        }
	else
		Process::setGroup(keyserver.getLast("group"));
	cp = keyengine.getLast("driver");
	if(cp && !stricmp(cp, "autodetect"))
	{
		cp = Process::getEnv("DRIVER_AUTODETECT");
		if(!cp || !*cp)
			cp = Process::getEnv("DRIVER");
		if(!cp || !*cp)
			flag_daemon = false;
	}
	if(flag_daemon)
		Process::detach();

        mainpid = getpid();
#endif
	logging();
        Bayonne::errlog("notice", "starting %s on %s %s; timeslots=%s",
                VERSION,
                keyserver.getLast("cpu"), keyserver.getLast("platform"),
                keyengine.getLast("timeslots"));   

	loading();

	Runtime::process();
	return 0;
}

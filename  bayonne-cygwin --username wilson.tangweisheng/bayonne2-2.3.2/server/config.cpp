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
#include <sys/utsname.h>
#endif

namespace server {
using namespace ost;
using namespace std;

#ifdef	WIN32
#define	PROMPT_FILES	"C:\\Program Files\\GNU Telephony\\Phrasebook Audio"
#define	SCRIPT_FILES	"C:\\Program Files\\GNU Telephony\\Bayonne Scripts"
#define	MACRO_FILES	"C:\\Program Files\\GNU Telephony\\Bayonne Macros"
#define	VAR_FILES	"C:\\Program Files\\GNU Telephony\\Bayonne Files"
#define	LOG_FILES	"C:\\Program Files\\GNU Telephony\\Bayonne Logs"
#define	CONFIG_FILES	"C:\\Program Files\\GNU Telephony\\Bayonne Config"
#define	RUN_FILES	"C:\\Program Files\\GNU Telephony\\Bayonne Logs"
#define	LIBEXEC_FILES	"C:\\Program Files\\GNU Telephony\\Bayonne Libxec"

#ifdef	_DEBUG
#define	LIBDIR_FILES	"C:\\Program Files\\GNU Telephony\\CAPE Framework\\debug"
#define	MODULE_FILES	LIBDIR_FILES
#define	DRIVER_FILES	LIBDIR_FILES
#define	PHRASE_FILES	LIBDIR_FILES	
#else	
#define	LIBDIR_FILES	"C:\\Program Files\\Common Files\\GNU Telephony\\Runtime"
#define	MODULE_FILES	"C:\\Program Files\\Common Files\\GNU Telephony\\Bayonne Plugins"
#define	DRIVER_FILES	"C:\\Program Files\\Common Files\\GNU Telephony\\Bayonne Drivers"
#define	PHRASE_FILES	"C:\\Program Files\\Common Files\\GNU Telephony\\Bayonne Languages"
#endif

#else
#define	MODULE_FILES	LIBDIR_FILES
#define	DRIVER_FILES	LIBDIR_FILES
#define	PHRASE_FILES	LIBDIR_FILES
#endif

static Keydata::Define keypad[] = {
	{"2", "abc"},
	{"3", "def"},
	{"4", "ghi"},
	{"5", "jkl"},
	{"6", "mno"},
	{"7", "pqrs"},
	{"8", "tuv"},
	{"9", "wxyz"},
	{NULL, NULL}}; 

static Keydata::Define paths[] = {
	{"libexec", LIBEXEC_FILES},
	{"datafiles", VAR_FILES},
	{"logfiles", LOG_FILES},
	{"runfiles", RUN_FILES},
	{"config", CONFIG_FILES},
	{"scripts", SCRIPT_FILES},
	{"macros", MACRO_FILES},
	{"prompts", PROMPT_FILES},
#ifndef	WIN32
	{"btsexec", BIN_FILES "/btsexec"},
#endif
	{NULL, NULL}};

static Keydata::Define server[] = {
	{"user", "bayonne"},
	{"group", "bayonne"},
	{"language", "none"},
	{"location", "us"},
	{"voice", "none/prompts"},
	{"state", "init"},
	{"logging", "warn"},
	{"monitoring", "tcp"},
	{"binding", "default"},
	{"definitions", "url"},
	{NULL, NULL}};

static Keydata::Define engine[] = {
	{"priority", "0"},
	{"scheduler", "default"},
	{"events", "32"},
	{"server", "localhost"},
	{"userid", ""},
	{"secret", ""},
	{"timeslots", "32"},
	{"driver", "autodetect"},
	{"protocol", "none"},
	{NULL, NULL}};

static Keydata::Define libkeys[] = {
	{"filesystem", VAR_FILES},
	{"interface", "eth0"},
	{"limit", "0"},
	{"tts", "theta"},
	{"voice", "female"},
	{"ttslimit", "0"},
	{NULL, NULL}};

static Keydata::Define localkeys[] = {
	{"encoding", "ulaw"},
	{"framing", "20"},
	{"ringing", "20s"},
	{"reconnect", "ulaw,alaw,g721,gsm"},
	{NULL, NULL}};

static Keydata::Define remotekeys[] = {
    {"encoding", "gsm"},
    {"framing", "20"},
    {"ringing", "20s"},
    {"reconnect", "ulaw,alaw,g721,gsm"},
    {NULL, NULL}};

static Keydata::Define msgkeys[] = {
	{"extension", ".au"},
	{"encoding", "ulaw"},
	{"framing", "20"},
	{"maxlimit", "600s"},
	{"greeting", "60s"},
	{"new", "7d"},
	{"read", "4w"},
	{"saved", "never"},
    {NULL, NULL}};

static Keydata::Define smtpkeys[] = {
	{"limit", "0"},
	{"sendmail", "/usr/sbin/sendmail"},
	{"sender", "bayonne@localhost.localdomain"},
	{"from", "Bayonne Server <bayonne@localhost.localdomain>"},
	{"errors", "postmaster@localhost.localdomain"}, 
	{NULL, NULL}};

static Keydata::Define urlkeys[] = {
	{"limit", "0"},
	{"prefix", "http://localhost"},
	{"agent", "bayonne/" VERSION},
	{NULL, NULL}};

static class KeymapConfig : public ReconfigKeydata, public Bayonne
{
private:
	virtual void updateConfig(Keydata *cfg);

public:
	KeymapConfig();

}	keymap;

bool flag_daemon = false;
bool flag_check = false;

StaticKeydata keypaths("/bayonne/server/paths", paths);
StaticKeydata keyserver("/bayonne/server/server", server);
StaticKeydata keyengine("/bayonne/server/engine", engine);
BayonneConfig keylibexec("libexec", libkeys, "/bayonne/server/libexec");
BayonneConfig keysmtp("smtp", smtpkeys, "/bayonne/server/smtp");
BayonneConfig keyurl("url", urlkeys, "/bayonne/server/url");
BayonneConfig keymessage("message", msgkeys, "/bayonne/server/message");
BayonneConfig keylocal("local", localkeys, "/bayonne/server/local");
BayonneConfig keyremote("remote", remotekeys, "/bayonne/server/remote");

Keydata keyoptions;

KeymapConfig::KeymapConfig() :
ReconfigKeydata("/bayonne/server/keymap", keypad)
{
	KeymapConfig::updateConfig(NULL);
}

void KeymapConfig::updateConfig(Keydata *cfg)
{
	char localmap[256];
	char keycode[2];
	unsigned char c, code;
	const char *cp;

	keycode[1] = 0;
	memset(localmap, 0, sizeof(localmap));
	for(c = '0'; c <= '9'; ++c)
	{
		code = (unsigned char)c;
		localmap[code] = c;
		keycode[0] = c;
		cp = updatedString(keycode);
		if(!cp || !*cp)
			continue;
		while(*cp)
		{
			if(isalpha(*cp))
			{
				code = (unsigned char)toupper(*cp);
				localmap[code] = c;
				code = (unsigned char)tolower(*cp);
				localmap[code] = c;
			}
			else
			{
				code = (unsigned char)*cp;
				localmap[code] = c;
			}
			++cp;
		}
	}
	code = (unsigned char)'*';
	localmap[code] = '*';
	code = (unsigned char)'#';
	localmap[code] = '#';
	memcpy(dtmf_keymap, localmap, 256);
}

void liveConfig(void)
{
	const char *cfgid = Process::getEnv("CONFIG");

	char path[256];
	char lang[32];
	const char *env;
	char *p, *sp, *tok;
	const char *cp = cfgid;
	bool ttsdef = false, libdef = false;
	unsigned timeslots = atoi(keyengine.getLast("timeslots"));
	Bayonne::timeslot_t overdraft = 1;

	while(cp && *cp && isalnum(*cp))
		++cp;

	if(cp && *cp)
	{
		slog.critical("invalid config in environment");
		exit(-1);
	}

#ifdef	WIN32
        char nodename[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD namelen = MAX_COMPUTERNAME_LENGTH + 1;
        SYSTEM_INFO sysinfo;
        OSVERSIONINFO osver;
	const char *cpu = "x86";
	const char *tmp = Process::getEnv("TEMP");
	if(!tmp)
		tmp = Process::getEnv("TMP");
	if(!tmp)
		tmp = "C:\\TEMP";
	if(!keypaths.getLast("tmp"))
		keypaths.setValue("tmp", tmp);
	if(!keypaths.getLast("tmpfs"))
		keypaths.setValue("tmpfs", tmp);
	Dir::create(tmp);

        osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx(&osver);
        GetComputerName(nodename, &namelen);
        keyserver.setValue("node", nodename);
		if(cfgid)
			keyserver.setValue("node", cfgid);

        GetSystemInfo(&sysinfo);
        switch(sysinfo.wProcessorArchitecture)
        {
        case PROCESSOR_ARCHITECTURE_INTEL:
		cpu="x86";
                break;
        case PROCESSOR_ARCHITECTURE_PPC:
                cpu = "ppc";
                break;
        case PROCESSOR_ARCHITECTURE_MIPS:
                cpu = "mips";
                break;
        case PROCESSOR_ARCHITECTURE_ALPHA:
                cpu = "alpha";
                break;
        }
	keyserver.setValue("node", nodename);
	keyserver.setValue("platform", "w32");
	keyserver.setValue("cpu", cpu);
#else
	struct utsname uts;
	char hostname[128];
	char *userid;
	uname(&uts);
	memset(hostname, 0, sizeof(hostname));
	gethostname(hostname, sizeof(hostname) - 1);
	hostname[sizeof(hostname) - 1] = 0;
	char *hp = strchr(hostname, '.');
	if(hp)
		*hp = 0;

	if(!keyserver.getLast("node"))
	{
		if(cfgid)
			keyserver.setValue("node", cfgid);
		else
			keyserver.setValue("node", hostname);
	}

	keyserver.setValue("platform", uts.sysname);
	keyserver.setValue("cpu", uts.machine);
	userid = getenv("USER");

	if(!userid || !stricmp(userid, "root"))
		userid="bayonne";	

	if(!cfgid)
		cfgid = userid;

	snprintf(path, sizeof(path), "/tmp/bayonne-%s", cfgid);
	keypaths.setValue("tmp", path);
	purgedir(path);
	Dir::create(path);

	if(canModify("/dev/shm"))
	{
		snprintf(path, sizeof(path), "/dev/shm/bayonne-%s", cfgid);
		keypaths.setValue("tmpfs", path);
		purgedir(path);
		Dir::create(path);
	}
	else
		keypaths.setValue("tmpfs", path);

	if(cfgid == userid)
		cfgid = NULL;

#endif
        sp = (char *)keyserver.getLast("definitions");

	if(sp && !stricmp(sp, "default"))
	{
		sp = NULL;
		cp = Process::getEnv("DEFINES");
		if(cp)
		{
			setString(path, sizeof(path), cp);
			sp = path;
		}
	}

        if(sp && !stricmp(sp, "none"))
                sp = NULL;

	if(sp && !stricmp(sp, "off"))
	{
		libdef = ttsdef = true;
		sp = NULL;
	}

        if(sp && !*sp)
                sp = NULL;
        if(sp)
                cp = strtok_r(sp, " ,;:\t", &tok);
        else
                cp = NULL;

        while(cp)
        {
                if(strrchr(cp, '.'))
                        setString(path, sizeof(path), cp);
                else
                        snprintf(path, sizeof(path), "%s.def", cp);
		if(!stricmp(path, "libexec.def"))
			libdef = true;
		if(!stricmp(path, "tts.def"))
			ttsdef = true;
                runtime.setValue("definitions", path);
                cp = strtok_r(NULL, " ,;:\t", &tok);
        }

#ifdef	HAVE_LIBEXEC
	cp = Process::getEnv("LIBEXEC");
	if(!cp || strnicmp(cp, "no", 2))
	{
		if(!libdef)
			runtime.setValue("definitions", "libexec.def");

		if(!ttsdef)
			runtime.setValue("definitions", "tts.def");
	}
#endif

	cp = keyoptions.getLast("prefix");
	if(cp && *cp)
		chdir(cp);

	env = Process::getEnv("LANG");
	if(env)
	{
		snprintf(lang, sizeof(lang), "%s", env);
		p = strchr(lang, '.');
		if(p)
			*p = 0;
		p = strchr(lang, '_');
		if(p)
		{
			p[1] = tolower(p[1]);
			p[2] = tolower(p[2]);
			if(isalpha(p[1]) && isalpha(p[2]) && !p[3])
				keyserver.setValue("location", ++p);
		}
		lang[0] = tolower(lang[0]);
		lang[1] = tolower(lang[1]);
		if(isalpha(lang[0]) && isalpha(lang[1]))
			keyserver.setValue("language", lang);
	}
#ifdef	WIN32
	snprintf(path, sizeof(path), "%s/tones.ini", keypaths.getLast("config"));
#else
	snprintf(path, sizeof(path), "%s/tones.conf", keypaths.getLast("config"));
#endif
	TelTone::load(path);
	keypaths.setValue("tones", path);

#ifdef	WIN32
#define	EVENTS	"%s/%s.evt"
#define	ERRORS	"%s/%s.err"
#define	OUTPUT	"%s/%s.out"
#define	CALLER	"%s/%s.log"
#define	CALLS   "%s/%s.cdr"
#define	STATS	"%s/%s.sta"
#else
#define	EVENTS	"%s/%s.events"
#define	ERRORS	"%s/%s.errors"
#define	OUTPUT	"%s/%s.output"
#define	CALLER	"%s/%s.sessions"
#define CALLS	"%s/%s.calls"
#define	STATS	"%s/%s.stats"
#endif

	runtime.setValue("node", keyserver.getLast("node"));

	snprintf(path, sizeof(path), ERRORS,
		keypaths.getLast("logfiles"), keyserver.getLast("node"));
	runtime.setValue("errlog", path);

        snprintf(path, sizeof(path), EVENTS,
                keypaths.getLast("logfiles"), keyserver.getLast("node"));
        runtime.setValue("evtlog", path); 

        snprintf(path, sizeof(path), OUTPUT,
                keypaths.getLast("logfiles"), keyserver.getLast("node"));
       	runtime.setValue("output", path);

        snprintf(path, sizeof(path), CALLER,
                keypaths.getLast("logfiles"), keyserver.getLast("node"));
        runtime.setValue("calls", path);

        snprintf(path, sizeof(path), CALLS, 
		keypaths.getLast("logfiles"), keyserver.getLast("node"));
        runtime.setValue("cdr", path);

        snprintf(path, sizeof(path), STATS,
                keypaths.getLast("logfiles"), keyserver.getLast("node"));
        runtime.setValue("stats", path);

	runtime.setValue("include", keypaths.getLast("macros"));
	keypaths.setValue("include", keypaths.getLast("macros"));
	runtime.setValue("tmp", keypaths.getLast("tmp"));
	runtime.setValue("tmpfs", keypaths.getLast("tmpfs"));
	runtime.setValue("prompts", keypaths.getLast("prompts"));
	runtime.setValue("prefix", keypaths.getLast("datafiles"));
	runtime.setValue("config", keypaths.getLast("config"));
	runtime.setValue("voice", keyserver.getLast("voice"));
	runtime.setValue("logfiles", keypaths.getLast("logfiles"));
	runtime.setValue("runfiles", keypaths.getLast("runfiles"));
	runtime.setValue("servername", "bayonne2");
	runtime.setValue("serverversion", VERSION);
	
	cp = keyengine.getLast("server");
	if(cp)
		runtime.setValue("server", cp);
	cp = keyengine.getLast("proxy");
	if(cp)
		runtime.setValue("proxy", cp);

	runtime.setValue("userid", keyengine.getLast("userid"));
	runtime.setValue("secret", keyengine.getLast("secret"));
	runtime.setValue("location", keyserver.getLast("location"));

	cp = keyengine.getLast("autostack");
	if(cp)
		Thread::setStack(atoi(cp) * 1024);

	cp = keyengine.getLast("stepping");
	if(cp)
		Bayonne::step_timer = atoi(cp);

	cp = keyengine.getLast("reset");
	if(cp)
		Bayonne::reset_timer = atoi(cp);

	cp = keyengine.getLast("exec");
	if(cp)
		Bayonne::exec_timer = atoi(cp) * 1000;

	cp = keyengine.getLast("autostep");
	if(cp)
		Script::autoStepping = atoi(cp);

	cp = keyengine.getLast("faststep");
	if(cp)
		Script::fastStepping = atoi(cp);

	cp = keyengine.getLast("paging");
	if(!cp)
		cp = keyengine.getLast("pagesize");
	if(cp)
	{
		Script::pagesize = atoi(cp);
		Script::symlimit = Script::pagesize - sizeof(Script::Symbol) - 32;
	}

	cp = keyengine.getLast("symsize");
	if(cp)
		Script::symsize = atoi(cp);	

	Script::fastStart = false;

#ifdef	WIN32
	Script::exec_extensions = SCRIPT_EXTENSIONS;
#else
	Script::exec_extensions = SCRIPT_EXTENSIONS;
#endif
	Script::exec_token = "bayonne:";

	cp = Process::getEnv("LIBEXEC");
	if(!cp || !*cp)
	{
		cp = keyengine.getLast("libexec");
		if(cp && *cp)
			Process::setEnv("LIBEXEC", cp, true);
	}

	cp = keyengine.getLast("virtual");
	if(cp)
		overdraft = atoi(cp);

        Bayonne::allocate(timeslots, dynamic_cast<ScriptCommand *>(&runtime), overdraft); 
}

void loadConfig(void)
{
	const char *cfgid = Process::getEnv("CONFIG");
	char path[128];

    if(cfgid)
    {
        snprintf(path, sizeof(path), "%s-%s", CONFIG_FILES, cfgid);
        keypaths.setValue("config", path);
        snprintf(path, sizeof(path), "%s-%s", RUN_FILES, cfgid);
        keypaths.setValue("runfiles", path);
    }

#ifdef	WIN32
	char pbuf[256];
	const char *config = keypaths.getLast("config");

//	Process::setGroup(keyserver.getLast("group"));
        Dir::create(keypaths.getLast("datafiles"));
        Dir::create(keypaths.getLast("runfiles"));
        Dir::create(keypaths.getLast("logfiles"));
#else
	char homepath[256];
	const char *home = NULL;
	bool homed = false;

	if(getuid())
		home = Process::getEnv("HOME");

	if(!isDir(keypaths.getLast("config")))
		keypaths.setValue("config", "/bayonne");

	if(home)
		snprintf(homepath, sizeof(homepath), "%s/.bayonne", home);
	else
		strcpy(homepath, "tempfiles");

	umask(007);
	Process::setGroup(keyserver.getLast("group"));
	Dir::create(keypaths.getLast("datafiles"));
	Dir::create(keypaths.getLast("runfiles"));
	Dir::create(keypaths.getLast("logfiles"));

	keypaths.setValue("home", keypaths.getLast("datafiles"));

	if(!getuid())
		goto live;

	if(!canModify(keypaths.getLast("datafiles")))
	{
		homed = true;
		keypaths.setValue("datafiles", homepath);
	}

	keypaths.setValue("control", keypaths.getLast("runfiles"));
	if(!canModify(keypaths.getLast("runfiles")))
	{
		homed = true;
		keypaths.setValue("runfiles", homepath);
	}

	if(!canModify(keypaths.getLast("logfiles")))
	{
		homed = true;
		keypaths.setValue("logfiles", homepath);
	}

	if(homed)
	{
		umask(077);
	        Dir::create(homepath, File::attrPrivate);
	}
live:
#endif
	keyoptions.setValue("prefix", keypaths.getLast("datafiles"));
	keyserver.setValue("state", "live");

#ifdef	SCRIPT_SERVER_PREFIX
	Script::var_prefix = keypaths.getLast("datafiles");
	Script::etc_prefix = keypaths.getLast("config");
	Script::log_prefix = keypaths.getLast("logfiles");
#endif

	// only for live server...
}

bool parseConfig(char **argv)
{
	char buffer[64];
	char path[256];
	const char *prefix;
	char *option;
	char *script = NULL;
	char *ext;
	unsigned scrcount = 0;

#ifndef	WIN32
	const char *cp;
	cp = keyengine.getLast("driver");
	if(cp && !stricmp(cp, "autodetect"))
		cp = Process::getEnv("TIMESLOTS");
	else if(cp && !stricmp(cp, "default"))
		cp = Process::getEnv("TIMESLOTS");

	if(cp && atoi(cp) > 0)
		keyengine.setValue("timeslots", cp);
#endif

	++argv;

retry:
	if(!*argv)
		goto skip;

	option = *argv;


        if(!strcmp("--", option))
        {
                ++argv;
                goto skip;
        }

        if(!strnicmp("--", option, 2))
                ++option;

	if(!strnicmp(option, "-config=", 8))
	{
		runtime.setValue("serverconfig", option + 8);
		++argv;
		goto retry;
	}

	if(!strnicmp(option, "-driver=", 8))
	{
		keyengine.setValue("driver", option + 8);
		++argv;
		goto retry;
	} 

	if(!strnicmp(option, "-protocol=", 10))
	{
		keyengine.setValue("protocol", option + 10);
		++argv;
		goto retry;
	}

        if(!strnicmp(option, "-bind=", 6))
        {
                keyserver.setValue("binding", option + 6);    
                ++argv;
                goto retry;
        }  


	if(!stricmp(option, "-check"))
	{
		keyserver.setValue("logging", "debug");
		flag_check = true;
		++argv;
		goto retry;
	}

	if(!strnicmp(option, "-vvv", 4))
	{
		flag_daemon = false;
		keyserver.setValue("logging", "debug");
		++argv;
		goto retry;
	}

        if(!stricmp(option, "-vv"))
        {
		flag_daemon = false;
                keyserver.setValue("logging", "notice");
                ++argv;
                goto retry;
        }

        if(!stricmp(option, "-v"))
        {
		flag_daemon = false;
                keyserver.setValue("logging", "error");
                ++argv;
                goto retry;
        }

	if(!stricmp(option, "-p"))
	{
		keyengine.setValue("priority", "2");
		keyengine.setValue("scheduler", "fifo");
		++argv;
		goto retry;
	}

	if(!strnicmp(option, "-voice=", 7))
	{
		keyserver.setValue("voice", option + 7);
		++argv;
		goto retry;
	} 

	if(!strnicmp(option, "-location=", 10))
	{ 
		Process::setEnv("LANG", option + 10, true);
		keyserver.setValue("location", option + 10);
		++argv;
		goto retry;
	}

        if(!strnicmp(option, "-timeslots=", 11))
        {
		keyengine.setValue("timeslots", option + 11);
                ++argv;
                goto retry;
        }

        if(!stricmp(option, "-location"))  
        {
                ++argv;
                if(!argv) 
                {
        		cerr << "bayonne: -location: missing argument" << endl;
                        exit(-1);
                }      
		Process::setEnv("LANG", *argv, true);
                keyserver.setValue("location", *(argv++));
                goto retry;
        }


	if(!stricmp(option, "-voice"))
	{
		++argv;
		if(!argv)
		{
			cerr << "bayonne: -voice: missing argument" << endl;
			exit(-1);
		}
		keyserver.setValue("voice", *(argv++));
		goto retry;
	}

        if(!stricmp(option, "-driver"))
        {
                ++argv;
                if(!*argv)
                {
			cerr << "bayonne: -driver: missing argument" << endl;
                        exit(-1);
                }
                keyengine.setValue("driver", *(argv++));
                goto retry;
        }

        if(!stricmp(option, "-config"))
        {
                ++argv;
                if(!*argv)
                {
                        cerr << "bayonne: -config: missing argument" << endl;
                        exit(-1);
                }
                runtime.setValue("serverconfig", *(argv++));
                goto retry;
        }

        if(!stricmp(option, "-protocol"))
        {
                ++argv;
                if(!*argv)
                {
                        cerr << "bayonne: -protocol: missing argument" << endl;
                        exit(-1);
                }
                keyengine.setValue("protocol", *(argv++));
                goto retry;
        }

        if(!stricmp(option, "-bind"))
        {
                ++argv;    
                if(!*argv)
                {
                        cerr << "bayonne: -binding: missing argument" << endl;
                        exit(-1);
                }
                keyserver.setValue("binding", *argv);
		++argv;
                goto retry;
        } 

        if(!stricmp(option, "-timeslots"))
        {
                ++argv;
                if(!*argv)
                {
			cerr << "bayonne: -timeslots: missing argument" << endl;
                        exit(-1);
                }
		keyengine.setValue("timeslots", *argv);
		++argv;
                goto retry;
	}

	if(**argv == '-')
	{
		cerr << "bayonne: " << *argv << ": unknown option" << endl;
		exit(-1);
	}

skip:
	while(*argv)
	{
		ext = strrchr(*argv, '.');
		if(!ext)
			keyoptions.setValue("modules", *argv);
		else if(!stricmp(ext, ".conf") || !stricmp(ext, ".ini"))
			slog.error("bayonne: %s: invalid script file", *argv);
		else if(!stricmp(ext, ".mac") || !stricmp(ext, ".scr") || strstr(Script::exec_extensions, ext) || !stricmp(ext, ".def"))
		{
			prefix = strrchr(*argv, '/');
			if(!prefix)
				prefix = strrchr(*argv, '\\');
			if(!prefix)
				prefix = strrchr(*argv, ':');

			if(prefix)
				++prefix;

			if(!prefix)
			{
				getcwd(path, sizeof(path));
				addString(path, sizeof(path), "/");
				addString(path, sizeof(path), *argv);
				prefix = path;
			}

			if(!canAccess(*argv))
				cerr << "bayonne: " << *argv << ": cannot access" << endl;
			else
			{
				if(!scrcount++)
					script = *argv;
				else
					script = NULL;

				if(!stricmp(ext, ".def"))
					runtime.setValue("definitions", prefix);
				else
					runtime.setValue(*argv, prefix);
			}
		}
		else
		{
			if(canAccess(*argv))
				keyoptions.setValue("configs", *argv);
			else
				cerr << "bayonne: " << *argv << ": cannot access" << endl;
		}
		++argv;
	}

	if(!script) 
	{
		if(!runtime.getLast("scripts"))
			runtime.setValue("scripts", keypaths.getLast("scripts"));
		if(keypaths.getLast("addons"))
			runtime.setValue("addons", keypaths.getLast("addons"));
		return false;
	}

	snprintf(path, sizeof(path), "%s", script);
	ext = strrchr(path, '/');
	if(!ext)
		ext = strrchr(path, '\\');
	if(ext)
		*ext = 0;
	else
		getcwd(path, sizeof(path));
	Process::setEnv("SERVER_SYSEXEC", path, true);
	option = strrchr(script, '/');
	if(!option)
		option = strrchr(script, '\\');
	if(!option)
		option = strrchr(script, ':');
	if(option)
		script = ++option;
	ext = strrchr(script, '.');
	if(ext && strstr(Script::exec_extensions, ext))
		snprintf(buffer, sizeof(buffer), "exec::%s", script);
	else
		setString(buffer, sizeof(buffer), script);
	script = buffer;
	ext = strrchr(script, '.');
	if(ext && !strstr(Script::exec_extensions, ext))
		*ext = 0;

	runtime.setValue("startup", script);
	return true;
}		

void purgedir(const char *dir)
{
	const char *cp;
	if(isDir(dir) && canAccess(dir))
	{
		DirTree dt(dir, 16);
		while(NULL != (cp = dt.getPath()))
			remove(cp);
		remove(dir);
	}
}

} // namespace

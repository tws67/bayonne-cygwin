// Copyright (C) 1999-2001 Open Source Telecom Corporation.
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
//
// As a special exception to the GNU General Public License, permission is
// granted for additional uses of the text contained in its release
// of ccscript.
//
// The exception is that, if you link the ccscript library with other
// files to produce an executable, this does not by itself cause the
// resulting executable to be covered by the GNU General Public License.
// Your use of that executable is in no way restricted on account of
// linking the ccscript library code into it.
//
// This exception does not however invalidate any other reasons why
// the executable file might be covered by the GNU General Public License.
//
// This exception applies only to the code released under the
// name ccscript.  If you copy code from other releases into a copy of
// ccscript, as the General Public License permits, the exception does
// not apply to the code that you add in this way.  To avoid misleading
// anyone as to the status of such modified files, you must delete
// this exception notice from them.
//
// If you write modifications of your own for ccscript, it is your choice
// whether to permit this exception to apply to your modifications.
// If you do not wish that, delete this exception notice.

#include <cc++/config.h>
#include <cc++/misc.h>
#include <cc++/slog.h>
#include <cc++/process.h>
#include <cc++/strchar.h>
#include <cstdlib>
#ifdef	WIN32
#include <process.h>
#else
#include <sys/wait.h>
#endif
#include <iostream>
#include <fstream>
#include "script3.h"

#ifndef	WEXITSTATUS
#define	WEXITSTATUS(x)	(x)
#endif

#ifdef	CCXX_NAMESPACES
using namespace ost;
using namespace std;
#endif

static ofstream output;
static char *sigtype[2] = {NULL, NULL};
static bool exited = false;
static int oddeven = 0;

class shScript : public ScriptRuntime
{
public:
	shScript();
} _image;

class shMethods : public ScriptMethods
{
public:
	bool scrEcho(void);
	bool scrSystem(void);
	bool scrAlarm(void);
	bool scrToken(void);
	bool scrMask(void);
};

enum {
	SIGEXIT = 0
};

class shInterp : public ScriptInterp, public Semaphore
{
private:
	friend class shScript;

	void exitThread(const char *errmsg);

	bool exit(void)
	{
		if(!exiting)
			if(ScriptInterp::exit())
				return true;

		cerr << "exiting..." << endl; detach(); ::exit(0);
	};

public:
	void waitThread(void);

	void signal(const char *sigid)
		{ScriptInterp::signal(sigid);};
	shInterp();
} interp;

class shCompiler : public ScriptCompiler
{
public:
	Name *getScript(const char *name);

	shCompiler();
} shCompiler;

shCompiler::shCompiler() :
ScriptCompiler(&_image, "/ccscript/define")
{
}

Script::Name *shCompiler::getScript(const char *name)
{
	static bool started = false;
	char fname[128], lname[128];
	const char *ext;
	Name *line = ScriptImage::getScript(name);
	if(!line) {
		strcpy(fname, name);
		ext = strrchr(name, '.');
		if(!ext)
			strcat(fname, ".scr");
		if(!canAccess(fname) && !started) {
			cerr << "testscript: " << fname << ": cannot execute" << endl;
			::exit(-1);
		}
		compile(fname);
		if(ext && stricmp(ext, ".scr")) {
			snprintf(lname, sizeof(lname), "exec::%s", name);
			name = lname;
		}
		line = ScriptImage::getScript(name);
		if(!line && !started) {
			cerr << "testscript: " << fname << ": compile failed" << endl;
			::exit(-1);
		}
		if(started)
			return line;
		strcpy(fname, name);
		strcat(fname, ".out");
		output.open(fname, ios::trunc);
		started = true;
		commit();
	}
	return line;
}

shInterp::shInterp() :
ScriptInterp(), Semaphore()
{
}

bool shMethods::scrMask(void)
{
	Line *line = getLine();
	Name *scr = getName();
	char buf[128];

	snprintf(buf, sizeof(buf), "mask: %08lx, line: %08lx",
		scr->mask, line->mask);
	cout << buf << endl;
	advance();
	return true;
}

bool shMethods::scrEcho(void)
{
	const char *val;
	while(NULL != (val = ScriptInterp::getValue(NULL))) {
		output << val;
		cout << val;
	}

	output << endl;
	cout << endl;
	advance();
	return true;
}

void shInterp::exitThread(const char *errmsg)
{
	ScriptInterp::exitThread(errmsg);
	post();
}

void shInterp::waitThread(void)
{
	timeout_t timer = getTimeout();

	if(!timer)
		return;

	if(!wait(timer)) {
		enter();
		if(!updated) {
			if(!ScriptInterp::signal("timeout"))
				error("timeout-expired");
			updated = true;
		}
		leave();
	}
	delete thread;
	thread = NULL;
}

bool shMethods::scrAlarm(void)
{
#ifndef	WIN32
	alarm(atoi(ScriptInterp::getValue("0")));
#endif
	advance();
	return true;
}

bool shMethods::scrToken(void)
{
	unsigned argc = 0;
	Line *line = getLine();

	while(argc < line->argc)
		cout << "[-" << line->args[argc++] << "-] ";

	cout << endl;
	advance();
	return true;
}

bool shMethods::scrSystem(void)
{
	const char *argv[33];
	int argc = 0;
	const char *val;
	int status;
#ifndef	WIN32
	int pid;
#endif

	while(argc < 32 && (NULL != (val = ScriptInterp::getValue(NULL))))
		argv[argc++] = val;

	argv[argc] = NULL;

#ifdef	WIN32
	status = (int)_spawnvp(_P_WAIT, argv[0], argv);
#else
	pid = vfork();
	if(!pid) {
		execvp(*argv, (char **)argv);
		::exit(-1);
	}
#ifdef	__FreeBSD__
	wait4(pid, &status, 0, NULL);
#else
	waitpid(pid, &status, 0);
#endif
	status = WEXITSTATUS(status);
#endif
	if(status)
		error("sys-error");
	else
		advance();
	return false;
}

shScript::shScript() :
ScriptRuntime()
{
	static Script::Define interp[] = {
		{"echo", false, (Method)&shMethods::scrEcho,
			(Check)&ScriptChecks::chkHasArgs},
		{"sys", false, (Method)&shMethods::scrSystem,
			(Check)&ScriptChecks::chkHasArgs},
		{"alarm", false, (Method)&shMethods::scrAlarm,
			(Check)&ScriptChecks::chkHasArgs},
		{"token", true, (Method)&shMethods::scrToken,
			(Check)&ScriptChecks::chkIgnore},
		{"mask", true, (Method)&shMethods::scrMask,
			(Check)&ScriptChecks::chkIgnore},
		{NULL, false, NULL, NULL}};

	trap("alarm");
	trap("hangup");
	trap("timeout");

	load(interp);

	Script::symsize = 64;
	Script::pagesize = 1024;
	Script::symlimit = (unsigned)Script::pagesize - 64;
}

#ifndef	WIN32
static RETSIGTYPE handler(int signo)
{
	switch(signo) {
	case SIGINT:
		if(exited)
			return;
		alarm(0);
		sigtype[oddeven] = "exit";
		break;
	case SIGHUP:
		if(sigtype[oddeven] && stricmp(sigtype[oddeven], "alarm"))
			return;
		alarm(0);
		sigtype[oddeven] = "hangup";
		break;
	case SIGALRM:
		if(!sigtype[oddeven])
			sigtype[oddeven] = "alarm";
		alarm(0);
		break;
	}
}
#endif

int main(int argc, char **argv)
{
	char *ext;
	int stepper;
	char argname[10];
	char lname[128];
	int argcount = 0;
	char *name;

	Script::use_merge = true;

	if(argc < 2)
		::exit(-1);

	if(argc != 2) {
		cerr << "use: testscript scriptname" << endl;
		::exit(-1);
	}

#ifdef	WIN32
	Script::plugins = "../w32/debug";
#else
	if(isDir("../modules/.libs"))
		Script::plugins = "../modules/.libs";
	else
		Script::plugins = "../modules";
#endif

	ext = strrchr(argv[1], '.');
	if(ext)
		if(!stricmp(ext, ".scr"))
			*ext = 0;

	slog("testscript", Slog::classUser, Slog::levelInfo);

	shCompiler.compileDefinitions("script.def");

	_image.aliasModule("str", "string");

	name = argv[1];
	shCompiler.getScript(name);

	if(ext && *ext) {
		snprintf(lname, sizeof(lname), "exec::%s", name);
		name = lname;
	}

	if(!interp.attach(&_image, name))
		::exit(-1);

	while(argcount < argc) {
		snprintf(argname, sizeof(argname), "argv.%d", argcount);
		interp.setConst(argname, argv[argcount++]);
	}
	snprintf(argname, sizeof(argname), "%d", argcount);
	interp.setConst("argv.count", argname);

#ifndef	WIN32
	Process::setPosixSignal(SIGINT, &handler);
	Process::setPosixSignal(SIGHUP, &handler);
	Process::setPosixSignal(SIGALRM, &handler);
#endif

	while(!interp.done()) {
		stepper = oddeven;
		if(oddeven)
			oddeven = 0;
		else
			++oddeven;
		if(sigtype[stepper]) {
			interp.signal(sigtype[stepper]);
			sigtype[stepper] = NULL;
		}
		interp.step();
		interp.waitThread();
	}

	interp.detach();
	::exit(-1);
	return -1;
}


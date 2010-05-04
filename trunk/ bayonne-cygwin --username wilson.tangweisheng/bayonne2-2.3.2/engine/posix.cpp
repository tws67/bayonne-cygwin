// Copyright (C) 2005 David Sugar, Tycho Softworks.
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

#if	defined(HAVE_LIBEXEC) && !defined(WIN32)
#include <cc++/process.h>
#include <cc++/slog.h>
#include <cc++/export.h>
#include "libexec.h"

#include <sys/wait.h>

#define	LIBEXEC_BUFFER 64

using namespace ost;
using namespace std;

int BayonneSysexec::iopair[2] = {-1, -1};
BayonneSysexec *BayonneSysexec::libexec = NULL;
bool BayonneSysexec::exiting = false;

typedef struct
{
	int pid;
	char tsid[16];
}	slot_t;

static slot_t *slot;
static int pid = 0;
static bool err = false;
static int buffers = 1024;
static socklen_t blen = sizeof(buffers);
static const char *libpath;
static const char *syspath;
static int slots;
static int iosock;
static int interval = 5;
static int server_pid = 0;

static void stop(void);

extern "C" {

RETSIGTYPE timer(int signo)
{
	if(server_pid && kill(server_pid, 0))
	{
		if(err)
			fprintf(stderr, "libexec exiting; server lost\n");
		else
			slog.notice("server lost...");
		stop();
		::exit(0);
	}
	alarm(interval);
}

RETSIGTYPE child(int signo)
{
	int status;
	unsigned count;
	char buf[65];
	
	while((pid = wait3(&status, WNOHANG, 0)) > 0)
	{
		count = 0;
		while(count < (unsigned)slots)
		{
			if(slot[count].pid == pid)
				break;
			++count;
		}

		if(count == (unsigned)slots)
		{
			if(err)
				fprintf(stderr, "libexec exiting; unknown pid=%d\n", pid);
			continue;
		}

		if(err)
			fprintf(stderr, "libexec exiting; timeslot=%d, pid=%d, status=%d\n",
				count, pid, WEXITSTATUS(status));

		snprintf(buf, sizeof(buf), "%s exit %d\n",
			slot[count].tsid, WEXITSTATUS(status));

		write(iosock, buf, strlen(buf));
		slot[count].tsid[0] = 0;
		slot[count].pid = 0;
	}
}

};

static void stop(void)
{
	unsigned count = 0;
	if(!slot || !slots)
		return;

	while(count < (unsigned)slots)
	{
		if(slot[count].pid)
			kill(slot[count].pid, SIGTERM);
		++count;
	}
}

BayonneSysexec::BayonneSysexec() :
Thread(0)
{
}

BayonneSysexec::~BayonneSysexec()
{
	exiting = true;
	write(iopair[0], "down\n", 5);
	terminate();
}

void BayonneSysexec::run(void)
{
	BayonneTSession *s;
	Event event;
	char buf[512];
	char *tok;
	const char *cp, *tsid, *cmd, *value;
	char *var;
	int pid, size;
	
	for(;;)
	{
		readline(buf, sizeof(buf));
		if(exiting)
			Thread::sync();

		cp = strtok_r(buf, " \t\r\n", &tok);
		if(!cp || !*cp)
			continue;

		// must be a transaction id

		if(!strchr(cp, '+'))
		{
			slog.error("libexec invalid request; tid=%s", cp);
			continue;
		}

		s = (BayonneTSession *)getSid(cp);

		if(!s)
		{
			slog.notice("libexec invalid or expired transaction; tid=%s", cp);
			continue;
		}

		tsid = cp;
		cmd = strtok_r(NULL, " \t\r\n", &tok);
		if(!cmd)
		{
			slog.error("libexec; command missing");
			continue;
		}

		if(!stricmp(cmd, "start"))
		{
			event.id = ENTER_LIBEXEC;
			event.libexec.tid = tsid;
			cp = strtok_r(NULL, " \t\r\n", &tok);
			pid = event.libexec.pid = atoi(cp);
			cp = strtok_r(NULL, " \t\r\n", &tok);
			event.libexec.fname = cp;
			if(!s->postEvent(&event))
			{
				slog.error("libexec start failed; pid=%d", pid);
				kill(pid, SIGINT);
			}
			else
				SLOG_DEBUG("libexec started; pid=%d, tsid=%s", pid, tsid);
		}
		else if(!stricmp(cmd, "hangup"))
			s->sysHangup(tsid);
		else if(!stricmp(cmd, "args"))
			s->sysArgs(tsid);
		else if(!stricmp(cmd, "head"))
			s->sysHeader(tsid);
                else if(!stricmp(cmd, "read"))
                	s->sysInput(tsid, tok);
		else if(!stricmp(cmd, "xfer"))
		{
			value = strtok_r(NULL, " \t\r\n", &tok);
			if(!value)
				value = "";
			s->sysXfer(tsid, value);
		}
		else if(!stricmp(cmd, "clr"))
		{
                        var = strtok_r(NULL, " \t\r\n", &tok);
                        value = "";
                        size = 0;
                        goto var;
		}
		else if(!stricmp(cmd, "add"))
		{
                        var = strtok_r(NULL, " \t\r\n", &tok);
                        value = strtok_r(NULL, " \t\r\n", &tok);
                        size = -1;
                        goto var;
		}
		else if(!stricmp(cmd, "set"))
		{
                        var = strtok_r(NULL, " \t\r\n", &tok);
                        value = strtok_r(NULL, " \t\r\n", &tok);
			size = 0;
			goto var;
		}
		else if(!stricmp(cmd, "new"))
		{
			var = strtok_r(NULL, " \t\r\n", &tok);
			cp = strtok_r(NULL, " \t\r\n", &tok);
			if(!cp)
				cp = "64";
			size = atoi(cp);
			value = "";
			goto var;
		}			
		else if(!stricmp(cmd, "get"))
		{
			var = strtok_r(NULL, " \t\r\n", &tok);
			value = NULL;
			size = 0;
var:
			s->sysVar(tsid, var, value, size);
		}
                else if(!stricmp(cmd, "post"))
                {
                        var = strtok_r(NULL, " \t\r\n", &tok); 
                        value = strtok_r(NULL, " \t\r\n", &tok);
                        s->sysPost(tsid, var, value);
                } 
		else if(!stricmp(cmd, "flush"))
			s->sysFlush(tsid);
		else if(!stricmp(cmd, "wait"))
			s->sysWait(tsid, tok);
		else if(!stricmp(cmd, "tone"))
			s->sysTone(tsid, tok);
		else if(!stricmp(cmd, "stone"))
			s->sysSTone(tsid, tok);
		else if(!stricmp(cmd, "dtone"))
			s->sysDTone(tsid, tok);
                else if(!stricmp(cmd, "args1"))
                {
                        event.id = ARGS_LIBEXEC;
                        event.libexec.tid = tsid;
                        if(!s->postEvent(&event))
				slog.error("libexec unknown transaction; tsid=%s", tsid);
                        else
                                SLOG_DEBUG("libexec %s; tsid=%s", cmd, tsid);
                }
		else if(!stricmp(cmd, "exit"))
			s->sysExit(tsid, tok);
		else if(!stricmp(cmd, "error"))
			s->sysError(tsid, tok);
		else if(!stricmp(cmd, "record"))
			s->sysRecord(tsid, tok);
		else if(!stricmp(cmd, "replay"))
			s->sysReplay(tsid, tok);
		else if(!stricmp(cmd, "result"))
			s->sysReturn(tsid, strtok_r(NULL, "\r\n", &tok));
		else if(!stricmp(cmd, "prompt") || (strchr(cmd, '/') && strchr(cmd, '/') == strrchr(cmd, '/')))
			s->sysPrompt(tsid, cmd, strtok_r(NULL, "", &tok));
		else
			slog.error("libexec unknown command %s", cmd);
	}
}

void BayonneSysexec::startup(void)
{
	libexec = new BayonneSysexec();
	libexec->start();
}

void BayonneSysexec::readline(char *buf, unsigned max)
{
	unsigned count = 0;
	*buf = 0;

	--max;

	while(count < max)
	{
		if(read(iopair[1], buf + count, 1) < 1)
			break;
		
		if(buf[count] == '\n')
			break;

		++count;
	}
	buf[count] = 0;
}
	
void BayonneSysexec::allocate(const char *libexec, size_t bs, int pri, const char *modpath)
{
	char buf[LIBEXEC_BUFFER];
	int fd = dup(2);
	char path[512];
	char cwd[512];
	char *argv[8];
	char tmp[16];
	const char *le = Process::getEnv("LIBEXEC");

	slots = ts_count;

	if(!le || !*le || strchr(le, '/'))
		le = "libexec";

	if(*libexec == '/')
		libpath = libexec;
	else
	{
		getcwd(cwd, sizeof(cwd));
		chdir(libexec);
		getcwd(path, sizeof(path));
		libpath = strdup(path);
		chdir(cwd);
	}

	if(modpath && *modpath != '/')
	{
		getcwd(path, sizeof(path));
		chdir(modpath);
		getcwd(cwd, sizeof(cwd));
		modpath = cwd;
		chdir(path);
	}
	else if(!modpath)
		modpath = libpath;

	if(bs)
		buffers = bs;

	if(getppid() > 1)
		err = true;

	if(socketpair(AF_UNIX, SOCK_STREAM, 0, iopair))
	{
		slog.error("libexec: cannot create socket pair");
		return;
	}

	setsockopt(iopair[0], SOL_SOCKET, SO_RCVBUF, &buffers, blen);
	setsockopt(iopair[1], SOL_SOCKET, SO_RCVBUF, &buffers, blen);
	iosock = iopair[0];

	pid = fork();
	if(pid)
	{
		Process::join(pid);
		::close(fd);
		snprintf(buf, sizeof(buf), "serv%d", getpid());
		write(iopair[1], buf, sizeof(buf));
		return;
	}

	syspath = Process::getEnv("SERVER_SYSEXEC");
	if(!syspath)
		syspath = Process::getEnv("SERVER_SCRIPTS");
	if(!syspath)
		syspath = libpath;

	Process::detach();
	dup2(iopair[0], 0);
	dup2(iopair[0], 1);
	dup2(fd, 2);
	::close(fd);
	::close(iopair[0]);

	nice(pri);

	if(!stricmp(syspath, modpath))
		snprintf(path, sizeof(path), "%s:%s/bayonne.jar", syspath, LIBDIR_FILES);
	else
		snprintf(path, sizeof(path), "%s:%s/bayonne.jar:%s/bayonne.jar", syspath, modpath, LIBDIR_FILES); 
	Process::setEnv("CLASSPATH", path, true);

	chdir(Process::getEnv("SERVER_PREFIX"));
	getcwd(path, sizeof(path));
	Process::setEnv("SERVER_PREFIX", path, true);	

        Process::setEnv("SERVER_PROTOCOL", "4.0", true); 
        Process::setEnv("SERVER_TMP", path_tmp, true);
        Process::setEnv("SERVER_TMPFS", path_tmpfs, true); 
	snprintf(path, sizeof(path), "%s:/bin:/usr/bin:/usr/local/bin", modpath);
	Process::setEnv("PATH", strdup(path), true);
	Process::setEnv("PERL5LIB", modpath, true);
	Process::setEnv("PYTHONPATH", modpath, true);
	Process::setEnv("SERVER_LIBEXEC", modpath, true);

	slog.open("bayonne", Slog::classDaemon);
	slog.level(Slog::levelInfo);

	slog.info("libexec starting; path=%s", libpath);

	// !exec
	argv[0] = "libexec.bin";
	snprintf(tmp, sizeof(tmp), "%d", slots);
	argv[1] = newString(tmp);
	snprintf(tmp, sizeof(tmp), "%d", LIBEXEC_BUFFER);
	argv[2] = newString(tmp);
	snprintf(tmp, sizeof(tmp), "%d", interval);
	argv[3] = newString(tmp);
	argv[4] = newString(libpath);
	argv[5] = newString(syspath);
	argv[6] = NULL;

	for(int i = 3; i < 100; ++i)
		::close(i);

	snprintf(path, sizeof(path), "%s/%s", LIBDIR_FILES, le);		

	execv(path, argv);
	slog.error("libexec failed; exiting...");
	::exit(-1);
}

bool BayonneSysexec::create(BayonneSession *s)
{
	char buf[LIBEXEC_BUFFER];
	const char *lib;
	Name *scr = s->getName();
	Line *line = s->getLine();

	if(iopair[1] < 0)
		return false;

	if(!strnicmp(line->cmd, "exec", 4) && strstr(scr->name, "::"))
		lib = scr->name;
	else
		lib = s->getValue();

	if(!strnicmp(lib, "libexec::", 9))
		lib += 9; 

	s->newTid();
	snprintf(buf, sizeof(buf), "%04d+%08x %s", 
		s->getSlot(), s->newTid(), lib);
	write(iopair[1], buf, sizeof(buf));
	return true;
}

void BayonneSysexec::cleanup(void)
{
	char buf[LIBEXEC_BUFFER];

	setString(buf, sizeof(buf), "down");

	if(iopair[1] > -1)
		write(iopair[1], buf, sizeof(buf));

	if(libexec)
	{
		delete libexec;
		libexec = NULL;
	}
}

#endif

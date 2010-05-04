// Copyright (C) 2005 Open Source Telecom Corporation.
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

#include <sys/types.h>
#include <sys/param.h>
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif
#ifdef HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#endif
#ifdef HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif

#ifdef HAVE_SYS_STATVFS_H
#define STATFS statvfs
#else
#define STATFS statfs
#endif

#endif

namespace moduleTCP {
using namespace ost;
using namespace std;

Session::Session(TCPSocket &server) :
TCPSession(server)
{
	if(so > -1 && (unsigned)so < (sizeof(fd_set) * 8))
		++Service::sequence[so];
	monmap = new char[ts_used];
	memset(monmap, 0, ts_used);
}

void Session::help(void)
{
	*tcp() << "bye - disconnect session" << endl;
	*tcp() << "calls [sid..] - display active calls" << endl;
	*tcp() << "diskstat - show disk status" << endl;
	*tcp() << "drivers [names..] - display loaded drivers" << endl;
	*tcp() << "global id cmd [val] - manipulate global" << endl;
	*tcp() << "hold sid - hold a running session" << endl;
	*tcp() << "jhold sid - hold a joined session" << endl;
	*tcp() << "mon state|all timeslots.. - enable state monitoring" << endl;
	*tcp() << "nomon timeslots.. - disable state monitoring" << endl;
    *tcp() << "reg - display protocol registrations" << endl;
	*tcp() << "refer sid target - refer bridge leg" << endl;
	*tcp() << "reload - reload script configuration" << endl;
	*tcp() << "release sid - release refer state" << endl;
	*tcp() << "resume sid - resume from hold state" << endl;
	*tcp() << "shutdown - shutdown server" << endl;
	*tcp() << "spans - list active spans in server" << endl;
	*tcp() << "start dial script [num [cid]] - start a script running" << endl;
	*tcp() << "stat - show status line record" << endl;
	*tcp() << "stop sid - stop a running session" << endl;
	*tcp() << "submit script ... - submit job request" << endl;
	*tcp() << "uptime - show current server uptime" << endl;
}

void Session::reg(void)
{
	regauth_t *data;
	unsigned count, idx = 0;
	char buf[80];
	BayonneDriver *d = BayonneDriver::getPrimary();

	if(!d)
	{
		*tcp() << "*** invalid driver or no protocols" << endl;
		return;
	}

	data = new regauth_t[1024];
	count = d->getRegistration(data, 1024);
	if(!count)
	{
		*tcp() << "*** no active registrations" << endl;
		return;
	}

	snprintf(buf, sizeof(buf), "%22s %16s %6s %8s",
		"remote", "userid", "type", "status");
	*tcp() << buf << endl;
	while(idx < count)
	{
		snprintf(buf, sizeof(buf), "%22s %16s %6s %8s",
			data[idx].remote, data[idx].userid,
			data[idx].type, data[idx].status);
		*tcp() << buf << endl;
		++idx;
	}

	delete[] data;
}


void Session::uptime(void)
{
	unsigned long upt = Bayonne::uptime();
	char dur[12];

	if(upt < 100 * 3600)
		snprintf(dur, sizeof(dur), "%02ld:%02ld:%02ld",
			upt / 3600, (upt / 60) % 60, upt % 60);
	else
		snprintf(dur, sizeof(dur), "%ld days", upt / (3600 * 24));	
	
	*tcp() << "uptime: " << dur << endl;
}

void Session::refer(char **list)
{
	BayonneSession *session;
	Event event;
	char *sid = *(list++);
	char *dest = *(list++);

	if(!sid)
	{
		*tcp() << "*** session id missing" << endl;
		return;
	}

	if(!dest)
	{
		*tcp() << "*** destination number missing" << endl;
		return;
	}
	session = getSid(sid);
	if(!session)
	{
		*tcp() << "*** " << sid << " invalid" << endl;
		return;
	}
	session->enterMutex();
	if(!session->isHolding())
	{
		session->leaveMutex();
		*tcp() << "*** " << sid << " not on hold" << endl;
		return;
	}
	if(!session->isJoined())
	{
		session->leaveMutex();
		*tcp() << "*** " << sid << " not joined" << endl;
		return;
	}
	memset(&event, 0, sizeof(event));
	event.id = START_REFER;
	event.dialing = dest;
	if(!session->postEvent(&event))
		*tcp() << "*** " << sid << " cannot refer" << endl;
	else
		*tcp() << "refer started" << endl;
	session->leaveMutex();
}

void Session::monitor(char **list)
{
	BayonneSession *session;
	Event event;
	char *evt = *(list++);
	char *ts;

	while(*list)
	{
		ts = *(list++);
		session = getSid(ts);
		if(!session)
		{
			*tcp() << "*** " << ts << " invalid" << endl;
			continue;
		}

		memset(&event, 0, sizeof(event));
		event.id = ENABLE_LOGGING;
		event.debug.output = tcp();
		event.debug.logstate = evt;
		if(session->postEvent(&event))
			monmap[session->getSlot()] = 'x';
		else
			*tcp() << "*** " << ts << " unavailable" << endl;
	}
}

void Session::nomon(char **list)
{
	BayonneSession *session;
	Event event;
	char *ts;
	timeslot_t pos = 0;

	if(!*list)
	{
		while(pos < ts_used)
		{
			session = getSession(pos);
			if(!monmap[pos] || !session)
			{
				++pos;
				continue;
			}

			monmap[pos++] = 0;
			memset(&event, 0, sizeof(event));
			event.id = DISABLE_LOGGING;
			event.debug.output = tcp();
			session->postEvent(&event);
			*tcp() << (pos - 1) << " monitoring disabled" << endl;
		}			
		return;
	}
	while(*list)
	{
		ts = *(list++);
		session = getSid(ts);
		if(!session)
		{
			*tcp() << "*** " << ts << " invalid" << endl;
			continue;
		}

		memset(&event, 0, sizeof(event));
		event.id = DISABLE_LOGGING;
		event.debug.output = tcp();
		session->postEvent(&event);
		monmap[session->getSlot()] = 0;
		*tcp() << ts << " monitoring disabled" << endl;
	}
}

void Session::calls(char **list)
{
	BayonneSession *session;
	char lr[128];
	const char *caller, *dialed, *display;
	unsigned count = 0;

	while(*list)
	{
		session = getSid(*(list++));
		if(!session)
			continue;
	
		session->enter();
		if(session->isIdle())
		{
			session->leave();
			continue;
		}
		caller = session->getSymbol("session.caller");
		dialed = session->getSymbol("session.dialed");
		display = session->getSymbol("session.display");
		if(!caller)
			caller = "unknown caller";
		if(!dialed)
			dialed = "unknown dialed";
		if(!display)
			display = "";
		snprintf(lr, sizeof(lr), "%-14s %-16s %-16s %-12s %s",
                        session->getExternal("session.deviceid"),
                        session->getExternal("session.id"),
                        session->getExternal("session.pid"),
                        session->getExternal("session.duration"),
                        session->getExternal("session.type")); 
		*tcp() << lr << endl;
		snprintf(lr, sizeof(lr), " %-20s %-20s %s",
			caller, dialed, display);
		*tcp() << lr << endl;
		session->leave();
		++count;
	}
	if(!count)
		*tcp() << "*** no active calls found" << endl;
}

void Session::allcalls(void)
{
	BayonneSession *session;
	char lr[128];
	timeslot_t pos = 0;
	const char *caller, *dialed, *display;
	unsigned count = 0;

	while(pos < ts_used)
	{
		session = getSession(pos++);
		if(!session)
			continue;
	
		session->enter();
		if(session->isIdle())
		{
			session->leave();
			continue;
		}
		caller = session->getSymbol("session.caller");
		dialed = session->getSymbol("session.dialed");
		display = session->getSymbol("session.display");
		if(!caller)
			caller = "unknown caller";
		if(!dialed)
			dialed = "unknown dialed";
		if(!display)
			display = "";
		snprintf(lr, sizeof(lr), "%-14s %-16s %-16s %-12s %s",
                        session->getExternal("session.deviceid"),
                        session->getExternal("session.id"),
                        session->getExternal("session.pid"),
                        session->getExternal("session.duration"),
                        session->getExternal("session.type")); 
		*tcp() << lr << endl;
		snprintf(lr, sizeof(lr), " %-20s %-20s %s",
			caller, dialed, display);
		*tcp() << lr << endl;
		session->leave();
		++count;
	}
	if(!count)
		*tcp() << "*** no active calls found" << endl;
}



void Session::spans(void)
{
	BayonneSpan *span;
	char lr[80];

	unsigned spid = 0;
	if(!BayonneSpan::getSpans())
	{
		*tcp() << "*** no spans active" << endl;
		return;
	}

	while(spid < BayonneSpan::getSpans())
	{
		span = BayonneSpan::get(spid);
		snprintf(lr, sizeof(lr), "%03d %04d %04d", spid,
			span->getFirst(), span->getCount());
		*tcp() << lr << endl;
	}
}

void Session::drivers(char **list)
{
	BayonneDriver *driver;
	char lr[80];
	while(*list)
	{
		driver = BayonneDriver::get(*list);
		snprintf(lr, sizeof(lr), "%-16s %04d %04d %03d %03d", 
			*list, driver->getFirst(), driver->getCount(), 
			driver->getSpanFirst(), driver->getSpansUsed());
		*tcp() << lr << endl;
		++list;
	}
}

void Session::diskstat(char **list)
{
    char lr[80];
    char path[256];
    char label[256];
#ifndef	WIN32
    struct STATFS fs;
    unsigned long long bfree, bsize;
#else
    ULARGE_INTEGER bavail, btotal, bfree;
#endif

    while(*list)
    {
	strcpy(path,*list);
	strcpy(label,*list);
	if(stricmp(label, "datafiles")==0) {
		if (getenv("SERVER_PREFIX")) {
			strcpy(path,getenv("SERVER_PREFIX")); 
		}
	}
	if(stricmp(*list, "prompts")==0) {
		if (getenv("SERVER_PROMPTS")) {
			strcpy(path,getenv("SERVER_PROMPTS")); 
		}
	}
	if(stricmp(*list, "scripts")==0) {
		if (getenv("SERVER_SCRIPTS")) {
			strcpy(path,getenv("SERVER_SCRIPTS"));
		}
	}
	if(stricmp(*list, "libexec")==0) {
		if (getenv("SERVER_LIBEXEC")) {
			strcpy(path,getenv("SERVER_LIBEXEC")); 
		}
	}

#ifdef	WIN32			
	GetDiskFreeSpaceEx(path, &bavail, &btotal, &bfree);
	snprintf(lr, sizeof(lr), "%-15s %llu", label, bavail);
#else
	if (0 == STATFS(path,&fs)) {
    	    bfree = fs.f_bfree;
    	    bsize = fs.f_bsize;
    	    snprintf(lr, sizeof(lr), "%-15s %llu",label,  bfree * bsize);
	    *tcp() << lr << endl;
	} else {
	    snprintf(lr, sizeof(lr), "%-15s %-15s","*** error in label", path);
	    *tcp() << lr << endl;
	}
#endif
	
	++list;
    }

	return;
	*tcp() << "*** Invalid label" << endl;	
}

void Session::run(void)
{
	Event event;
	BayonneSession *session;
	bool login = false;
	int retries = 0;
	char exitbuf[16];
	char buffer[256];
	char *argv[128];
	char *tok;
	char *bp;
	const char *val;
	int argc;
	tpport_t port;
	IPV4Address addr = getLocal(&port);
	const char *name = server->getLast("servername");
	const char *ver = server->getLast("serverversion");
	*tcp() << "welcome to " << name << " " << ver << " monitor at " << addr.getHostname() << endl;
	*tcp() << ts_used << " timeslots active; " << ts_count << " timeslots allocated" << endl;
        *tcp() << "secret: ";
        flush();

	while(Service::active && !login && retries++ < 3)
	{
		buffer[0] = 0;
		if(isPending(pendingInput, 10000))
		{
			getline(buffer, sizeof(buffer));
			if(!buffer[0])
				Thread::exit();
			
			bp = strtok_r(buffer, "\r\n", &tok);
			if(bp)
				Thread::sleep(1000);
			if(bp && !strcmp(bp, Service::tcp.getString("secret")))
				login = true;
			else
			{
				*tcp() << "secret: ";
				flush();
			}
		}
	}

	if(login)
	{
		*tcp() << "-> ";
		flush();
	}

	while(Service::active && login)
	{
		if(isPending(pendingInput, 100))
		{
			buffer[0] = 0;
			getline(buffer, sizeof(buffer));
			if(!buffer[0])
				Thread::exit();

			argc = 0;
			bp = strtok_r(buffer, " \t\r\n", &tok);
			while(bp && argc < 127)
			{
				argv[argc++] = bp;
				bp = strtok_r(NULL, " \t\r\n", &tok);
			}
			argv[argc] = NULL;

			if(!*argv)
			{
			}
			else if(!stricmp(*argv, "quit") || !stricmp(*argv, "bye") || !stricmp(*argv, "goodbye"))
			{
				login = false;
				break;
			}
                        else if(!strnicmp(*argv, "reg", 3))
                        {
                                reg();
                        }
			else if(!stricmp(*argv, "global"))
			{
				if(!argv[1])
				{
					*tcp() << "*** symbol id missing" << endl;
					goto skip;
				}
				if(!argv[2])
				{
					*tcp() << "*** symbol command missing" << endl;
					goto skip;
				}
				if(!stricmp(argv[2], "clear"))
				{
					if(!BayonneSession::clearGlobal(argv[1]))
						*tcp() << "*** cannot find symbol" << endl;
 					goto skip;
				}
				if(!stricmp(argv[2], "get"))
				{
					val = BayonneSession::getGlobal(argv[1]);
					if(!val)
						*tcp() << "*** cannot find symbol" << endl;
					else
						*tcp() << argv[1] << " = [" << val << "]" << endl;
					goto skip;
				}
				if(!argv[3])
				{
					*tcp() << "*** symbol size or value missing" << endl;
					goto skip;
				}
				if(!stricmp(argv[2], "size"))
				{
					if(!BayonneSession::sizeGlobal(argv[1], atoi(argv[3])))
						*tcp() << "*** cannot find symbol" << endl;
					goto skip;
				}
				if(!stricmp(argv[2], "set"))
				{
					if(!BayonneSession::setGlobal(argv[1], argv[3]))
						*tcp() << "*** cannot find symbol" << endl;
					goto skip;
				}
				if(!stricmp(argv[2], "add"))
				{
					if(!BayonneSession::addGlobal(argv[1], argv[3]))
						*tcp() << "*** cannot find symbol" << endl;
					goto skip;
				}
			}	
			else if(!strnicmp(*argv, "nomon", 5))
			{
				nomon(&argv[1]);
			}
			else if(!stricmp(*argv, "refer"))
			{
				refer(&argv[1]);
			}
			else if(!strnicmp(*argv, "mon", 3))
			{
				if(!argv[1])
				{
					*tcp() << "*** events missing to monitor" << endl;
					goto skip;
				}
				if(!argv[2])
				{
					*tcp() << "*** no timeslots listed to monitor" << endl;
					goto skip;
				}
				monitor(&argv[1]);
			}
			else if(!stricmp(*argv, "calls"))
			{
				if(argv[1])
					calls(&argv[1]);
				else
					allcalls();
			}
			else if(!stricmp(*argv, "submit"))
			{
				if(!argv[1])
					*tcp() << "*** missing submit name" << endl;
				if(!BayonneBinder::submitRequest((const char **)&argv[1]))
					*tcp() << "*** submit request failed" << endl;
			}
			else if(!stricmp(*argv, "spans"))
				spans();
			else if(!stricmp(*argv, "drivers"))
			{
				if(!argv[1])
					BayonneDriver::list(&argv[1], 120);
				drivers(&argv[1]);
			}
			else if(!stricmp(*argv, "start"))
			{
				if(!argv[1])
				{
					*tcp() << "*** dial destination missing" << endl;
					goto skip;
				}

				if(!argv[2])
				{
					*tcp() << "*** missing script name" << endl;
					goto skip;
				}

				session = Bayonne::startDialing(argv[1], 
					argv[2], argv[3], argv[4], NULL, "tcpmon");
				if(!session)
					*tcp() << "*** session unable to start call" << endl;
				else
				{
					if((unsigned)so < (sizeof(fd_set) * 8))
					{
						snprintf(exitbuf, sizeof(exitbuf), "tcpmon:%d,%d",
							so, Service::sequence[so]);
						session->setConst("exit_manager", exitbuf);
					}
					*tcp() << "starting call " << session->getExternal("session.id") << endl;
					session->leave();
				}
			}
			else if(!stricmp(*argv, "stop"))
			{
				if(!argv[1])
				{
					*tcp() << "*** missing session id" << endl;
					goto skip;
				}
				session = getSid(argv[1]);
				if(!session)
				{
					*tcp() << "*** invalid or unknown call session" << endl;
					goto skip;
				}
				memset(&event, 0, sizeof(event));
				event.id = STOP_SCRIPT;
				if(session->postEvent(&event))
					*tcp() << "script stopped" << endl;
				else
					*tcp() << "*** stop ignored" << endl;
			}
			else if(!stricmp(*argv, "hold"))
			{
				if(!argv[1])
				{
					*tcp() << "*** missing session id" << endl;
					goto skip;
				}
				session = getSid(argv[1]);
				if(!session)
				{
					*tcp() << "*** invalid or unknown call session" << endl;
					goto skip;
				}
				memset(&event, 0, sizeof(event));
				event.id = CALL_HOLD;
				if(session->postEvent(&event))
					*tcp() << "script held" << endl;
				else
					*tcp() << "*** hold ignored" << endl;
			}
			else if(!stricmp(*argv, "release"))
			{
				if(!argv[1])
				{
					*tcp() << "*** missing session id" << endl;
					goto skip;
				}
				session = getSid(argv[1]);
				if(!session)
				{
					*tcp() << "*** invalid or unknown call session" << endl;
					goto skip;
				}
				memset(&event, 0, sizeof(event));
				event.id = DROP_RECALL;
				if(session->postEvent(&event))
					*tcp() << "script released" << endl;
				else
					*tcp() << "*** release ignored" << endl;
			}
			else if(!stricmp(*argv, "jhold"))
			{
				if(!argv[1])
				{
					*tcp() << "*** missing session id" << endl;
					goto skip;
				}
				session = getSid(argv[1]);
				if(!session)
				{
					*tcp() << "*** invalid or unknown call session" << endl;
					goto skip;
				}
				memset(&event, 0, sizeof(event));
				event.id = CALL_HOLD;
				session->enter();
				if(!session->isJoined())
					*tcp() << "*** session not joined" << endl;
				else if(session->postEvent(&event))
					*tcp() << "script held" << endl;
				else
					*tcp() << "*** hold ignored" << endl;
				session->leave();
			}
			else if(!stricmp(*argv, "resume"))
			{
				if(!argv[1])
				{
					*tcp() << "*** missing session id" << endl;
					goto skip;
				}
				session = getSid(argv[1]);
				if(!session)
				{
					*tcp() << "*** invalid or unknown call session" << endl;
					goto skip;
				}
				memset(&event, 0, sizeof(event));
				event.id = CALL_NOHOLD;
				if(session->postEvent(&event))
					*tcp() << "script resumed" << endl;
				else
					*tcp() << "*** resume ignored" << endl;
			}

			else if(!strnicmp(*argv, "stat", 4))
				*tcp() << status << endl;
			else if(!stricmp(*argv, "uptime"))
				Session::uptime();
			else if(!stricmp(*argv, "reload"))
				Bayonne::reload();
			else if(!stricmp(*argv, "diskstat")) {
			    if(!argv[1])
			    {
				*tcp() << "*** disk label missing" << endl;
				goto skip;
			    }
				diskstat(&argv[1]);
			}	
			else if(!stricmp(*argv, "help"))
				help();
			else if(!stricmp(*argv, "shutdown"))
			{
				Service::active = false;
				login = false;
				Bayonne::down();
				break;
			}
			else
				*tcp() << "*** unknown command" << endl;

skip:			
			*tcp() << "-> ";
			flush();
		}
		Thread::yield();
	}
	*tcp() << "goodbye" << endl;
	Thread::exit();
}

void Session::final(void)
{
	Event event;
	timeslot_t pos = 0;
	BayonneSession *session;

	while(pos < ts_used)
	{
		if(monmap[pos])
		{
			session = getSession(pos);
			if(session)
			{
				memset(&event, 0, sizeof(event));
				event.id = DISABLE_LOGGING;
				event.debug.output = tcp();
				session->postEvent(&event);
			}
		}
		++pos;
	}

	delete[] monmap;
}

} // namespace 

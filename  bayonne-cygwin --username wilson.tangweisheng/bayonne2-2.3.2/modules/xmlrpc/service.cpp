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

#include <engine.h>

namespace moduleXMLRPC {
using namespace ost;
using namespace std;

static class Service : public BayonneService, public BayonneRPC, public StaticKeydata
{
private:
	void run(void);
	void stopService(void);
	void detachSession(BayonneSession *session);

public:
	Service();
} xmlrpc;

extern bool xmlrpc_update;
extern const char *getExit(const char *sid);

static bool active = false;
static char rpcpath[128] = {0};
static int so = INVALID_SOCKET;
bool xmlrpc_update = false;

typedef struct
{
	char session[16];
	char reason[64];
}	exit_t;


static exit_t *exit_list = NULL;
static unsigned exit_mark = 0;
static unsigned exit_count = 0;
static Mutex exit_lock;

const char *getExit(const char *sid)
{
	unsigned pos = 0;

	if(!sid || !*sid || !exit_count || !exit_list)
		return "none";

	while(pos < exit_count)
	{
		if(!stricmp(exit_list[pos].session, sid))
			return exit_list[pos].reason;
		++pos;
	}
	return "none";
}	

static Keydata::Define defkeys[] = {
	{"local", "yes"},
	{"send", "64"},
	{"recv", "16"},
	{"timer", "60"},
	{"exits", "0"},
	{NULL, NULL}};

Service::Service() :
BayonneService(0, 0), BayonneRPC(), 
StaticKeydata("/bayonne/xmlrpc", defkeys)
{
	exit_count = getValue("exits");
	if(exit_count)
	{
		exit_list = new exit_t[exit_count];
		memset(exit_list, 0, sizeof(exit_t) * exit_count);
	}
}

void Service::stopService(void)
{
	active = false;
	terminate();
	if(so != INVALID_SOCKET)
	{
		::close(so);
		so = INVALID_SOCKET;
	}
	if(rpcpath[0])
		remove(rpcpath);
}

void Service::detachSession(BayonneSession *session)
{
	const char *manager = session->getSymbol("session.exit_manager");
	const char *reason = session->getSymbol("session.exit_reason");
	exit_t *ex;

	if(!reason)
	{
		reason = session->getSymbol("script.error");
		if(!reason || !stricmp(reason, "none"))
			reason = NULL;
	}
	
	if(!exit_count || !manager || !reason || !*manager || !*reason)
		return;

	if(stricmp(manager, "xmlrpc"))
		return;

	exit_lock.enter();
	ex = &exit_list[exit_mark++];
	exit_mark = exit_mark % exit_count;
	exit_lock.leave();
	setString(ex->session, 16, session->getSessionId());
	setString(ex->reason, 64, reason);
}

#ifdef	WIN32
void Service::run(void)
{
	unsigned long timer = getValue("timer");
	for(;;)
	{
		if(xmlrpc_update)
		{
			xmlrpc_update = false;
			reload();
		}
		Thread::sleep(timer * 1000);
	}
}
#else

#include <sys/un.h>

void Service::run(void)
{
	static char *request, *reply;
	static struct sockaddr_un addr;
	static socklen_t len;
	time_t now, last;
	unsigned slen;
	int rtn;
#ifdef	HAVE_POLL
	struct pollfd pfd[1];
#else
	fd_set sfd;
	struct timeval tv;
#endif
	unsigned long reply_size = getValue("send") * 1000;
	unsigned long request_size = getValue("recv") * 1000;
	unsigned long timer = getValue("timer");

	time(&last);
	if(getBoolean("local"))
	{
		request = new char[request_size];
		reply = new char[reply_size];
		so = socket(AF_UNIX, SOCK_DGRAM, 0);
	}

	if(so > -1)
	{
		snprintf(rpcpath, sizeof(rpcpath), "%s/bayonne.rpc", server->getLast("runfiles"));
		slen = strlen(rpcpath);
    	if(slen > sizeof(addr.sun_path))
        	slen = sizeof(addr.sun_path);
    	memset(&addr, 0, sizeof(addr));
    	addr.sun_family = AF_UNIX;
    	memcpy( addr.sun_path, rpcpath, slen );
	
#ifdef  __SUN_LEN
    	len = sizeof(addr.sun_len) + strlen(addr.sun_path) +sizeof(addr.sun_family) + 1;
		addr.sun_len = len;
#else
    	len = strlen(addr.sun_path) + sizeof(addr.sun_family) + 1;
#endif
	}
	remove(rpcpath);
	
	if(so > -1)
		if(bind(so, (struct sockaddr *)&addr, len))
		{
			::close(so);
			so = -1;
		}

	if(!getBoolean("local"))
		slog.debug("xmlrpc service started, remote only");
	else if(so > -1)
		slog.debug("xmlrpc service started");
	else
	{
		so = INVALID_SOCKET;
		slog.error("xmlrpc failed; socket=%s", rpcpath);
	}

	for(;;)
	{
		time(&now);
		if(now >= (time_t)(last + timer) && xmlrpc_update)
		{
			last = now;
			xmlrpc_update = false;
			reload();
		}
			
		if(so < 0)
		{			
			Thread::sleep(timer * 1000);
			continue;
		}

#ifdef	HAVE_POLL
		pfd[0].fd = so;
#ifdef	POLLRDNORM
		pfd[0].events = POLLIN | POLLRDNORM;
#else
		pfd[0].events = POLLIN;
#endif
		pfd[0].revents = 0;
		rtn = poll(pfd, 1, timer * 1000);
#else
		FD_ZERO(&sfd);
		FD_SET(so, &sfd);
		tv.tv_sec = timer;
		tv.tv_usec = 0;
		rtn = select(so + 1, &sfd, NULL, NULL, &tv) > 0);
#endif
		if(rtn < 1)
			continue;

		len = sizeof(addr);
		rtn = recvfrom(so, request, request_size - 1,  0, (struct sockaddr *)&addr, &len);
		if(rtn < 1)
			continue;
		request[rtn] = 0;					
        transport.authorized = true;
        transport.driver = NULL;
        transport.userid = NULL;
        transport.protocol = "unix";
        transport.agent_id = "bayonne.rpc";
        transport.buffer = reply;
        transport.bufsize = reply_size;
        transport.bufused = 0;
        transport.buffer[0] = 0;
        result.code = 200;
        result.string = "OK";

		if(parseCall(request))
		{
			if(!invokeXMLRPC())
				sendFault(1, "Unknown Method");
			
			if(result.code && result.code != 200)
				sendFault(result.code, result.string);
		}
		else 
			sendFault(400, "Malformed Request");

		sendto(so, reply, strlen(reply), 0, (struct sockaddr *)&addr, len);
	}
	
	Thread::sync();
}
#endif

} // namespace 

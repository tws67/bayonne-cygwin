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
#include <cc++/process.h>

namespace moduleWebservice {
using namespace ost;
using namespace std;

Service Service::webservice;
bool Service::active = false;
DynamicKeydata Service::admin("/bayonne/web/admin");
DynamicKeydata Service::rpc("/bayonne/web/rpc");

static Keydata::Define defkeys[] = {
    {"interface", "127.0.0.1"},
    {"port", "8055"},
    {"backlog", "5"},
    {"output", "2048"},
    {"input", "1024"},
    {"segment", "536"},
    {"refresh", "10"},
    {"realm", "bayonne"},
    {"datafiles", WEBSERVER_FILES},
    {"logfiles", LOG_FILES},
    {"zeroconf", "yes"},
	{"authenticate", "yes"},
    {NULL, NULL}};

Service::Service() :
BayonneService(0, 0), 
BayonneZeroconf("_http._tcp"),
StaticKeydata("/bayonne/web/service", defkeys)
{
	if(getBoolean("zeroconf"))
		zeroconf_port = getValue("port");
}

void Service::stopService(void)
{
	active = false;
	terminate();
}

#include <cerrno>

void Service::attachSession(BayonneSession *s)
{
	char buf[320];
	char *bp = buf;
	size_t max = sizeof(buf);
	const char *pid = s->getSessionParent();
	const char *cp;
	time_t now;

	if(!EventStream::first)
		return;

	time(&now);

	if(pid && *pid && stricmp(pid, "none"))
	{
		xmlwrite(&bp, &max, 
			"<session type=\"ring\" id=%q>\r\n"
			" <connection>%s</connection>\r\n", 
				s->getSessionId(), pid);
	}
	else
		xmlwrite(&bp, &max, "<session type=\"call\" id=%q>\r\n",
			s->getSessionId());

	xmlwrite(&bp, &max, " <start>%t</start>\n", now);

	cp = s->getSymbol("session.caller");
	if(cp && *cp)
		xmlwrite(&bp, &max, " <caller>%s</caller>\r\n", cp);

    cp = s->getSymbol("session.display");
    if(cp && *cp)
        xmlwrite(&bp, &max, " <display>%s</display>\r\n", cp);

    cp = s->getSymbol("session.dialed");
    if(cp && *cp)
        xmlwrite(&bp, &max, " <dialed>%s</dialed>\r\n", cp);

	xmlwrite(&bp, &max, "</session>\r\n");
	postEvent(buf, s);
}

void Service::postEvent(const char *buf, BayonneSession *s)
{
	EventStream *event, *next;
	const char *uuid = s->getSymbol("session.uuid");
	const char *aid = getRegistryId(s->getSymbol("session.authorized"));
	const char *uid = getRegistryId(s->getSymbol("session.identity"));

    EventStream::lock.enter();
    event = EventStream::first;
	while(event)
	{
		next = event->next;
		if(!event->isConnected())
			goto disconnect;
		if(event->authorized[0])
		{
			if(uuid && !stricmp(uuid, event->authorized))
				goto use;
			uuid = NULL;
			if(uid && !stricmp(uid, event->authorized))
				goto use;
			if(aid && !stricmp(aid, event->authorized))
				goto use;
			goto skip;
		}

use:
		if(!event->sendEvent(buf))
		{
disconnect:
			SLOG_DEBUG("webservice/%d: disconnected", event->so);
			event->release();
		}

skip:
		event = next;
	}
	EventStream::lock.leave();
}

void Service::exitingCall(BayonneSession *s)
{
    BayonneSession *joined = NULL;
    const char *jid = s->getSessionJoined();

    if(jid && *jid)
        joined = Bayonne::getSid(jid);

    if(joined)
        breakSession(joined);

    breakSession(s);
}

void Service::enteringCall(BayonneSession *s)
{
	BayonneSession *joined = NULL;
	const char *jid = s->getSessionJoined();

	if(jid && *jid)
		joined = Bayonne::getSid(jid);

	if(joined)
		joinedSession(joined);

	joinedSession(s);
}

void Service::breakSession(BayonneSession *s)
{			
    char buf[256];
	const char *a = s->getSessionJoined();
	char *bp = buf;
	size_t max = sizeof(buf);

    if(!EventStream::first)
        return;

	if(a && !strcmp(a, "none"))
		a = NULL;

	xmlwrite(&bp, &max, 
		"<session type=\"break\" id=%q>\r\n"
		" <position>%s</position>\r\n",
		" <reason>%s</reason>\r\n",
		s->getSessionId(), s->getSymbol("session.duration"),
		s->getSymbol("script.error"));

	if(a)
		xmlwrite(&bp, &max, " <connection>%s</connection>\r\n", a);

	xmlwrite(&bp, &max, "</session>\r\n");

	postEvent(buf, s);
}

void Service::joinedSession(BayonneSession *s)
{
    char buf[256];
	char *bp = buf;
	size_t max = sizeof(buf);

    if(!EventStream::first)
        return;

	xmlwrite(&bp, &max, 
		"<session type=\"join\" id=%q>\r\n"
        " <position>%s</position>\r\n"
		" <connection>%s</connection>\r\n"
		"</session>\r\n",
			s->getSessionId(),
			s->getExternal("session.position"),
			s->getSymbol("session.joined"));

	postEvent(buf, s);
}
	
void Service::detachSession(BayonneSession *s)
{
	static char reply[1024];
	char buf[256];
	char *bp = buf;
	size_t max = sizeof(buf);
	const char *reason = s->getSymbol("session.exit_reason");
	const char *manager = s->getSymbol("session.exit_manager");
	EventStream *event, *next;

	if(!EventStream::first)
		return;

	if(!reason || !*reason)
		reason = s->getSymbol("script.error");

	xmlwrite(&bp, &max,
		"<session type=\"exit\" id=%q>\r\n"
		" <position>%s</position>\r\n"
		" <reason>%s</reason>\r\n"
		"</session>\r\n",
				s->getSessionId(),
				s->getSymbol("session.duration"),
				reason);

	postEvent(buf, s);

	if(!manager || stricmp(manager, "webservice"))
		return;

	EventStream::lock.enter();
	event = EventStream::first;
	while(event)
	{
		next = event->next;
		if(!stricmp(event->authorized, s->getSessionId()))
		{
			bp = reply;
			max = sizeof(reply);
			xmlwrite(&bp, &max, 		
				"<?xml version=\"1.0\"?>\r\n"
				"<methodResponse><params><param><value><struct>\r\n"
				" <member><name>session</name>\r\n"
				"  <value><string>%s</string></value></member>\r\n"
				" <member><name>reason</name>\r\n"
				"  <value><string>%s</string></value></member>\r\n"
				" <member><name>complete</name>\r\n"
				"  <value><boolean>1</boolean></value></member>\r\n"
				"</struct></value></param></params></methodResponse>\r\n",
				s->getSessionId(), reason);
			event->sendReply(reply);
			break;
		}
		event = next;
	}
	EventStream::lock.leave();
}

void Service::run(void)
{
	Session *s;
	SOCKET so;
	TCP *tcp;
	const char *cp;
	char *tok;
	char *p = (char *)getString("interface");
	unsigned count = 0, ipv6_count = 0;

	slog.info("webservice started");
	
	cp = strtok_r(p, " ,;\t", &tok);
	while(cp)
	{
#ifdef	CCXX_IPV6
		++count;
		if(strchr(cp, ':'))
		{
			++ipv6_count;
			new TCPV6(cp);
		}
		else
#endif
		new TCPV4(cp);
		cp = strtok_r(NULL, " ,;\t", &tok);
	}

	if(count && ipv6_count == count)
		zeroconf_family = ZEROCONF_IPV6;
	else if(count && !ipv6_count)
		zeroconf_family = ZEROCONF_IPV4;
	
	for(;;)
	{
		tcp = TCP::getSelect();
		if(!tcp)
			break;
		so = tcp->getAccept();
		if(so == INVALID_SOCKET)
			continue;
		s = new Session(so);
		s->detach();
	}
}

} // namespace 

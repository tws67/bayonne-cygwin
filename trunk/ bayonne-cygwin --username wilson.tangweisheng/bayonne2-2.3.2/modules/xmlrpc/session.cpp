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
#include <cc++/slog.h>
#include <cc++/socket.h>

namespace moduleXMLRPC {
using namespace ost;
using namespace std;

typedef	Bayonne::rpcint_t rpcint_t;
typedef	rpcint_t rpcbool_t;
typedef	Bayonne::regauth_t registry_t;
typedef	Bayonne::timeslot_t timeslot_t;
typedef	Bayonne::Event Event;
typedef Bayonne::event_t event_t;

extern const char *getExit(const char *sid);

static void session_version(BayonneRPC *rpc)
{
    if(rpc->getCount() != 1)
    {
        rpc->sendFault(3, "Invalid Parameters");
        return;
    }
	rpc->buildResponse("(ii)", 
		"current", (rpcint_t)3,
		"prior", (rpcint_t)3);
}

static void session_connect(BayonneRPC *rpc)
{
	const char *dialed, *caller, *display, *authorized;
	const char *script = "connect::connect";
	BayonneSession *session;

	if(rpc->getCount() != 1)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	dialed = rpc->getNamed(1, "source");
	caller = rpc->getNamed(1, "target");
	display = rpc->getNamed(1, "display");

	if(rpc->transport.authorized)
		authorized = "xmlrpc";
	else
		authorized = rpc->transport.userid;

	if(!dialed || !*dialed)
	{
		rpc->sendFault(4, "Invalid origin");
		return;
	}

	if(caller && !*caller)
	{
		rpc->sendFault(4, "Invalid destination");
		return;
	}

	if(display && !*display)
		display = NULL;

	session = Bayonne::startDialing(dialed, script, caller, display, NULL, authorized);
	if(!session)
	{
		rpc->sendFault(5, "Destination not dialable");
		return;
	}
	session->setConst("session.exit_manager", "xmlrpc");
	if(!rpc->transport.authorized)
		session->setConst("session.authorized", authorized); 
	rpc->buildResponse("s", session->getSessionId());
	session->leave();
}

static void session_create(BayonneRPC *rpc)
{
	const char *dialed, *script, *caller, *display, *authorized, *uuid;
	BayonneSession *session;

	if(rpc->getCount() != 1)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	dialed = rpc->getNamed(1, "dialed");
	script = rpc->getNamed(1, "script");
	caller = rpc->getNamed(1, "caller");
	display = rpc->getNamed(1, "display");
	uuid = rpc->getNamed(1, "uuid");

	if(uuid && strrchr(uuid, ':'))
		uuid = strrchr(uuid, ':') + 1;

	if(rpc->transport.authorized)
		authorized = "xmlrpc";
	else
	{
		rpc->transportFault(401, "Not Authorized");
		return;
	}

	if(!dialed || !*dialed)
	{
		rpc->sendFault(4, "Invalid destination");
		return;
	}

	if(!script || !*script)
	{
		rpc->sendFault(4, "Invalid script");
		return;
	}

	if(caller && !*caller)
		caller = NULL;

	if(display && !*display)
		display = NULL;

	session = Bayonne::startDialing(dialed, script, caller, display, NULL, authorized);
	if(!session)
	{
		rpc->sendFault(5, "Destination not dialable");
		return;
	}
	if(uuid && *uuid)
		session->setConst("session.uuid", uuid);
	session->setConst("session.exit_manager", "xmlrpc");
	if(!rpc->transport.authorized)
		session->setConst("session.authorized", authorized); 
	rpc->buildResponse("s", session->getSessionId());
	session->leave();
}

static void session_start(BayonneRPC *rpc)
{
	const char *dialed, *script, *caller, *display, *authorized, *uuid;
	const char *service;
	BayonneSession *session;
	char buf[80];
	const char *value;
	unsigned member = 1;
	Script::Line *line = NULL;
	ScriptImage *img = NULL;
	unsigned idx = 0;

	if(rpc->getCount() <1 || rpc->getCount() > 2)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	dialed = rpc->getNamed(1, "dialed");
	service = rpc->getNamed(1, "service");
	caller = rpc->getNamed(1, "caller");
	display = rpc->getNamed(1, "display");
	uuid = rpc->getNamed(1, "uuid");

	if(uuid && strrchr(uuid, ':'))
		uuid = strrchr(uuid, ':') + 1;

	if(rpc->transport.authorized)
		authorized = "xmlrpc";
	else
		authorized = rpc->transport.userid;

	if(!dialed || !*dialed)
	{
		rpc->sendFault(4, "Invalid destination");
		return;
	}

	if(!service || !*service)
	{
		rpc->sendFault(4, "Invalid Service");
		return;
	}

	snprintf(buf, sizeof(buf), "service.%s", service);
	img = Bayonne::useImage();
	if(img)
		line = (Script::Line *)img->getPointer(buf);

	if(!line)
	{
        rpc->sendFault(5, "Unknown Service");
		Bayonne::endImage(img);
        return;
	}

	script = line->cmd;

	if(!script || !*script)
	{
		rpc->sendFault(4, "Invalid script");
		Bayonne::endImage(img);
		return;
	}

	if(caller && !*caller)
		caller = NULL;

	if(display && !*display)
		display = NULL;

	session = Bayonne::startDialing(dialed, script, caller, display, NULL, authorized);
	if(!session)
	{
		rpc->sendFault(5, "Destination not dialable");
		Bayonne::endImage(img);
		return;
	}
	if(uuid && *uuid)
		session->setConst("session.uuid", uuid);
	session->setConst("session.exit_manager", "xmlrpc");
	if(!rpc->transport.authorized)
		session->setConst("session.authorized", authorized); 
	rpc->buildResponse("(sb)", 
		"session", session->getSessionId(),
		"complete", (rpcbool_t)0);

	while(NULL != (value = rpc->getIndexed(2, member)))
	{
		snprintf(buf, sizeof(buf), "param.%s", rpc->getParamId(2, member));
		session->setConst(buf, value);
		++member;
	}
	while(idx < line->argc)
	{
		value = line->args[idx++];
		if(!value)
			break;

		if(*value != '=')
			continue;
		snprintf(buf, sizeof(buf), "param.%s", ++value);
		value = session->getContent(line->args[idx++]);
		if(value)
			session->setConst(buf, value);	
	}

	session->leave();
	Bayonne::endImage(img);
}

static void session_service(BayonneRPC *rpc)
{
	const char *dialed, *script, *caller, *display, *authorized, *uuid;
	const char *service;
	BayonneSession *session;
	char buf[80];
	const char *value;
	unsigned member = 1;
	Script::Line *line = NULL;
	ScriptImage *img = NULL;
	unsigned idx = 0;

	if(rpc->getCount() < 1 || rpc->getCount() > 2)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	dialed = rpc->getNamed(1, "dialed");
	service = rpc->getNamed(1, "service");
	caller = rpc->getNamed(1, "caller");
	display = rpc->getNamed(1, "display");
	uuid = rpc->getNamed(1, "uuid");

	if(uuid && strrchr(uuid, ':'))
		uuid = strrchr(uuid, ':') + 1;

	if(rpc->transport.authorized)
		authorized = "xmlrpc";
	else
		authorized = rpc->transport.userid;

	if(!dialed || !*dialed)
	{
		rpc->sendFault(4, "Invalid destination");
		return;
	}

	if(!service || !*service)
	{
		rpc->sendFault(4, "Invalid Service");
		return;
	}

	snprintf(buf, sizeof(buf), "service.%s", service);
	img = Bayonne::useImage();
	if(img)
		line = (Script::Line *)img->getPointer(buf);

	if(!line)
	{
        rpc->sendFault(5, "Unknown Service");
		Bayonne::endImage(img);
        return;
	}

	script = line->cmd;

	if(!script || !*script)
	{
		rpc->sendFault(4, "Invalid script");
		Bayonne::endImage(img);
		return;
	}

	if(caller && !*caller)
		caller = NULL;

	if(display && !*display)
		display = NULL;

	session = Bayonne::startDialing(dialed, script, caller, display, NULL, authorized);
	if(!session)
	{
		rpc->sendFault(5, "Destination not dialable");
		Bayonne::endImage(img);
		return;
	}
	if(uuid && *uuid)
		session->setConst("session.uuid", uuid);
	if(!rpc->transport.authorized)
		session->setConst("session.authorized", authorized); 

	while(NULL != (value = rpc->getIndexed(2, member)))
	{
		snprintf(buf, sizeof(buf), "param.%s", rpc->getParamId(2, member));
		session->setConst(buf, value);
		++member;
	}
	while(idx < line->argc)
	{
		value = line->args[idx++];
		if(!value)
			break;

		if(*value != '=')
			continue;
		snprintf(buf, sizeof(buf), "param.%s", ++value);
		value = session->getContent(line->args[idx++]);
		if(value)
			session->setConst(buf, value);	
	}
	Bayonne::endImage(img);
	rpc->setComplete(session);
	session->setConst("session.exit_manager", "xmlrpc");
	session->leave();
}

static void session_transfer(BayonneRPC *rpc)
{
	BayonneSession *session = NULL;
	const char *destination, *id;
	bool result;
	Event event;

	if(rpc->getCount() != 2)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	id = rpc->getIndexed(1);
	destination = rpc->getIndexed(2);
	if(id && strchr(id, '-'))
		session = Bayonne::getSid(id); 
	if(!id || !session)
	{
		rpc->sendFault(4, "Session Id Invalid");
		return;
	}

    if(!destination || !*destination)
    {
        rpc->sendFault(4, "Destination Invalid");
        return;
    }

	session->enter();

	if(!session->isHolding())
	{
		session->leave();
		rpc->sendFault(6, "Session not on hold");
		return;
	}

	if(!session->isJoined())
	{
		session->leave();
		rpc->sendFault(6, "Session not joined");
		return;
	}

	memset(&event, 0, sizeof(event));
	event.id = Bayonne::START_REFER;
	event.dialing = destination;
	result = session->postEvent(&event);
	session->leave();
	rpc->buildResponse("b", (rpcbool_t)result);
}

static void session_getExit(BayonneRPC *rpc)
{
	BayonneSession *session = NULL;
	const char *id;
	bool active = false;
	const char *reason = "none";

	if(rpc->getCount() != 1)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	id = rpc->getIndexed(1);
	if(id && strchr(id, '-'))
		session = Bayonne::getSid(id); 
	if(!id || !*id)
	{
		rpc->sendFault(4, "Session Id Invalid");
		return;
	}

	if(session)
	{
		active = true;
		goto finish;
	}

	reason = getExit(id);

finish:
	rpc->buildResponse("(bs)",
		"active", (rpcbool_t)active,
		"reason", reason);
}

static void session_getSymbol(BayonneRPC *rpc)
{
	BayonneSession *session = NULL;
	const char *id, *var, *value = NULL;

	if(rpc->getCount() != 2)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	id = rpc->getIndexed(1);
	var = rpc->getIndexed(2);
	if(id && strchr(id, '-'))
		session = Bayonne::getSid(id); 
	if(!id || !session)
	{
		rpc->sendFault(4, "Session Id Invalid");
		return;
	}

    if(!var || !*var || !strchr(var, '.'))
    {
        rpc->sendFault(4, "Symbol Id Invalid");
        return;
    }

	session->enter();
	value = session->getSymbol(var);
	rpc->buildResponse("s", value);
	session->leave();
}

static void session_setSymbol(BayonneRPC *rpc)
{
	BayonneSession *session = NULL;
	const char *id, *var, *value;
	bool result;

	if(rpc->getCount() != 3)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	id = rpc->getIndexed(1);
	var = rpc->getIndexed(2);
	value = rpc->getIndexed(3);
	if(id && strchr(id, '-'))
		session = Bayonne::getSid(id); 
	if(!id || !session)
	{
		rpc->sendFault(4, "Session Id Invalid");
		return;
	}
	
	if(!var || !*var || !strchr(var, '.'))
	{
		rpc->sendFault(4, "Symbol Id Invalid");
		return;
	}

	if(!value)
	{
        rpc->sendFault(4, "Symbol Value Missing");
        return;
	}

	session->enter();
	result = session->setSymbol(var, value);
	rpc->buildResponse("s", (rpcbool_t)result);
	session->leave();
}

static void session_addSymbol(BayonneRPC *rpc)
{
	BayonneSession *session = NULL;
	const char *id, *var, *value;
	bool result;

	if(rpc->getCount() != 3)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	id = rpc->getIndexed(1);
	var = rpc->getIndexed(2);
	value = rpc->getIndexed(3);
	if(id && strchr(id, '-'))
		session = Bayonne::getSid(id); 
	if(!id || !session)
	{
		rpc->sendFault(4, "Session Id Invalid");
		return;
	}
	
	if(!var || !*var || !strchr(var, '.'))
	{
		rpc->sendFault(4, "Symbol Id Invalid");
		return;
	}

	if(!value)
	{
        rpc->sendFault(4, "Symbol Value Missing");
        return;
	}

	session->enter();
	result = session->addSymbol(var, value);
	rpc->buildResponse("s", (rpcbool_t)result);
	session->leave();
}

static void session_clearSymbol(BayonneRPC *rpc)
{
	BayonneSession *session = NULL;
	const char *id, *var;
	bool result;

	if(rpc->getCount() != 2)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	id = rpc->getIndexed(1);
	var = rpc->getIndexed(2);
	if(id && strchr(id, '-'))
		session = Bayonne::getSid(id); 
	if(!id || !session)
	{
		rpc->sendFault(4, "Session Id Invalid");
		return;
	}
	
	if(!var || !*var || !strchr(var, '.'))
	{
		rpc->sendFault(4, "Symbol Id Invalid");
		return;
	}

	session->enter();
	result = session->clearSymbol(var);
	rpc->buildResponse("s", (rpcbool_t)result);
	session->leave();
}

static void session_stop(BayonneRPC *rpc)
{
	BayonneSession *session = NULL;
	const char *id;
	bool result = false;
	Event event;

	if(rpc->getCount() != 1)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	id = rpc->getIndexed(1);
	if(id && strchr(id, '-'))
		session = Bayonne::getSid(id); 
	if(!id || !session)
	{
		rpc->sendFault(4, "Session Id Invalid");
		return;
	}

	memset(&event, 0, sizeof(event));
	event.id = Bayonne::STOP_SCRIPT;
	result = session->postEvent(&event);
	rpc->buildResponse("b", (rpcbool_t)result);
}

static void session_hold(BayonneRPC *rpc)
{
	BayonneSession *session = NULL;
	const char *id;
	bool result = false;
	Event event;

	if(rpc->getCount() != 1)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	id = rpc->getIndexed(1);
	if(id && strchr(id, '-'))
		session = Bayonne::getSid(id); 
	if(!id || !session)
	{
		rpc->sendFault(4, "Session Id Invalid");
		return;
	}

	memset(&event, 0, sizeof(event));
	event.id = Bayonne::CALL_HOLD;
	result = session->postEvent(&event);
	rpc->buildResponse("b", (rpcbool_t)result);
}

static void session_release(BayonneRPC *rpc)
{
	BayonneSession *session = NULL;
	const char *id;
	bool result = false;
	Event event;

	if(rpc->getCount() != 1)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	id = rpc->getIndexed(1);
	if(id && strchr(id, '-'))
		session = Bayonne::getSid(id); 
	if(!id || !session)
	{
		rpc->sendFault(4, "Session Id Invalid");
		return;
	}

	memset(&event, 0, sizeof(event));
	event.id = Bayonne::DROP_RECALL;
	result = session->postEvent(&event);
	rpc->buildResponse("b", (rpcbool_t)result);
}


static void session_holdJoined(BayonneRPC *rpc)
{
	BayonneSession *session = NULL;
	const char *id;
	bool result = false;
	Event event;

	if(rpc->getCount() != 1)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	id = rpc->getIndexed(1);
	if(id && strchr(id, '-'))
		session = Bayonne::getSid(id); 
	if(!id || !session)
	{
		rpc->sendFault(4, "Session Id Invalid");
		return;
	}

	memset(&event, 0, sizeof(event));
	
	session->enter();
	event.id = Bayonne::CALL_HOLD;
	if(session->isJoined())
		result = session->postEvent(&event);
	session->leave();
	rpc->buildResponse("b", (rpcbool_t)result);
}


static void session_resume(BayonneRPC *rpc)
{
	BayonneSession *session = NULL;
	const char *id;
	bool result = false;
	Event event;

	if(rpc->getCount() != 1)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	id = rpc->getIndexed(1);
	if(id && strchr(id, '-'))
		session = Bayonne::getSid(id); 
	if(!id || !session)
	{
		rpc->sendFault(4, "Session Id Invalid");
		return;
	}

	memset(&event, 0, sizeof(event));
	event.id = Bayonne::CALL_NOHOLD;
	result = session->postEvent(&event);
	rpc->buildResponse("b", (rpcbool_t)result);
}

static void session_getInfo(BayonneRPC *rpc)
{
	BayonneSession *session = NULL;
	BayonneSpan *span;
	BayonneDriver *driver;
	const char *id, *pid, *jid;
	rpcint_t spanid = 0;

	if(rpc->getCount() != 1)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	id = rpc->getIndexed(1);
	if(id && strchr(id, '-'))
		session = Bayonne::getSid(id); 
	if(!id || !session)
	{
		rpc->sendFault(4, "Session Id Invalid");
		return;
	}

	driver = session->getDriver();
	span = session->getSpan();
	if(span)
		spanid = span->getId();

	if(session->isAssociated())
		pid = session->getSessionParent();
	else
		pid = "none";

	jid = session->getSessionJoined();
	if(!jid || !*jid)
		jid = "none";

	if(!session->isJoined())
		jid = "none";

	rpc->buildResponse("(ssiissti)",
		"parent", pid,
		"joined", jid,
		"timeslot", (rpcint_t)session->getTimeslot(),
		"span", spanid,
		"driver", driver->getName(),
		"encoding", session->audioEncoding(),
		"started", session->getSessionStart(),
		"started_int", (rpcint_t)session->getSessionStart()
	);
}

static void session_callstats(BayonneRPC *rpc)
{
	BayonneSession *session = NULL;
	const char *id;

	if(rpc->getCount() != 1)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	id = rpc->getIndexed(1);
	if(id && strchr(id, '-'))
		session = Bayonne::getSid(id); 
	if(!id || !session)
	{
		rpc->sendFault(4, "Session Id Invalid");
		return;
	}

	rpc->buildResponse("(iiiiii)",
		"iattempts", 
			(rpcint_t)session->call_attempts.iCount,
		"oattempts", 
			(rpcint_t)session->call_attempts.oCount,
		"sattempts",
			(rpcint_t)session->call_attempts.getStamp(),
		"icomplete",
			(rpcint_t)session->call_complete.iCount,
		"ocomplete",
			(rpcint_t)session->call_complete.oCount,
		"scomplete",
			(rpcint_t)session->call_complete.getStamp()
	);
}

static void session_list(BayonneRPC *rpc)
{
	BayonneSession *session;
	timeslot_t timeslot = 0;
	const char *sid, *uid, *aid;
	
	if(rpc->getCount())
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	rpc->buildResponse("[");
	while(timeslot < Bayonne::getTimeslotsUsed())
	{
		session = Bayonne::getSession(timeslot++);
		if(!session)
			continue;
		session->enter();
		if(rpc->transport.authorized)
			goto authorized;

		sid = Bayonne::getRegistryId(rpc->transport.userid);
		if(!sid || !*sid)
			goto noauth;
		uid = Bayonne::getRegistryId(session->getSymbol("session.identity"));
		aid = Bayonne::getRegistryId(session->getSymbol("session.authorized"));
		if(uid && !stricmp(uid, sid))
			goto authorized;
		if(aid && !stricmp(aid, sid))
			goto authorized;
		goto noauth;

authorized:
		sid = session->getSessionId();
		if(sid && *sid && stricmp(sid, "none"))			
			rpc->buildResponse("!s", sid);
noauth:
		session->leave();
	}
	rpc->buildResponse("]");
}

static Bayonne::RPCDefine dispatch[] = {
	{"version", session_version,
		"API version number of session rpc", "struct"},
	{"getInfo", session_getInfo,
		"List session information", "struct, string"},
	{"getExit", session_getExit,
		"Get session exit code", "struct, string"},
	{"list", session_list,
		"List sessions active in server", "array"},
	{"stop", session_stop,
		"Stop active call session", "boolean, string"},
    {"hold", session_hold,
        "Put active call session on hold", "boolean, string"},
    {"holdJoined", session_holdJoined,
        "Put joined call session on hold for transfer", "boolean, string"},
    {"resume", session_resume,
        "Resume call session from hold", "boolean, string"},
	{"release", session_release,
		"Release held/transfer call", "boolean, string"},
    {"getSymbol", session_getSymbol,
        "Get value of symbol in session", "string, string, string"},
    {"setSymbol", session_setSymbol,
        "Set value of symbol in session", "boolean, string, string, string"},
    {"addSymbol", session_addSymbol,
        "Add to value of symbol in session", "boolean, string, string, string"},
    {"clearSymbol", session_clearSymbol,
        "Clear value of symbol in session", "boolean, string, string"},
	{"callstats", session_callstats,
		"List session specific call statistics", "struct, string"},
	{"connect", session_connect,
		"Connect two endpoints", "string, struct"},
	{"create", session_create,
		"Create a new call session", "string, struct"},
	{"start", session_start,
		"Start a new service with params", "struct, struct, struct"},
	{"service", session_service,
		"Execute a service script with params", "struct, struct, struct"},
	{"transfer", session_transfer,
		"Transfer a call thru session", "boolean, string, string"},
	{NULL, NULL, NULL, NULL}};

static Bayonne::RPCNode xmlrpc_session("session", dispatch);

}; // namespace


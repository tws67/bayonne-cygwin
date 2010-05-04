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

static void timeslot_version(BayonneRPC *rpc)
{
    if(rpc->getCount() != 1)
    {
        rpc->sendFault(3, "Invalid Parameters");
        return;
    }
	rpc->buildResponse("(ii)", 
		"current", (rpcint_t)1,
		"prior", (rpcint_t)0);
}

static void timeslot_transfer(BayonneRPC *rpc)
{
	BayonneSession *session = NULL;
	const char *destination, *id;
	bool result;
	Event event;

	if(!rpc->transport.authorized)
	{
		rpc->transportFault(401, "Not Authorized");
		return;
	}

	if(rpc->getCount() != 2)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	id = rpc->getIndexed(1);
	destination = rpc->getIndexed(2);
	if(id)
		session = Bayonne::getSession((timeslot_t)atoi(id)); 
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


static void timeslot_getSymbol(BayonneRPC *rpc)
{
	BayonneSession *session = NULL;
	const char *id, *var, *value = NULL;

	if(!rpc->transport.authorized)
	{
		rpc->transportFault(401, "Not Authorized");
		return;
	}

	if(rpc->getCount() != 2)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	id = rpc->getIndexed(1);
	var = rpc->getIndexed(2);
	if(id)
		session = Bayonne::getSession((timeslot_t)atoi(id)); 
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

static void timeslot_setSymbol(BayonneRPC *rpc)
{
	BayonneSession *session = NULL;
	const char *id, *var, *value;
	bool result;

	if(!rpc->transport.authorized)
	{
		rpc->transportFault(401, "Not Authorized");
		return;
	}

	if(rpc->getCount() != 3)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	id = rpc->getIndexed(1);
	var = rpc->getIndexed(2);
	value = rpc->getIndexed(3);
	if(id)
		session = Bayonne::getSession((timeslot_t)atoi(id)); 
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

static void timeslot_addSymbol(BayonneRPC *rpc)
{
	BayonneSession *session = NULL;
	const char *id, *var, *value;
	bool result;

	if(!rpc->transport.authorized)
	{
		rpc->transportFault(401, "Not Authorized");
		return;
	}

	if(rpc->getCount() != 3)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	id = rpc->getIndexed(1);
	var = rpc->getIndexed(2);
	value = rpc->getIndexed(3);
	if(id)
		session = Bayonne::getSession((timeslot_t)atoi(id)); 
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

static void timeslot_clearSymbol(BayonneRPC *rpc)
{
	BayonneSession *session = NULL;
	const char *id, *var;
	bool result;

	if(!rpc->transport.authorized)
	{
		rpc->transportFault(401, "Not Authorized");
		return;
	}

	if(rpc->getCount() != 2)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	id = rpc->getIndexed(1);
	var = rpc->getIndexed(2);
	if(id)
		session = Bayonne::getSession((timeslot_t)atoi(id)); 
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

static void timeslot_drop(BayonneRPC *rpc)
{
	BayonneSession *session = NULL;
	const char *id;
	bool result = false;
	Event event;

	if(!rpc->transport.authorized)
	{
		rpc->transportFault(401, "Not Authorized");
		return;
	}

	if(rpc->getCount() != 1)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	id = rpc->getIndexed(1);
	if(id)
		session = Bayonne::getSession((timeslot_t)atoi(id)); 
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

static void timeslot_hold(BayonneRPC *rpc)
{
	BayonneSession *session = NULL;
	const char *id;
	bool result = false;
	Event event;

	if(!rpc->transport.authorized)
	{
		rpc->transportFault(401, "Not Authorized");
		return;
	}

	if(rpc->getCount() != 1)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	id = rpc->getIndexed(1);
	if(id)
		session = Bayonne::getSession((timeslot_t)atoi(id)); 
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

static void timeslot_release(BayonneRPC *rpc)
{
	BayonneSession *session = NULL;
	const char *id;
	bool result = false;
	Event event;

	if(!rpc->transport.authorized)
	{
		rpc->transportFault(401, "Not Authorized");
		return;
	}

	if(rpc->getCount() != 1)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	id = rpc->getIndexed(1);
	if(id)
		session = Bayonne::getSession((timeslot_t)atoi(id)); 
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


static void timeslot_holdJoined(BayonneRPC *rpc)
{
	BayonneSession *session = NULL;
	const char *id;
	bool result = false;
	Event event;

	if(!rpc->transport.authorized)
	{
		rpc->transportFault(401, "Not Authorized");
		return;
	}

	if(rpc->getCount() != 1)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	id = rpc->getIndexed(1);
	if(id)
		session = Bayonne::getSession((timeslot_t)atoi(id)); 
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


static void timeslot_resume(BayonneRPC *rpc)
{
	BayonneSession *session = NULL;
	const char *id;
	bool result = false;
	Event event;

	if(!rpc->transport.authorized)
	{
		rpc->transportFault(401, "Not Authorized");
		return;
	}

	if(rpc->getCount() != 1)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	id = rpc->getIndexed(1);
	if(id)
		session = Bayonne::getSession((timeslot_t)atoi(id)); 
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

static void timeslot_getInfo(BayonneRPC *rpc)
{
	BayonneSession *session = NULL, *parent = NULL;
	BayonneSpan *span;
	BayonneDriver *driver;
	const char *id;
	rpcint_t spanid = 0;
	timeslot_t pid = 0xffff;

	if(!rpc->transport.authorized)
	{
		rpc->transportFault(401, "Not Authorized");
		return;
	}

	if(rpc->getCount() != 1)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	id = rpc->getIndexed(1);
	if(id)
		session = Bayonne::getSession((timeslot_t)atoi(id)); 
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
		parent = Bayonne::getSid(session->getSessionParent());

	if(parent)
		pid = parent->getTimeslot();

	rpc->buildResponse("(siissti)",
		"session", session->getSessionId(),
		"parent", (rpcint_t)pid,
		"span", spanid,
		"driver", driver->getName(),
		"encoding", session->audioEncoding(),
		"started", session->getSessionStart(),
		"started_int", (rpcint_t)session->getSessionStart()
	);
}

static void timeslot_callstats(BayonneRPC *rpc)
{
	BayonneSession *session = NULL;
	const char *id;

	if(!rpc->transport.authorized)
	{
		rpc->transportFault(401, "Not Authorized");
		return;
	}

	if(rpc->getCount() != 1)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	id = rpc->getIndexed(1);
	if(id)
		session = Bayonne::getSession((timeslot_t)atoi(id)); 
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

static Bayonne::RPCDefine dispatch[] = {
	{"version", timeslot_version,
		"API version level of timeslot rpc", "struct"},
	{"getInfo", timeslot_getInfo,
		"List timeslot information", "struct, int"},
	{"drop", timeslot_drop,
		"Drop active call timeslot", "boolean, int"},
    {"hold", timeslot_hold,
        "Put active call timeslot on hold", "boolean, int"},
    {"holdJoined", timeslot_holdJoined,
        "Put joined call timeslot on hold for transfer", "boolean, int"},
    {"resume", timeslot_resume,
        "Resume call timeslot from hold", "boolean, int"},
	{"release", timeslot_release,
		"Release held/transfer call timeslot", "boolean, int"},
    {"getSymbol", timeslot_getSymbol,
        "Get value of symbol in timeslot", "string, int, string"},
    {"setSymbol", timeslot_setSymbol,
        "Set value of symbol in timeslot", "boolean, int, string, string"},
    {"addSymbol", timeslot_addSymbol,
        "Add to value of symbol in timeslot", "boolean, int, string, string"},
    {"clearSymbol", timeslot_clearSymbol,
        "Clear value of symbol in timeslot", "boolean, int, string"},
	{"callstats", timeslot_callstats,
		"List timeslot specific call statistics", "struct, int"},
	{"transfer", timeslot_transfer,
		"Transfer a call thru timeslot", "boolean, int, string"},
	{NULL, NULL, NULL, NULL}};

static Bayonne::RPCNode xmlrpc_timeslot("timeslot", dispatch);

}; // namespace


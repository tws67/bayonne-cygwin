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

extern bool xmlrpc_update;

class Server : public Bayonne
{
public:
	static inline char *getLevel(void)
		{return sla;};	

	static inline char *getStatus(void)
		{return status;};

	static inline const char *getLast(const char *key)
		{return server->getLast(key);};

	static inline time_t getStarted(void)
		{return start_time;};

	static inline time_t getReloaded(void)
		{return reload_time;};

	static inline void setService(const char *str)
		{service(str);};

	static inline void setRunning(void)
		{sla[0] = 0;};
};	

static void server_version(BayonneRPC *rpc)
{
    if(rpc->getCount())
    {
        rpc->sendFault(3, "Invalid Parameters");
        return;
    }
	rpc->buildResponse("(ii)", 
		"current", (rpcint_t)4,
		"prior", (rpcint_t)0);
}

static void server_keymap(BayonneRPC *rpc)
{
	char keymap[257];
	unsigned pos = 0;

    if(rpc->getCount())
    {
        rpc->sendFault(3, "Invalid Parameters");
        return;
    }

	memcpy(keymap, Bayonne::dtmf_keymap, 256);
	while(pos < 256)
	{
		if(!keymap[pos])
			keymap[pos] = '-';

		++pos;
	}
	keymap[pos] = 0;
	rpc->buildResponse("s", keymap);
}

static void server_update(BayonneRPC *rpc)
{
    if(rpc->getCount())
    {
        rpc->sendFault(3, "Invalid Parameters");
        return;
    }

    if(!rpc->transport.authorized)
    {
        rpc->transportFault(401, "Not Authorized");
        return;
	}

	xmlrpc_update = true;
	rpc->sendSuccess();
}
	
static void server_reload(BayonneRPC *rpc)
{
	if(rpc->getCount())
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	if(!rpc->transport.authorized)
	{
		rpc->transportFault(401, "Not Authorized");
		return;
	}

	Server::reload();
	rpc->sendSuccess();
}

static void server_errlog(BayonneRPC *rpc)
{
	if(rpc->getCount() != 2)
		rpc->sendFault(3, "Invalid Parameters");

	Bayonne::errlog(rpc->getIndexed(1), "rpc: %s", rpc->getIndexed(2));
	rpc->sendSuccess();
}

static void server_callstats(BayonneRPC *rpc)
{
	if(rpc->getCount())
		rpc->sendFault(3, "Invalid Parameters");
	else
		rpc->buildResponse("(iiiiiii)",
			"iattempts", 
				(rpcint_t)Server::total_call_attempts.iCount,
			"oattempts", 
				(rpcint_t)Server::total_call_attempts.oCount,
			"sattempts",
				(rpcint_t)Server::total_call_attempts.getStamp(),
			"icomplete",
				(rpcint_t)Server::total_call_complete.iCount,
			"ocomplete",
				(rpcint_t)Server::total_call_complete.oCount,
			"scomplete",
				(rpcint_t)Server::total_call_complete.getStamp(),
			"active",
				(rpcint_t)Server::total_active_calls
		);
}

static void server_isScript(BayonneRPC *rpc)
{
	ScriptImage *img = Bayonne::useImage();
	Script::Name *scr;
	const char *id;
	bool rtn = false;

	if(rpc->getCount() != 1)
	{
		rpc->sendFault(3, "Invalid Parameters");
		goto done;
	}

	id = rpc->getIndexed(1);
	if(!id || !*id)
	{
		rpc->sendFault(4, "Script Missing");
		goto done;
	}

	scr = img->getScript(id);
	if(scr && scr->access == Script::scrPUBLIC)
		rtn = true;

	rpc->buildResponse("b", rtn);

done:
	if(img)
		Bayonne::endImage(img);
}

static void server_isService(BayonneRPC *rpc)
{
	ScriptImage *img = Bayonne::useImage();
	char buffer[80];
	const char *id;
	bool rtn = false;

	if(rpc->getCount() != 1)
	{
		rpc->sendFault(3, "Invalid Parameters");
		goto done;
	}

	id = rpc->getIndexed(1);
	if(!id || !*id)
	{
		rpc->sendFault(4, "Script Missing");
		goto done;
	}

	snprintf(buffer, sizeof(buffer), "service.%s", id);
	if(img->getPointer(buffer))
		rtn = true;

	rpc->buildResponse("b", rtn);

done:
	if(img)
		Bayonne::endImage(img);
}

static void server_isSelect(BayonneRPC *rpc)
{
	ScriptImage *img = Bayonne::useImage();
	Script::Name *scr;
	const char *id;
	bool rtn = false;

	if(rpc->getCount() != 1)
	{
		rpc->sendFault(3, "Invalid Parameters");
		goto done;
	}

	id = rpc->getIndexed(1);
	if(!id || !*id)
	{
		rpc->sendFault(4, "Script Missing");
		goto done;
	}

	scr = img->getScript(id);
	if(scr && scr->select)
		rtn = true;

	rpc->buildResponse("b", rtn);

done:
	if(img)
		Bayonne::endImage(img);
}

static void server_listDestinations(BayonneRPC *rpc)
{
	const char *list[1024];
	ScriptImage *img = Bayonne::useImage();
	unsigned count = 0, pos = 0;

	if(rpc->getCount())
    {
        rpc->sendFault(3, "Invalid Parameters");
        goto done;
    }

	rpc->buildResponse("[");
	if(img)
		count = BayonneBinder::gatherDestinations(img, list, 1024);
	while(pos < count)
		rpc->buildResponse("!s", list[pos++]);
	rpc->buildResponse("]");
done:
	if(img)
		Bayonne::endImage(img);
}

static void server_isDestination(BayonneRPC *rpc)
{
	const char *dest;

	if(rpc->getCount() != 1)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	dest = rpc->getIndexed(1);
	if(!dest || !*dest)
	{
		rpc->sendFault(4, "Invalid Parameter Value");
		return;
	}

	rpc->buildResponse("b", (rpcbool_t)BayonneBinder::isDestination(dest));
}

static void server_getPaths(BayonneRPC *rpc)
{
    if(rpc->getCount())
        rpc->sendFault(3, "Invalid Parameters");
    else
        rpc->buildResponse("(ss)",
            "prefix", Server::getLast("prefix"),
            "config", Server::getLast("config"),
			"prompts", Server::getLast("prompts"),
			"logfiles", Server::getLast("logfiles"),
			"runfiles", Server::getLast("runfiles"),
			"tmp", Server::getLast("tmp"),
			"tmpfs", Server::getLast("tmpfs")
        );
}
	
static void server_status(BayonneRPC *rpc)
{
	if(rpc->getCount())
		rpc->sendFault(3, "Invalid Parameters");
	else
		rpc->buildResponse("(isis)",
			"uptime", (rpcint_t)Server::uptime(), 
			"level", Server::getLevel(),
			"calls", (rpcint_t)Server::total_active_calls,
			"states", Server::getStatus()
		);
}

static void server_setSymbol(BayonneRPC *rpc)
{
	const char *var, *value;
	bool rtn;

	if(rpc->getCount() != 2)
	{
		rpc->sendFault(3, "Invalid parameters");
		return;
	}

	if(!rpc->transport.authorized)
	{
		rpc->transportFault(401, "Not Authorized");
		return;
	}

	var = rpc->getIndexed(1);
	value = rpc->getIndexed(2);

	if(!var || !value || !*var)
	{
		rpc->sendFault(4, "Invalid parameter value");
		return;
	}

	rtn = BayonneSession::setGlobal(var, value);
	rpc->buildResponse("b", (rpcbool_t)rtn);
}

static void server_createSymbol(BayonneRPC *rpc)
{
	const char *var, *cp;
	unsigned size;
	bool rtn;

	if(rpc->getCount() != 2)
	{
		rpc->sendFault(3, "Invalid parameters");
		return;
	}

	if(!rpc->transport.authorized)
	{
		rpc->transportFault(401, "Not Authorized");
		return;
	}

	var = rpc->getIndexed(1);
	cp = rpc->getIndexed(2);
	if(!cp)
		cp = "0";
	size = atoi(cp);

	if(!var || *var || !size)
	{
		rpc->sendFault(4, "Invalid parameter value");
		return;
	}

	rtn = BayonneSession::sizeGlobal(var, size);
	rpc->buildResponse("b", (rpcbool_t)rtn);
}


static void server_addSymbol(BayonneRPC *rpc)
{
	const char *var, *value;
	bool rtn;

	if(rpc->getCount() != 2)
	{
		rpc->sendFault(3, "Invalid parameters");
		return;
	}

	if(!rpc->transport.authorized)
	{
		rpc->transportFault(401, "Not Authorized");
		return;
	}

	var = rpc->getIndexed(1);
	value = rpc->getIndexed(2);

	if(!var || !value || !*var)
	{
		rpc->sendFault(4, "Invalid parameter value");
		return;
	}

	rtn = BayonneSession::addGlobal(var, value);
	rpc->buildResponse("b", (rpcbool_t)rtn);
}


static void server_clearSymbol(BayonneRPC *rpc)
{
	const char *var;
	bool rtn;

	if(rpc->getCount() != 1)
	{
		rpc->sendFault(3, "Invalid parameters");
		return;
	}

	if(!rpc->transport.authorized)
	{
		rpc->transportFault(401, "Not Authorized");
		return;
	}

	var = rpc->getIndexed(1);

	if(!var || !*var)
	{
		rpc->sendFault(4, "Invalid parameter value");
		return;
	}

	rtn = BayonneSession::clearGlobal(var);
	rpc->buildResponse("b", (rpcbool_t)rtn);
}


static void server_setRunning(BayonneRPC *rpc)
{
	if(rpc->getCount())
	{
		rpc->sendFault(3, "Invalid parameters");
		return;
	}

	if(!rpc->transport.authorized)
	{
		rpc->transportFault(401, "Not Authorized");
		return;
	}

	Server::setRunning();
	rpc->sendSuccess();
}

static void server_setService(BayonneRPC *rpc)
{
	const char *level;

	if(rpc->getCount() != 1)
	{
		rpc->sendFault(3, "Invalid parameters");
		return;
	}

	if(!rpc->transport.authorized)
	{
		rpc->transportFault(401, "Not Authorized");
		return;
	}

	level = rpc->getIndexed(1);
	if(!level || !*level)
	{
		rpc->sendFault(4, "Invalid parameter value");
		return;
	}

	Server::setService(level);
	return rpc->sendSuccess();
}

static void server_getInfo(BayonneRPC *rpc)
{
	const char *node = Server::getLast("node");

	if(!node)
		node = "unknown";

	if(rpc->getCount())
		rpc->sendFault(3, "Invalid Parameters");
	else
		rpc->buildResponse("(titissii)",
			"started", Server::getStarted(),
			"started_int", (rpcint_t)Server::getStarted(),
			"reloaded", Server::getReloaded(),
			"reloaded_int", (rpcint_t)Server::getReloaded(),
			"node", node,
			"version", VERSION,
			"timeslots", Server::getTimeslotsUsed(),
			"spans", BayonneSpan::getSpans()
		);
}

static Bayonne::RPCDefine dispatch[] = {
	{"version", server_version,
		"API version number of server rpc", "struct"},
	{"getPaths", server_getPaths,
		"Return server directory layout", "struct"},
	{"status", server_status,
		"Return bayonne runtime status information", "struct"},
	{"keymap", server_keymap,
		"Return server keymap configuration", "string"},
	{"callstats", server_callstats,
		"Return server global call stats", "struct"},
	{"getInfo", server_getInfo,
		"Return static server information", "struct"},
	{"errlog", server_errlog,
		"Send error message to server log", "nil, string, string"},
	{"reload", server_reload,
		"Reload server configuration while running", "nil"},
	{"update", server_update,
		"Notify update reload request", "nil"},
	{"setLevel", server_setService,
		"Set service level for the server", "nil, string"},
	{"clearLevel", server_setRunning,
		"Clear server service level", "nil"},
	{"setSymbol", server_setSymbol,
		"Set global symbol", "boolean, string, string"},
	{"addSymbol", server_addSymbol,
		"Append global symbol", "boolean, string, string"},
	{"clearSymbol", server_clearSymbol,
		"Clear global symbol", "boolean, string"},
	{"createSymbol", server_createSymbol,
		"Create global symbol of specified size", "boolean, string, int"},
	{"isService", server_isService,
		"Check if a given service is defined", "boolean, string"},
	{"isScript", server_isScript,
		"Check if a given script exists", "boolean, string"},
	{"isSelect", server_isSelect,
		"Check if a given script is selectable", "boolean, string"},
	{"isDestination", server_isDestination,
		"Check if a given destination is configured", "boolean, string"},
	{"listDestinations", server_listDestinations,
		"List configured destinations for this server", "list"},
	{NULL, NULL, NULL, NULL}};

static Bayonne::RPCNode xmlrpc_server("server", dispatch);

}; // namespace

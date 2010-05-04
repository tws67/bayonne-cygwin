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

static void driver_version(BayonneRPC *rpc)
{
    if(rpc->getCount())
    {
        rpc->sendFault(3, "Invalid Parameters");
        return;
    }
	rpc->buildResponse("(ii)", 
		"current", (rpcint_t)2,
		"prior", (rpcint_t)2);
}

static void driver_getInfo(BayonneRPC *rpc)
{
	BayonneDriver *driver = NULL;
	const char *id, *type, *proto, *iface;

	if(rpc->getCount() != 1)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	id = rpc->getIndexed(1);
	if(id)
		driver = BayonneDriver::get(id);
	if(!id || !driver)
	{
		rpc->sendFault(4, "Driver Id Invalid");
		return;
	}
	
	type = driver->getLast("type");
	proto = driver->getLast("proto");
	iface = driver->getLast("interface");

	if(!type)
		type = "port";

	if(!proto)
		proto = "none";

	if(!iface)
		iface = "none";

	rpc->buildResponse("(sssiiii)",
		"type", type,
		"protocol", proto,
		"interface", iface,
		"tfirst", (rpcint_t)driver->getFirst(),
		"tcount", (rpcint_t)driver->getCount(),
		"sfirst", (rpcint_t)driver->getSpanFirst(),
		"scount", (rpcint_t)driver->getSpansUsed()
	);
}

static void driver_suspend(BayonneRPC *rpc)
{
	BayonneDriver *driver = NULL;
	const char *id;
	bool rtn;

	if(rpc->getCount() != 1)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	if(!rpc->transport.authorized)
	{
		rpc->transportFault(401, "Not Authorized");
		return;
	}

	id = rpc->getIndexed(1);
	if(id)
		driver = BayonneDriver::get(id);
	if(!id || !driver)
	{
		rpc->sendFault(4, "Driver Id Invalid");
		return;
	}

	rtn = driver->suspend();
	rpc->buildResponse("b", (rpcbool_t)rtn);
}

static void driver_resume(BayonneRPC *rpc)
{
	BayonneDriver *driver = NULL;
	const char *id;
	bool rtn;

	if(rpc->getCount() != 1)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	if(!rpc->transport.authorized)
	{
		rpc->transportFault(401, "Not Authorized");
		return;
	}

	id = rpc->getIndexed(1);
	if(id)
		driver = BayonneDriver::get(id);
	if(!id || !driver)
	{
		rpc->sendFault(4, "Driver Id Invalid");
		return;
	}

	rtn = driver->resume();
	rpc->buildResponse("b", (rpcbool_t)rtn);
}

static void driver_deregisterRegistry(BayonneRPC *rpc)
{
	BayonneDriver *driver = NULL;
	const char *id;
	bool rtn;

    if(rpc->getCount() != 1)
    {
        rpc->sendFault(3, "Invalid Parameters");
        return;
    }

    if(!rpc->transport.authorized)
    {
        rpc->transportFault(401, "Not Authorized");
        return;
    }

    id = rpc->getNamed(1, "driver");
    if(id)
        driver = BayonneDriver::get(id);
    if(!id || !driver)
    {
        rpc->sendFault(4, "Driver Id Invalid");
        return;
    }

	id = rpc->getNamed(1, "registry");
	if(!id)
		id = rpc->getIndexed(2);

	if(!id)
	{
        rpc->sendFault(4, "Registry Id Invalid");
        return;
	}

	rtn = driver->deregister(id);
	rpc->buildResponse("b", rtn);
}	

static void driver_registerRegistry(BayonneRPC *rpc)
{
	BayonneDriver *driver = NULL;
	const char *id, *uri, *cp;
	const char *secret;
	bool rtn = false;

    if(rpc->getCount() != 1)
    {
        rpc->sendFault(3, "Invalid Parameters");
        return;
    }

    if(!rpc->transport.authorized)
    {
        rpc->transportFault(401, "Not Authorized");
        return;
    }

    id = rpc->getNamed(1, "driver");
    if(id)
        driver = BayonneDriver::get(id);
    if(!id || !driver)
    {
        rpc->sendFault(4, "Driver Id Invalid");
        return;
    }

	id = rpc->getNamed(1, "registry");
	if(!id || !*id)
	{
        rpc->sendFault(4, "Registry Entry Invalid");
        return;
	}

	uri = rpc->getNamed(1, "identity");
	if(!uri || !*uri)
	{
        rpc->sendFault(4, "Registry Identity Invalid");
        return;
    }

    secret = rpc->getNamed(1, "secret");
    if(!secret || !*secret)
    {
        rpc->sendFault(4, "Registry Secret Invalid");
        return;
    }

    cp = rpc->getNamed(1, "expires");
    if(!cp || !*cp)
    {
        rpc->sendFault(4, "Registry Duration Invalid");
        return;
    }

	rtn = driver->reregister(id, uri, secret, atol(cp));
	rpc->buildResponse("b", rtn);
}	

static void driver_reregister(BayonneRPC *rpc)
{
	BayonneDriver *driver = NULL;
	const char *id;

	if(rpc->getCount() != 1)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	if(!rpc->transport.authorized)
	{
		rpc->transportFault(401, "Not Authorized");
		return;
	}

	id = rpc->getIndexed(1);
	if(id)
		driver = BayonneDriver::get(id);
	if(!id || !driver)
	{
		rpc->sendFault(4, "Driver Id Invalid");
		return;
	}

	driver->reregister();
	rpc->sendSuccess();
}

static void driver_listRegistry(BayonneRPC *rpc)
{
	registry_t *registry;
	BayonneDriver *driver = NULL;
	const char *id;
	unsigned count, index = 0;

	if(rpc->getCount() != 1)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	id = rpc->getIndexed(1);
	if(id)
		driver = BayonneDriver::get(id);
	if(!id || !driver)
	{
		rpc->sendFault(4, "Driver Id Invalid");
		return;
	}

	registry = new registry_t[1024];
	count = driver->getRegistration(registry, 1024);
	rpc->buildResponse("[");
	while(index < count)
		rpc->buildResponse("!s", registry[index++].userid);
	rpc->buildResponse("]");
	delete[] registry;
}

static void driver_getRegistry(BayonneRPC *rpc)
{
	registry_t registry;
	BayonneDriver *driver = NULL;
	const char *id, *rid;
	unsigned count;

	if(rpc->getCount() != 1)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	id = rpc->getNamed(1, "driver");
	if(id)
		driver = BayonneDriver::get(id);
	if(!id || !driver)
	{
		rpc->sendFault(4, "Driver Id Invalid");
		return;
	}

	rid = rpc->getNamed(1, "registry");
	if(!rid || !*rid)
	{
		rpc->sendFault(4, "Registry Id Invalid");
		return;
	}
	count = driver->getRegistration(&registry, 1, rid);
	if(count < 1)
	{
		rpc->sendFault(4, "Registry Id Invalid");
		return;
	}
	rpc->buildResponse("(sssiiiiiiti)",
		"remote", registry.remote,
		"type", registry.type,
		"status", registry.status,
		"calls", (rpcint_t)registry.active_calls,
		"limit", (rpcint_t)registry.call_limit,
		"oattempts", (rpcint_t)registry.attempts_oCount,
		"iattempts", (rpcint_t)registry.attempts_iCount,
		"ocomplete", (rpcint_t)registry.complete_oCount,
		"icomplete", (rpcint_t)registry.complete_iCount,
		"updated", registry.updated,
		"updated_int", (rpcint_t)registry.updated);
}

static void driver_listRegistryOfType(BayonneRPC *rpc)
{
	registry_t *registry;
	BayonneDriver *driver = NULL;
	const char *id, *type;
	unsigned count, index = 0;

	if(rpc->getCount() != 2)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	id = rpc->getIndexed(1);
	if(id)
		driver = BayonneDriver::get(id);
	if(!id || !driver)
	{
		rpc->sendFault(4, "Driver Id Invalid");
		return;
	}

	type = rpc->getIndexed(2);
	if(!type || !*type)
	{
		rpc->sendFault(4, "Registry Type Invalid");
		return;
	}

	registry = new registry_t[1024];
	count = driver->getRegistration(registry, 1024);
	rpc->buildResponse("[");
	while(index < count)
	{
		if(!stricmp(registry[index].type, type))
			rpc->buildResponse("!s", registry[index].userid);
		++index;
	}
	rpc->buildResponse("]");
	delete[] registry;
}

static void driver_list(BayonneRPC *rpc)
{
	BayonneDriver *driver = BayonneDriver::getRoot();
	
	if(rpc->getCount())
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	rpc->buildResponse("[");
	while(driver)
	{
		rpc->buildResponse("!s", driver->getName());
		driver = driver->getNext();
	}
	rpc->buildResponse("]");
}

static void driver_callstats(BayonneRPC *rpc)
{
	BayonneDriver *driver = NULL;
	const char *id;

	if(rpc->getCount() != 1)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	id = rpc->getIndexed(1);
	if(id)
		driver = BayonneDriver::get(id);
	if(!id || !driver)
		rpc->sendFault(4, "Driver Id Invalid");
	else
		rpc->buildResponse("(iiiiiii)",
			"iattempts", 
				(rpcint_t)driver->call_attempts.iCount,
			"oattempts", 
				(rpcint_t)driver->call_attempts.oCount,
			"sattempts",
				(rpcint_t)driver->call_attempts.getStamp(),
			"icomplete",
				(rpcint_t)driver->call_complete.iCount,
			"ocomplete",
				(rpcint_t)driver->call_complete.oCount,
			"scomplete",
				(rpcint_t)driver->call_complete.getStamp(),
			"active",
				(rpcint_t)driver->active_calls
		);
}

static Bayonne::RPCDefine dispatch[] = {
	{"version", driver_version,
		"API version of driver rpc", "int"},
	{"suspend", driver_suspend,
		"Suspend driver", "boolean, string"},
	{"resume", driver_resume,
		"Resume driver", "boolean, string"},
	{"reregister", driver_reregister,
		"Reregister driver", "nil, string"},
	{"listRegistry", driver_listRegistry,
		"List driver registration entries", "array, string"},
	{"listRegistryOfType", driver_listRegistryOfType,
		"List driver registration entries of specified type", 
		"array, string, string"},
	{"getRegistry", driver_getRegistry,
		"Get a registration record for the driver",
		"struct, struct"},
	{"deregisterRegistry", driver_deregisterRegistry,
		"Clear a dynamic registration record",
		"boolean, struct"},
    {"registerRegistry", driver_registerRegistry,
        "Clear a dynamic registration record",
        "boolean, struct"},
	{"getInfo", driver_getInfo,
		"List driver information", "struct, string"},
	{"list", driver_list,
		"List drivers loaded into server", "array"},
	{"callstats", driver_callstats,
		"List drtiver specific call statistics", "struct, string"},
	{NULL, NULL, NULL, NULL}};

static Bayonne::RPCNode xmlrpc_driver("driver", dispatch);

}; // namespace


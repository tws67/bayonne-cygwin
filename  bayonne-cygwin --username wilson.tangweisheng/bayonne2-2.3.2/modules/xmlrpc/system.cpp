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

static void system_identity(BayonneRPC *rpc)
{
	if(rpc->getCount())
		rpc->sendFault(3, "Invalid Parameters");
	else
		rpc->buildResponse("s", "Bayonne/" VERSION);
}

static void system_status(BayonneRPC *rpc)
{
	time_t now, started;

	if(rpc->getCount())
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	time(&now);
	started = now - Bayonne::uptime();
	rpc->buildResponse("(titissi)",
		"date", now, 
		"date_int", (rpcint_t)now,
		"started", started,
		"started_int", (rpcint_t)started,
		"name", "Bayonne",
		"version", VERSION,
		"methods_known", (rpcint_t)Bayonne::RPCNode::getCount()
	);
}

static void system_methodHelp(BayonneRPC *rpc)
{
	Bayonne::RPCNode *node = Bayonne::RPCNode::getFirst();
	Bayonne::RPCDefine *methods;
	char *prefix, *method;

	if(rpc->getCount() != 1)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	prefix = (char *)rpc->getIndexed(1);
	if(!prefix || !*prefix)
	{
		rpc->sendFault(4, "Invalid Method Argument");
		return;
	}
	method = strchr(prefix, '.');
	if(method)
		*(method++) = 0;
	else
	{
		rpc->sendFault(4, "Invalid Method Argument");
		return;
	}

	while(node)
	{
		if(!stricmp(node->getPrefix(), prefix))
			methods = node->getMethods();
		else
			methods = NULL;
		while(methods && methods->name)
		{
			if(!stricmp(methods->name, method))
			{
				rpc->buildResponse("s", methods->help);
				return;
			}
			++methods;
		}
		node = node->getNext();
	}
	rpc->sendFault(4, "Unknown Method");
}

static void system_methodSignature(BayonneRPC *rpc)
{
	Bayonne::RPCNode *node = Bayonne::RPCNode::getFirst();
	Bayonne::RPCDefine *methods;
	char *prefix, *method;
	unsigned count = 0;

	if(rpc->getCount() != 1)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	prefix = (char *)rpc->getIndexed(1);
	if(!prefix || !*prefix)
	{
		rpc->sendFault(4, "Invalid Method Argument");
		return;
	}
	method = strchr(prefix, '.');
	if(method)
		*(method++) = 0;
	else
	{
		rpc->sendFault(4, "Invalid Method Argument");
		return;
	}

	while(node)
	{
		if(!stricmp(node->getPrefix(), prefix))
			methods = node->getMethods();
		else
			methods = NULL;
		while(methods && methods->name)
		{
			if(!stricmp(methods->name, method))
			{
				if(!count)
					rpc->buildResponse("[");

				rpc->buildResponse("!s", methods->signature);
				++count;
			}
			++methods;
		}
		node = node->getNext();
	}
	if(!count)		
		rpc->sendFault(4, "Unknown Method");
	else
		rpc->buildResponse("]");
}


static void system_listMethods(BayonneRPC *rpc)
{
	Bayonne::RPCNode *node = Bayonne::RPCNode::getFirst();
	Bayonne::RPCDefine *methods;
	char buffer[65];

	if(rpc->getCount())
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	rpc->buildResponse("[");

	while(node)
	{
		methods = node->getMethods();
		while(methods->name)
		{
			snprintf(buffer, sizeof(buffer), "%s.%s",
				node->getPrefix(), methods->name);
			rpc->buildResponse("!s", buffer);				
			++methods;
		}
		node = node->getNext();
	}
	rpc->buildResponse("]");
}

static Bayonne::RPCDefine dispatch[] = {
	{"identity", system_identity, 
		"Identify server type and version", "string"}, 
	{"listMethods", system_listMethods, 
		"List server methods", "array"},
	{"methodHelp", system_methodHelp, 
		"Get help text for method", "string, string"},
	{"methodSignature", system_methodSignature, 
		"Get paramater signature for specified method", "array, string"},
	{"status", system_status,
		"Return server status information", "struct"},
	{NULL, NULL, NULL, NULL}};

static Bayonne::RPCNode xmlrpc_system("system", dispatch);

}; // namespace

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

static void span_version(BayonneRPC *rpc)
{
    if(rpc->getCount() != 1)
    {
        rpc->sendFault(3, "Invalid Parameters");
        return;
    }
	rpc->buildResponse("i", (rpcint_t)1);
}

static void span_getInfo(BayonneRPC *rpc)
{
	BayonneSpan *span = NULL;
	const char *id;

	if(rpc->getCount() != 1)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	id = rpc->getIndexed(1);
	if(id)
		span = BayonneSpan::get(atoi(id));	
	if(!id || !span)
	{
		rpc->sendFault(4, "Span Id Invalid");
		return;
	}
	
	rpc->buildResponse("(ii)",
		"tfirst", (rpcint_t)span->getFirst(),
		"tcount", (rpcint_t)span->getCount()
	);
}

static void span_suspend(BayonneRPC *rpc)
{
	BayonneSpan *span = NULL;
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
		span = BayonneSpan::get(atoi(id));
	if(!span)
	{
		rpc->sendFault(4, "Span Id Invalid");
		return;
	}

	rtn = span->suspend();
	rpc->buildResponse("b", (rpcbool_t)rtn);
}

static void span_resume(BayonneRPC *rpc)
{
	BayonneSpan *span = NULL;
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
		span = BayonneSpan::get(atoi(id));
	if(!span)
	{
		rpc->sendFault(4, "Span Id Invalid");
		return;
	}

	rtn = span->resume();
	rpc->buildResponse("b", (rpcbool_t)rtn);
}

static void span_reset(BayonneRPC *rpc)
{
	BayonneSpan *span = NULL;
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
		span = BayonneSpan::get(atoi(id));
	if(!span)
	{
		rpc->sendFault(4, "Span Id Invalid");
		return;
	}

	rtn = span->reset();
	rpc->buildResponse("b", (rpcbool_t)rtn);
}


static void span_callstats(BayonneRPC *rpc)
{
	BayonneSpan *span = NULL;
	const char *id;

	if(rpc->getCount() != 1)
	{
		rpc->sendFault(3, "Invalid Parameters");
		return;
	}

	id = rpc->getIndexed(1);
	if(id)
		span = BayonneSpan::get(atoi(id));
	if(!span)
		rpc->sendFault(4, "Span Id Invalid");
	else
		rpc->buildResponse("(iiiiiii)",
			"iattempts", 
				(rpcint_t)span->call_attempts.iCount,
			"oattempts", 
				(rpcint_t)span->call_attempts.oCount,
			"sattempts",
				(rpcint_t)span->call_attempts.getStamp(),
			"icomplete",
				(rpcint_t)span->call_complete.iCount,
			"ocomplete",
				(rpcint_t)span->call_complete.oCount,
			"scomplete",
				(rpcint_t)span->call_complete.getStamp(),
			"active",
				(rpcint_t)span->active_calls
		);
}

static Bayonne::RPCDefine dispatch[] = {
	{"version", span_version,
		"API version number of span rpc", "struct"},
	{"suspend", span_suspend,
		"Suspend span", "boolean, int"},
	{"resume", span_resume,
		"Resume span", "boolean, int"},
	{"reset", span_reset,
		"Reset span", "boolean, int"},
	{"getInfo", span_getInfo,
		"List span information", "struct, int"},
	{"callstats", span_callstats,
		"List span specific call statistics", "struct, int"},
	{NULL, NULL, NULL, NULL}};

static Bayonne::RPCNode xmlrpc_span("span", dispatch);

}; // namespace


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

#include <bayonne.h>
#include <cc++/slog.h>
#include "bayonne_rpc.h"
#include "bayonne_svc.c"
#include "bayonne_xdr.c"

using namespace ost;
using namespace std;

class RPCServer : private BayonneService, private Keydata
{
private:
	bool active;
	void run(void);

	void tcpService(void);
	void udpService(void);

	inline unsigned getSendBuffer(void)
		{return atoi(getLast("send"));};
	inline unsigned getRecvBuffer(void)
		{return atoi(getLast("recv"));};

public:
	RPCServer();
}	sunrpc;

RPCServer::RPCServer() :
BayonneService(-1, 0), Keydata("/bayonne/module/sunrpc")
{
	static Keydata::Define defkeys[] = {
	{"protocols", "tcp udp"},
	{"send", "0"},
	{"recv", "0"},
	{NULL, NULL}};

	load(defkeys);
	active = false;

#ifdef  WIN32
        const char *env = Process::getEnv("MODULECONFIG");

        if(env)
                loadFile(env, "sunrpc");
#endif
}

void RPCServer::run(void)
{
	char probuf[65];
	char *pro;
	char *tok;

	pmap_unset(BAYONNE_PROGRAM, BAYONNE_VERSION);
	strcpy(probuf, getLast("protocols"));
	pro = strtok_r(probuf, " \t,;:", &tok);

	slog.debug("sunrpc service started");

	active = false;	
	while(pro)
	{
		if(!stricmp(pro, "tcp"))
			tcpService();

		if(!stricmp(pro, "udp"))
			udpService();

		pro = strtok_r(NULL, " \t,;:", &tok);
	}

	if(!active)
	{
		slog(Slog::levelError) << "rpc: no services active" << endl;
		return;
	}

	setCancel(cancelImmediate);
	svc_run();
}

void RPCServer::udpService(void)
{
	register SVCXPRT *transp = svcudp_create(RPC_ANYSOCK);
	if(!transp)
		slog(Slog::levelError) << "rpc: cannot create udp service" << endl;
	else if(!svc_register(transp, BAYONNE_PROGRAM, BAYONNE_VERSION, &bayonne_program_2, IPPROTO_UDP))
		slog(Slog::levelError) << "rpc: cannot register udp service" << endl;
	else
		active = true;
}

void RPCServer::tcpService(void)
{
	register SVCXPRT *transp = svctcp_create(RPC_ANYSOCK, getSendBuffer(), getRecvBuffer());
	if(!transp)
		slog(Slog::levelError) << "rpc: cannot create tcp service" << endl;
 	else if(!svc_register(transp, BAYONNE_PROGRAM, BAYONNE_VERSION, &bayonne_program_2, IPPROTO_TCP))
		slog(Slog::levelError) << "rpc: cannot register tcp service" << endl;
	else
		active = true;
}

	

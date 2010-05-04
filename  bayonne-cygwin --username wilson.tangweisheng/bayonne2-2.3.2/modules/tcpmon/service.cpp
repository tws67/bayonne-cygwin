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

namespace moduleTCP {
using namespace ost;
using namespace std;

Service Service::tcp;
bool Service::active = false;
unsigned char Service::sequence[sizeof(fd_set) * 8];

static Keydata::Define defkeys[] = {
    {"interface", "127.0.0.1"},
    {"port", "5555"},
    {"secret", "pass"},
    {"zeroconf", "yes"},
    {NULL, NULL}};

Service::Service() :
BayonneService(0, 0), 
BayonneZeroconf("_tcpmon._tcp", ZEROCONF_IPV4),
StaticKeydata("/bayonne/module/tcpmon", defkeys)
{
	if(getBoolean("zeroconf"))
		zeroconf_port = getValue("port");	
}

void Service::stopService(void)
{
	active = false;
	terminate();
}

void Service::run(void)
{
	InetAddress addr(getString("interface"));
	tpport_t port = getValue("port");
	TCPSocket tcp(addr, port);
	Session *session;

	active = true;
	slog.debug("tcpmon service started");

	for(;;)
	{
                if(tcp.isPendingConnection(30))
                {
                        session = new Session(tcp);
                        session->detach();
                }
		Thread::yield();
	}

	Thread::sync();
}

} // namespace 

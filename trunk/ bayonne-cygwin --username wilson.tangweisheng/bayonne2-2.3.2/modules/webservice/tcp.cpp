// Copyright (C) 2006 David Sugar, Tycho Softworks.
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

namespace moduleWebservice {
using namespace ost;
using namespace std;

TCP *TCP::first = NULL;
fd_set TCP::selectors;
fd_set TCP::input;
SOCKET TCP::last = 0;
SOCKET TCP::hiwater = 0;
TCP *TCP::index[sizeof(fd_set) * 8];

TCP::TCP(SOCKET so)
{
	if(!first)
	{
		FD_ZERO(&selectors);
		FD_ZERO(&input);
	}

	next = first;
	first = this;
	FD_SET(so, &selectors);
	index[so] = this;
	if(++so > last)
	{
		last = so;
		hiwater = so;
	}
}	

TCP *TCP::getSelect(void)
{
	for(;;)
	{
		while(last < hiwater)
		{
			if(FD_ISSET(last, &input))
				return index[last++];
			++last;
		}
		memcpy(&input, &selectors, sizeof(input));
		last = 0;
		if(select(hiwater, &input, NULL, NULL, NULL) < 1)
			return NULL;
	}
}

void TCP::endSockets(void)
{
	TCP *tcp = first;
	while(tcp)
	{
		tcp->disconnect();
		tcp = tcp->next;
	}
	first = NULL;
}

tpport_t TCP::getPort(void)
{
	return Service::webservice.getValue("port");
}

unsigned TCP::getBacklog(void)
{
	return Service::webservice.getValue("backlog");
}

size_t TCP::getInputBuffer(void)
{
	return Service::webservice.getValue("input");
}

size_t TCP::getOutputBuffer(void)
{
	return Service::webservice.getValue("output");
}

unsigned TCP::getSegment(void)
{
	return Service::webservice.getValue("segment");
}

IPV4Address TCP::getV4Address(const char *iface)
{
	return IPV4Address(iface);
}

#ifdef	CCXX_IPV6
IPV6Address TCP::getV6Address(const char *iface)
{
	return IPV6Address(iface);
}

#endif

TCPV4::TCPV4(const char *iface) :
TCPSocket(getV4Address(iface), getPort(), getBacklog(), getSegment()),
TCP(getSocket())
{
	sendBuffer(TCP::getOutputBuffer());
	receiveBuffer(TCP::getInputBuffer());
}

void TCPV4::disconnect(void)
{
	endSocket();
}

SOCKET TCPV4::getAccept(void)
{
	return accept(so, NULL, NULL);
}

#ifdef	CCXX_IPV6

TCPV6::TCPV6(const char *iface) :
TCPV6Socket(getV6Address(iface), getPort(), getBacklog(), getSegment()),
TCP(getSocket())
{
        sendBuffer(TCP::getOutputBuffer());
	receiveBuffer(TCP::getInputBuffer());
}

void TCPV6::disconnect(void)
{
	endSocket();
}

SOCKET TCPV6::getAccept(void)
{
	return accept(so, NULL, NULL);
}	

#endif
		
} // namespace

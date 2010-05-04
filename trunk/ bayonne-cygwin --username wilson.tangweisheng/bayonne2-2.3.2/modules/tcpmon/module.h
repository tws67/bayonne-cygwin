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
#include <cc++/socket.h>

namespace moduleTCP {
using namespace ost;
using namespace std;

class Service : public BayonneService, public BayonneZeroconf, public StaticKeydata
{
private:
	void run(void);
	void stopService(void);

public:
	static unsigned char sequence[];
	static bool active;
	static Service tcp;
	Service();
};

class Session : public TCPSession, public Bayonne
{
private:
	char *monmap;

	void help(void);
	void calls(char **list);
	void allcalls(void);
	void spans(void);
	void reg(void);
	void monitor(char **list);
	void nomon(char **list);
	void refer(char **list);
	void drivers(char **list);
	void uptime(void);
	void diskstat(char **list);
        void run(void);
	void final(void);

public:
        Session(TCPSocket &server);
};

} // namespace

	

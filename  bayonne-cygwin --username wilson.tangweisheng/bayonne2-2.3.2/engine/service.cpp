// Copyright (C) 2005 Open Source Telecom Corp.
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

#include "engine.h"

using namespace ost;
using namespace std;

BayonneService *BayonneService::first = NULL;
BayonneService *BayonneService::last = NULL;

BayonneService::BayonneService(int pri, size_t stack) :
Thread(pri, stack)
{
	next = first;
	first = this;
}

void BayonneService::startService(void)
{
	Thread::start();
}

void BayonneService::stopService(void)
{
	terminate();
}

void BayonneService::enteringCall(BayonneSession *child)
{
}

void BayonneService::exitingCall(BayonneSession *child)
{
}

void BayonneService::detachSession(BayonneSession *s)
{
}

void BayonneService::attachSession(BayonneSession *s)
{
}

void BayonneService::start(void)
{
	BayonneService *svc = first;

	while(svc && svc != last)
	{
		last = svc;
		svc->startService();
		svc = svc->next;
	}
}

void BayonneService::stop(void)
{ 
        BayonneService *svc = first;         

        while(svc)
        {
                svc->stopService();
                svc = svc->next;
        }
}


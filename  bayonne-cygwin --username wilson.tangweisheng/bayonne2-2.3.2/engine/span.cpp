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

BayonneSpan *BayonneSpan::first = NULL;
BayonneSpan *BayonneSpan::last = NULL;
BayonneSpan **BayonneSpan::index = NULL;
unsigned BayonneSpan::spans = 0;

BayonneSpan::BayonneSpan(BayonneDriver *d, timeslot_t ports) :
Keydata()
{
	char name[128];

	snprintf(name, sizeof(name), "/bayonne/spans/%d", spans);
	load(name);

	id = spans++;
	if(first)
		last->next = this;
	else
		first = last = this;

	driver = d;
	next = NULL;
	timeslot = ts_used;
	count = ports;
	used = 0;
	active_calls = 0;
}

bool BayonneSpan::suspend(void)
{
	return false;
}

bool BayonneSpan::resume(void)
{
	return false;
}

bool BayonneSpan::reset(void)
{
	return false;
}

BayonneSession *BayonneSpan::getTimeslot(timeslot_t ts)
{
	if(ts >= count)
		return NULL;

	return getSession(timeslot + ts);
}

void BayonneSpan::allocate(unsigned count)
{
	BayonneSpan *span = first;

	if(!count)
		count = spans;

	if(index)
		return;

	index = new BayonneSpan*[spans];
	while(span)
	{
		index[span->id] = span;
		span = span->next;
	}
}

BayonneSpan *BayonneSpan::get(unsigned id)
{
        if(!index && spans)
                allocate();

        if(id >= spans)
                return NULL;

        return index[id];
}


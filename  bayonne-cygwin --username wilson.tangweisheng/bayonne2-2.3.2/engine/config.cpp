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

BayonneConfig *BayonneConfig::first = NULL;
BayonneZeroconf *BayonneZeroconf::zeroconf_first = NULL;

BayonneZeroconf::BayonneZeroconf(const char *t, zeroconf_family_t family)
{
	zeroconf_family = family;
	zeroconf_type = t;
	zeroconf_port = 0;
	zeroconf_next = zeroconf_first;
	zeroconf_first = this;
}

BayonneConfig::BayonneConfig(const char *tag, Keydata::Define *def, const char *path) :
DynamicKeydata(path, def)
{
    char *list[128];
    char buffer[128];
    unsigned count;
    const char *cp, *name;
    unsigned pos = 0;
    char *up;

	id = tag;
	next = first;
	first = this;

    count = keys->getIndex(list, 128);
    while(pos < count)
    {
        name = list[pos++];
        cp = keys->getLast(name);
        if(!cp)
            continue;

        snprintf(buffer, sizeof(buffer), "%s_%s", id, name);
        up = buffer;
        while(*up)
        {
            *up = toupper(*up);
            ++up;
        }
        Process::setEnv(buffer, cp, true);
    }
}

void BayonneConfig::setEnv(const char *sym)
{
	char buffer[65];
	const char *cp;

	readLock();
	cp = keys->getLast(sym);

	if(!cp)
	{
		unlock();
		return;
	}

	snprintf(buffer, sizeof(buffer), "%s_%s", id, sym);
	setUpper(buffer, 0);
	Process::setEnv(buffer, cp, true);
	unlock();
}

BayonneConfig::BayonneConfig(const char *tag, const char *path) :
DynamicKeydata(path)
{
	id = tag;
	next = first;
	first = this;
}

void BayonneConfig::rebuild(ScriptImage *img)
{
	char buffer[128];
	unsigned count, pos;
	char *list[128];
	const char *cp, *id;
	char *up;

	BayonneConfig *cfg = first;
	
	while(cfg)
	{
		cfg->readLock();
		count = cfg->keys->getIndex(list, 128);
		pos = 0;
		while(pos < count)
		{
			id = list[pos++];
			cp = cfg->keys->getLast(id);
			if(!cp)
				continue;
			snprintf(buffer, sizeof(buffer), "%s.%s", cfg->id, id);
			img->setValue(buffer, cp);
			snprintf(buffer, sizeof(buffer), "%s_%s", cfg->id, id);
			up = buffer;
			while(*up)
			{
				*up = toupper(*up);
				++up;
			}
			Process::setEnv(buffer, cp, true);
		}
		cfg->unlock();
		cfg = cfg->next;
	}
}

BayonneConfig *BayonneConfig::get(const char *id)
{
	BayonneConfig *cfg = first;
	
	while(cfg)
	{
		if(!stricmp(cfg->id, id))
			return cfg;
		cfg = cfg->next;
	}
	return NULL;
}

	

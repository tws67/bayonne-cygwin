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

#include "module.h"

namespace binder {
using namespace ost;
using namespace std;

PersistProperty typePersist; 

ThreadLock PersistProperty::lock;
ScriptSymbols PersistProperty::syms;
bool PersistProperty::loaded = false;

PersistProperty::PersistProperty() :
ScriptProperty("persist")
{
}

void PersistProperty::set(const char *data, char *save, unsigned size) 
{
	persist_t *p = (persist_t *)save;
	Symbol *sym = p->sym;
	snprintf(p->cache, PERSIST_CACHE_SIZE, "%s", data);
	lock.writeLock();
	setString(sym->data, PERSIST_CACHE_SIZE, data);
	lock.unlock();
}

void PersistProperty::load(void)
{
	FILE *fp;
	char buf[128];
	char *cp;
	char *ep;
	Symbol *sym;

	snprintf(buf, sizeof(buf), "%s/bayonne.key", server->getLast("prefix"));
	fp = fopen(buf, "r");

	loaded = true;
	if(!fp)
		return;

	for(;;)
	{
		if(!fgets(buf, sizeof(buf), fp) || feof(fp))
			break;
		
		if(!isalnum(buf[0]))
			continue;

		cp = strchr(buf, ' ');
		if(!cp)
			continue;
		*(cp++) = 0;
		ep = strchr(cp, '\r');
		if(!ep)
			ep = strchr(cp, '\n');
		if(ep)
			*ep = 0;
		sym = syms.find(buf, PERSIST_CACHE_SIZE);
		if(!sym)
			continue;
		sym->type = ScriptProperty::symNORMAL;
		setString(sym->data, PERSIST_CACHE_SIZE, cp);
	}

	fclose(fp);
}

void PersistProperty::save(void)
{
	char sbuf[128];
	char tbuf[128];

	if(!loaded)
		return;

	Symbol **ind = new Symbol *[4096];
	Symbol **mem = ind;

	snprintf(sbuf, sizeof(sbuf), "%s/bayonne.key", server->getLast("prefix"));
	snprintf(tbuf, sizeof(tbuf), "%s/bayonne.tmp", server->getLast("prefix"));
	FILE *fp;
	
	ind[0] = NULL;
	remove(tbuf);
	fp = fopen(tbuf, "w");
	lock.writeLock();
	syms.gather(ind, 4095, "", NULL);
	while(*ind)
	{
		if((*ind)->data[0])
			fprintf(fp, "%s %s\n", (*ind)->id, (*ind)->data);
		++ind;
	}
	lock.unlock();
	fclose(fp);
	rename(tbuf, sbuf);
	delete[] mem;
}

bool PersistProperty::test(const char *id)
{
	lock.readLock();
	Symbol *sym = syms.find(id);
	lock.unlock();
	if(!sym)
		return false;
	if(!sym->data[0])
		return false;
	return true;
}

bool PersistProperty::remap(const char *id, char *save, const char *val)
{
	persist_t *p = (persist_t *)save;

	if(!val)
		val = "";

	lock.writeLock();
	Symbol *sym = syms.find(id, PERSIST_CACHE_SIZE);
	if(!sym)
	{
		lock.unlock();
		return false;
	}

	if(sym->type == ScriptProperty::symINITIAL)
	{
		sym->type = ScriptProperty::symNORMAL;
		sym->data[0] = 0;
		setString(p->cache, PERSIST_CACHE_SIZE, val);		
	}	
	else
		setString(p->cache, PERSIST_CACHE_SIZE, sym->data);
	lock.unlock();
	p->sym = sym;
	return true;
}	

bool PersistProperty::refresh(Symbol *var, const char *ind, const char *val)
{
	ScriptProperty *p = &typePersist;
	persist_t *pv;
	char *dp = var->data + sizeof(p);
	char databuf[128];

	if(!val)
		val = "";

	if(var->type == ScriptProperty::symINITIAL && var->size == getSize())
	{
		memcpy(&var->data, &p, sizeof(p));
		var->type = ScriptProperty::symPROPERTY;
		if(!ind && !strchr(var->id, '.'))
			ind = "sys";
		if(ind)
		{
			snprintf(databuf, sizeof(databuf), "%s.%s", ind, var->id); 
			ind = databuf;
		}
		else
			ind = var->id;	
		return remap(ind, dp, val);
	}

	if(var->type != ScriptProperty::symPROPERTY)
		return false;

        memcpy(&p, &var->data, sizeof(p)); 
	if(p != &typePersist)
		return false;

	pv = (persist_t *)dp;
	lock.readLock();
	setString(pv->cache, PERSIST_CACHE_SIZE, pv->sym->data);
	lock.unlock();
	if(!pv->cache[0])
		setString(pv->cache, PERSIST_CACHE_SIZE, val);
	return true;
}


} // end namespace

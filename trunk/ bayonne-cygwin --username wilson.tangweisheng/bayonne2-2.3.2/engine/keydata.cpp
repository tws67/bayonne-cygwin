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

DynamicKeydata *DynamicKeydata::firstConfig = NULL;

#ifdef	WIN32
#define	 CONFIG_FILES    "C:/Program Files/GNU Telephony/Bayonne Config"	
#endif

static const char *get_config(const char *path)
{
	static char buf[128];
	static bool init = false;
	static const char *env;

	if(!init)
	{
#ifdef	WIN32
		env = NULL;
#else
		env = Process::getEnv("CONFIG");
		if(!getuid())
		{
			if(!Process::getEnv("HOME"))
				Process::setEnv("HOME", "/root", true);
			if(!Process::getEnv("USER"))
				Process::setEnv("USER", "root", true);
		}
#endif
		init = true;
	}

	if(!env || !path || strnicmp(path, "/bayonne/", 9))
		return path;

	snprintf(buf, sizeof(buf), "/bayonne-%s/%s", env, path + 9);
	return buf;
}

StaticKeydata::StaticKeydata(const char *path, Keydata::Define *defs, const char *home) :
Keydata()
{
	char fpath[65];

	if(defs)
		load(defs);
	load(get_config(path));
#ifdef	WIN32
	if(!strncmp(path, "/bayonne/", 9))
	{
		char *tail;
		const char *section = strrchr(path, '/') + 1;
		snprintf(fpath, sizeof(fpath) - 5, "%s/%s",
				CONFIG_FILES, path + 9);
		tail = strrchr(fpath, '/');
		strcpy(tail, ".conf");
		loadFile(fpath, section);		
	} 
#else
	if(Bayonne::getUserdata())
	{
		if(!home && !strncmp(path, "/bayonne/", 9))
		{
			snprintf(fpath, sizeof(fpath), "~bayonne/%s", strrchr(path, '/') + 1);
			home = fpath;
		}
		else if(!stricmp(home, "none"))
			home = NULL;
		else
		{
			snprintf(fpath, sizeof(fpath), "~bayonne/%s", home);
			home = fpath;
		}	
	}
	else
		home = NULL;

	if(home)
		load(home);
#endif
}

DynamicKeydata::DynamicKeydata(const char *p, Keydata::Define *def, const char *h) :
ThreadLock()
{
	defkeys = def;
	keypath = p;
	homepath = h;
	keys = NULL;
	nextConfig = firstConfig;
	firstConfig = this;
	loadConfig();
}

void DynamicKeydata::loadConfig(void)
{
	keys = new Keydata();
	if(defkeys)
		keys->load(defkeys);
	if(keypath)
		keys->load(get_config(keypath));
#ifdef	WIN32
    if(!strncmp(path, "/bayonne/", 9))
    {
        char fpath[65];
        char *tail;
        const char *section = strrchr(path, '/') + 1;
        snprintf(fpath, sizeof(fpath) - 5, "%s/%s",
                CONFIG_FILES, path + 9);
        tail = strrchr(fpath, '/');
        strcpy(tail, ".conf");
        keys->loadFile(fpath, section);
    }
#else
	if(homepath)
		keys->load(homepath);
#endif
}

void DynamicKeydata::updateConfig(Keydata *keydata)
{
	return;
}

void DynamicKeydata::reload(void)
{
	DynamicKeydata *keydata = firstConfig;

	while(keydata)
	{
		slog.debug("reloading keydata %s", keydata->keypath);
		keydata->writeLock();
		delete keydata->keys;
		keydata->loadConfig();
		keydata->updateConfig(keydata->keys);
		keydata->unlock();
		keydata = keydata->nextConfig;
	}
}

long DynamicKeydata::getValue(const char *id)
{
	const char *cp = NULL;
	long val = 0;

	readLock();
	if(keys)
		cp = DynamicKeydata::keys->getLast(id);
	if(cp)
		val = atol(cp);
	unlock();
	return val;
}

bool DynamicKeydata::getBoolean(const char *id)
{
	bool rtn = false;
	const char *cp = NULL;

	readLock();
	if(keys)
		cp = DynamicKeydata::keys->getLast(id);
	if(cp)
		switch(*cp)
		{
		case 'N':
		case 'n':
		case '0':
		case 'f':
		case 'F':
			rtn = false;
		default:
			rtn = true;
		}
	unlock();
	return rtn;
}

bool DynamicKeydata::isKey(const char *id)
{
	const char *cp = NULL;
	bool rtn = false;

	readLock();
	if(keys)
		cp = DynamicKeydata::keys->getLast(id);

	if(cp)
		rtn = true;
	unlock();
	return rtn;
}

const char *DynamicKeydata::getString(const char *id, char *buf, size_t size)
{
	const char *cp = NULL;

	readLock();
	if(keys)
		cp = keys->getLast(id);

	if(cp)
	{
		setString(buf, size, cp);
		cp = buf;
	}
	else
		*buf = 0;

	unlock();
	return cp;
}

ReconfigKeydata::ReconfigKeydata(const char *p, Keydata::Define *def) :
StaticKeydata(p, def), DynamicKeydata(p)
{
}

const char *ReconfigKeydata::updatedString(const char *id)
{
	const char *cp = NULL;

	if(DynamicKeydata::keys)
		cp = DynamicKeydata::keys->getLast(id);

	if(!cp)
		cp = Keydata::getLast(id);

	return cp;
}

long ReconfigKeydata::updatedValue(const char *id)
{
	const char *cp = updatedString(id);
	if(cp)
		return atol(cp);
	return 0l;
}

bool ReconfigKeydata::updatedBoolean(const char *id)
{
	const char *cp = updatedString(id);
	if(!cp)
		return false;

	switch(*cp)
	{
	case 'n':
	case 'N':
	case 'f':
	case 'F':
	case '0':
		return false;
	default:
		return true;
	}
}

timeout_t ReconfigKeydata::updatedSecTimer(const char *id)
{
	const char *cp = updatedString(id);
	if(cp)
		return Audio::toTimeout(cp);
	return 0l;
}

timeout_t ReconfigKeydata::updatedMsecTimer(const char *id)
{
	const char *cp = updatedString(id);
	const char *d = cp;

	while(d && *d && isdigit(*d))
		++d;

	if(d && *d)
		return Audio::toTimeout(cp);

	if(cp)
		return atol(cp);

	return 0l;
}

timeout_t ReconfigKeydata::getSecTimer(const char *id)
{
	const char *cp = NULL;
	timeout_t val = 0;

	readLock();
	if(DynamicKeydata::keys)
		cp = DynamicKeydata::keys->getLast(id);
	if(!cp)
		cp = getLast(id);
	unlock();
	if(cp)
		val = Audio::toTimeout(cp);
	return val;
}	

timeout_t ReconfigKeydata::getMsecTimer(const char *id)
{
	const char *cp = NULL;
	timeout_t val = 0;
	const char *d;

	readLock();
	if(DynamicKeydata::keys)
		cp = DynamicKeydata::keys->getLast(id);
	if(!cp)
		cp = getLast(id);

	d = cp;
	while(d && *d && isdigit(*d))
		++d;
	if(d && *d)
		val = Audio::toTimeout(cp);
	else
		val = atol(cp);
	unlock();
	return val;
}

long ReconfigKeydata::getValue(const char *id)
{
	const char *cp = NULL;
	long val = 0;

	readLock();
	if(DynamicKeydata::keys)
		cp = DynamicKeydata::keys->getLast(id);
	if(!cp)
		cp = getLast(id);
	if(cp)
		val = atol(cp);
	unlock();
	return val;
}

bool ReconfigKeydata::getBoolean(const char *id)
{
	bool rtn = false;
	const char *cp = NULL;

	readLock();
	if(DynamicKeydata::keys)
		cp = DynamicKeydata::keys->getLast(id);
	if(!cp)
		cp = getLast(id);
	if(cp)
		switch(*cp)
		{
		case 'N':
		case 'n':
		case '0':
		case 'f':
		case 'F':
			rtn = false;
		default:
			rtn = true;
		}
	unlock();
	return rtn;
}

bool ReconfigKeydata::isKey(const char *id)
{
	const char *cp = NULL;
	bool rtn = false;

	readLock();
	if(DynamicKeydata::keys)
		cp = DynamicKeydata::keys->getLast(id);

	if(!cp)
		cp = getLast(id);

	if(cp)
		rtn = true;
	unlock();
	return rtn;
}

const char *ReconfigKeydata::getString(const char *id, char *buf, size_t size)
{
	const char *cp = NULL;

	readLock();
	if(DynamicKeydata::keys)
		cp = DynamicKeydata::keys->getLast(id);

	if(!cp)
		cp = getLast(id);

	if(cp)
	{
		setString(buf, size, cp);
		cp = buf;
	}
	else
		*buf = 0;

	unlock();
	return cp;
}

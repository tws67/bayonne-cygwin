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

BayonneAudio::BayonneAudio() :
AudioStream()
{
	extension = ".au";
	voicelib = "none/prompts";
	libext = ".au";
	encoding = mulawAudio;
	framing = 10;
	offset = NULL;
	tone = NULL;

	list = NULL;
}

const char* BayonneAudio::getVoicelib(const char *lib)
{
	char buf[MAX_PATHNAME];
	const char *f, *l;

	if(!lib)
		return NULL;

	if(strstr(lib, ".."))
		return NULL;

	if(strstr(lib, "/."))
		return NULL;

	f = strchr(lib, '/');
	l = strrchr(lib, '/');
	if(!f || f != l)
		return NULL;

	snprintf(buf, sizeof(buf), "%s/%s", path_prompts, lib);
	if(isDir(buf))
		return lib;

	if(lib[2] == '_')
	{
		vlib[0] = lib[0];
		vlib[1] = lib[1];
		lib = strchr(lib, '/');
		if(!lib)	
			lib = "/default";
		snprintf(vlib + 2, sizeof(vlib) - 2, "%s", lib);
		snprintf(buf, sizeof(buf), "%s/%s", path_prompts, vlib);
		if(isDir(buf))
			return vlib;
	}
	return NULL;
}

const char *BayonneAudio::getFilename(const char *name, bool write)
{
	const char *ext;
	const char *prefix = NULL;
	char buf[MAX_PATHNAME];
	char *cp;
	size_t max = sizeof(filename);

	if(!name)
		return NULL;
	
	if(*name == '/' || *name == '\\' || name[1] == ':')
		return NULL;

	if(strstr(name, ".."))
		return NULL;

	if(strstr(name, "/."))
		return NULL;
	
	if(name[1] == ':')
		return NULL;

	if((strchr(name, '/') || strchr(name, '\\')) && !strchr(name,':'))
	{
		ext = strrchr(name, '.');
		if(ext)
			return name;
		
		snprintf(buf, max, "%s%s", name, extension);
		goto finish;
	}

	if(!strnicmp(name, "tmp:", 4))
	{
		name += 4;
		prefix = path_tmp;
	}
	else if(!strnicmp(name, "ram:", 4))
	{
		name += 4;
		prefix = path_tmpfs;
	}

	if(!prefix)
		prefix = prefixdir;
	
	if(prefix)
	{
		ext = strrchr(name, '.');
		if(ext)
			ext = "";
		else
			ext = extension;
		
		snprintf(buf, max, "%s/%s%s", prefix, name, ext);
		goto finish; 
	}

	if(write)
		return NULL;

	prefix = strchr(name, ':');
	if(!prefix)
	{
		ext = strrchr(name, '.');
		if(ext)
			ext = "";
		else
			ext = libext;

		snprintf(filename, max, "%s/%s/%s%s", path_prompts, voicelib, name, ext);
		return filename;
	}

	ext = strrchr(name, '.');
	if(ext)
		ext = "";
	else
		ext = extension;

	snprintf(buf, max, "%s/none/%s%s", path_prompts, name, ext);
	cp = strrchr(buf, ':');
	if(cp)
		*cp = '/';

finish:
	if(buf[0] == '/' || buf[1] == ':')
		setString(filename, max, buf);
	else
		snprintf(filename, max, "%s/%s", server->getLast("prefix"), buf);
	return filename;
}

void BayonneAudio::play(const char **files, Mode m)
{	
	const char *fn = getFilename(*(files++));

	if(isOpen())
		AudioStream::close();

	if(!fn)
		return;

	list = files;
	mode = m;
	AudioStream::open(fn, m, framing);
	if(!isOpen())
	{
		list = NULL;
		return;
	}

	if(offset)
		position(offset);
}

char *BayonneAudio::getContinuation(void)
{
	if(!list)
		return NULL;

	if(!*list)
		return NULL;

	return (char *)getFilename(*(list++));
}

void BayonneAudio::record(const char *fname, Mode m, const char*annotation) 
{
	Info info;
	info.encoding = encoding;
	info.rate = getRate(encoding);
	info.annotation = (char *)annotation;

	mode = m;

	if(info.rate == rateUnknown)
		info.rate = rate8khz;

	if(isOpen())
		AudioStream::close();

	fname = getFilename(fname, true);

	if(!fname)
		return;

	switch(mode)
	{
	case modeCreate:
		remove(fname);
		create(fname, &info, false, framing);
		return;
	case modeWrite:
		open(fname, mode, framing);		
		if(isOpen() && offset)
			position(offset);
		return;
	case modeAppend:
		open(fname, modeWrite, framing);
		if(isOpen())
			setPosition();
	default:
		return;
	}
}

void BayonneAudio::cleanup(void)
{
	if(isOpen())
		close();
}


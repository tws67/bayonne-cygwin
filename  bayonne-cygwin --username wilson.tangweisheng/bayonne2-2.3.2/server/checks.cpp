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

#include "server.h"

namespace server {
using namespace ost;
using namespace std;

const char *Checks::chkSay(Line *line, ScriptImage *img)
{
	unsigned idx = 0;
	if(getMember(line))
		return "say has no members";

	if(!getOption(line, &idx))
		return "say requires at least one argument";

	if(!useKeywords(line, "=file=voice=prefix=extension"))
		return "invalid keywords for say";

	return NULL;
}		

const char *Checks::chkService(Line *line, ScriptImage *img)
{
	char buffer[80];
	unsigned idx = 0;
	const char *cp = getOption(line, &idx);

	if(getMember(line))
		return "service has no members";

	if(!cp)
		return "service requires name";

	if(getOption(line, &idx))
		return "service uses one name only";

	snprintf(buffer, sizeof(buffer), "service.%s", cp);
	line->scr.name = img->getCurrent();
	line->cmd = line->scr.name->name;
	img->setPointer(buffer, line);
	return "";
}

const char *Checks::chkParam(Line *line, ScriptImage *img)
{
	unsigned idx = 0;

	if(getMember(line))
		return "param has no members";

	if(line->argc < 1)
		return "param requires at least one keyword pair";

	if(getOption(line, &idx))
		return "param uses only keyword pairs";

	return NULL;
}

const char *Checks::chkId(Line *line, ScriptImage *img)
{
	const char *cp = getMember(line);
	unsigned idx = 0;

	if(cp)
	{
		if(!stricmp(cp, "contact"))
			goto use;
		if(!strnicmp(cp, "host", 4))
			goto use;
		if(!stricmp(cp, "uniq"))
			goto use;
		if(!strnicmp(cp, "base", 4))
			goto use;
		if(!strnicmp(cp, "dtmf", 4))
			goto use;
	
		return "id may use .contact, .base, .uniq, .dtmf, or .host only";
	}
use:
	if(!useKeywords(line, "=prefix=extension=size=id=idprefix"))
		return "id keyword invalid";

	while(NULL != (cp = getOption(line, &idx)))
	{
		if(*cp != '%' && *cp != '&')
			return "arguments must be symbol";
	}
	return NULL;
}		

const char *Checks::chkDetach(Line *line, ScriptImage *img)
{
	unsigned idx = 0;

	if(getMember(line))
		return "detach has no members";

	if(!getOption(line, &idx))
		return "detach requires at least command argument";

	return NULL;
}

const char *Checks::chkReconnect(Line *line, ScriptImage *img)
{
	unsigned idx = 0;
	if(!useKeywords(line, "=encoding=framing"))
		return "invalid keyword used for reconnect";

	if(getOption(line, &idx))
		return "no arguments for reconnect";

	return NULL;
}

const char *Checks::chkExit(Line *line, ScriptImage *img)
{
	if(getMember(line))
		return "exit has no members";

	if(line->argc > 1)
		return "exit only has one reason argument";

	return NULL;
}

const char *Checks::chkPaths(Line *line, ScriptImage *img)
{	
	unsigned idx = 0;
	const char *cp;

	if(getMember(line))
		return "paths do not use members";

	if(!strnicmp(line->cmd, "read", 4))
	{
		if(!useKeywords(line, "=prefix=voice=extension"))
			return "invalid keywords used for path";
	}	
	else if(!useKeywords(line, "=prefix=extension"))
		return "invalid keywords used for path";

	cp = getOption(line, &idx);
	if(!cp || (*cp != '%' && *cp != '&'))
		return "invalid or missing variable target";

	if(!getOption(line, &idx))
		return "missing path to evaluate";

	return NULL;
}

const char *Checks::chkTrap(Line *line, ScriptImage *img)
{
	unsigned idx = 0;

	if(getMember(line))
		return "trap uses no members";

	if(!useKeywords(line, "=id=text"))
		return "invalid keyword, use id and text";

	if(getOption(line, &idx))
		return "no options are passed for trap";	

	return NULL;
}

const char *Checks::chkChildSignal(Line *line, ScriptImage *img)
{
	unsigned idx = 0;

	if(getMember(line))
		return "signal uses no members";

	if(hasKeywords(line))
		return "signal has no keywords";

	if(getOption(line, &idx))
		return "signal has no arguments";

	return NULL;
}

const char *Checks::chkTransfer(Line *line, ScriptImage *img)
{
	unsigned idx = 0;

	if(getMember(line))
		return "transfer has no members";

	if(!useKeywords(line, "=prefix=server"))
		return "invalid keyword used";

	if(!getOption(line, &idx))
		return "transfer destination required";

	return NULL;
}

const char *Checks::chkDialer(Line *line, ScriptImage *img)
{
	unsigned idx = 0;

	if(getMember(line))
		return "dialers have no members";

	if(!useKeywords(line, "=interdigit=level=prefix"))
		return "unknown keyword for dialers";

	if(!getOption(line, &idx))
		return "no dial string argument";

	return NULL;
}

const char *Checks::chkTonegen(Line *line, ScriptImage *img)
{
	const char *cp;

	if(getMember(line))
		return "tone has no members";

	if(!useKeywords(line, "=level=freq1=freq2=frequency=duration"))
		return "unknown keyword for tone";

	cp = findKeyword(line, "frequency");
	if(!cp)
		cp = findKeyword(line, "freq1");

	if(!cp)
		return "no tone frequency specified";

	return NULL;
}	

const char *Checks::chkTone(Line *line, ScriptImage *img)
{
	unsigned idx = 0;

	if(getMember(line))
		return "tone has no members";

	if(!useKeywords(line, "=location=timeout=level=name"))
		return "unknown keyword for a tone";

	if(getOption(line, &idx))
		return "no arguments used for tones";

	return NULL;
}

const char *Checks::chkList(Line *line, ScriptImage *img)
{
	unsigned idx = 0;
	const char *cp;

	if(getMember(line))
		return "list has no members";

	if(!useKeywords(line, "=prefix=match=extension=after=prior"))
		return "invalid keyword for list";

	cp = getOption(line, &idx);
	if(!cp)
		return "list missing result argument";

	if(*cp != '&' && *cp != '%')
		return "list result argument must be a symbol";

	if(!getOption(line, &idx))
		return "list missing directory argument";

	return NULL;
}

const char *Checks::chkDir(Line *line, ScriptImage *img)
{
	unsigned count = 0;
	const char *cp;
	char path[MAX_PATHNAME];

	if(getMember(line))
		return "dir has no members";

	if(hasKeywords(line))
		return "dir has no keywords";

	if(!line->argc)
		return "dir requires at least one prefix";

	while(NULL != (cp = getOption(line, &count)))
	{
		if(!isalnum(*cp))
			return "invalid directory name used";

		if(strstr(cp, ".."))
			return "invalid directory name used";

		if(strstr(cp, "/."))
			return "invalid directory name used";

		if(strchr(cp, ':'))
			return "invalid directory name used";

		snprintf(path, sizeof(path), "%s/%s", keypaths.getLast("datafiles"), cp);
		Dir::create(path);		
	}
	return "";
}

const char *Checks::chkLang(Line *line, ScriptImage *img)
{
	unsigned idx = 0;
	const char *cp;

	if(getMember(line))
		return "lang has no members";

	if(hasKeywords(line))
		return "lang has no keywords";

	while(NULL != (cp = getOption(line, &idx)))
		BayonneTranslator::loadTranslator(cp);
		
	return "";
}

const char *Checks::chkLoad(Line *line, ScriptImage *img)
{
	unsigned count = 0;
	const char *cp;

	if(getMember(line))
		return "load has no members";

	if(hasKeywords(line))
		return "load has no keywords";

	if(!line->argc)
		return "load requires at least one module";

	while(count < line->argc)
	{ 
		cp = line->args[count++];
		if(!isalpha(*cp))
			return "an invalid module name was used";
	}	

	count = 0;
	while(count < line->argc)
	{
		cp = line->args[count++];
		loadPlugin(cp);
	}
	BayonneService::start();
	
	return "";
}	

const char *Checks::chkLibexec(Line *line, ScriptImage *img)
{
	const char *cp = getMember(line);
	unsigned idx = 0;

	if(cp && atoi(cp) < 1)
		return "libexec timeout member must be at least one second";

	if(!strnicmp(line->cmd, "exec", 4))
		goto get;

	cp = getOption(line, &idx);
	if(!cp)
		return "libexec requires command argument";

	if(*cp == '%' || *cp == '&' || *cp == '#')
		return "libexec command must be constant";

	if(strchr(cp, '/') || strchr(cp, '\\'))
		return "libexec command cannot be path";

get:
	while(NULL != (cp = getOption(line, &idx)))
	{
		if(*cp != '%' && *cp != '&' && *cp != '#')
			return "libexec arguments must be variables";
	}

	if(!useKeywords(line, "=token=results=prefix=encoding=framing=voice=extension=timeout=limit=interval"))
		return "invalid keyword used in libexec";

	cp = findKeyword(line, "results");
	if(cp && *cp != '&')
		return "results must be saved into &var reference";

	return NULL;
}

const char *Checks::chkVoicelib(Line *line, ScriptImage *img)
{
	char path[256];
	char lang[64];
	const char *cp;
	char *p;

	if(getMember(line))
		return "voice library has no members";

	if(hasKeywords(line))
		return "voice library has no keywords";

	if(line->argc != 1)
		return "voice library argument must be specified";

	cp = line->args[0];

	if(*cp == '%' || *cp == '#' || *cp == '&')
		return "voice library cannot be variable";

	if(!strchr(cp, '/'))
		return "voice library missing voice";

	if(strchr(cp, '/') != strrchr(cp, '/'))
		return "invalid voice library path";

	if(strchr(cp, '.'))
		return "invalid voice library path";		

	snprintf(path, sizeof(path), "%s/%s", 
		keypaths.getLast("prompts"), cp);

	if(!stricmp(line->cmd, "voicelib"))
	{
		setString((char *)line->cmd, 9, cp);
		p = (char *)strchr(line->cmd, '/');
		if(p)
			*p = 0;
	}

	if(isDir(path))
		return NULL;

	if(cp[2] != '_')
		return "invalid voice library";

	lang[0] = cp[0];
	lang[1] = cp[1];
	cp = strchr(cp, '/');
	snprintf(lang + 2, sizeof(lang) - 2, "%s", cp);
	snprintf(path, sizeof(path), "%s/%s",
		keypaths.getLast("prompts"), lang);

	if(!isDir(path))
		return "unknown voice library";
	
	strcpy((char *)line->args[0], lang);
	
	return NULL;
}

const char *Checks::chkErase(Line *line, ScriptImage *img)
{
	unsigned idx = 0;

	if(getMember(line))
		return "erase has no members";

	if(!useKeywords(line, "=extension=prefix"))
		return "invalid keyword for erase";

	if(!getOption(line, &idx))
		return "erase requires file argument";

	if(getOption(line, &idx))
		return "erase uses only one file argument";
	
	return NULL;
}	

const char *Checks::chkInfo(Line *line, ScriptImage *img)
{
	unsigned idx = 0;
	const char *cp;

	if(getMember(line))
		return "erase has no members";

	if(!useKeywords(line, 
"=extension=prefix=date=time=note=size=type=coding=rate=bitrate=count"))
		return "invalid keyword for info";

	cp = findKeyword(line, "date");
	if(cp && *cp != '%' && *cp != '&')
		return "info date must be symbol target";

	cp = findKeyword(line, "time");
	if(cp && *cp != '%' && *cp != '&')
		return "info time must be symbol target";

	cp = findKeyword(line, "size");
	if(cp && *cp != '%' && *cp != '&')
		return "info size must be symbol target";

	cp = findKeyword(line, "coding");
	if(cp && *cp != '%' && *cp != '&')
		return "info coding must be symbol target";

	cp = findKeyword(line, "type");
        if(cp && *cp != '%' && *cp != '&')
                return "info type must be symbol target";

        cp = findKeyword(line, "rate");
        if(cp && *cp != '%' && *cp != '&')
                return "info rate must be symbol target";  

        cp = findKeyword(line, "bitrate");
        if(cp && *cp != '%' && *cp != '&')
                return "info bitrate must be symbol target";  

        cp = findKeyword(line, "count");
        if(cp && *cp != '%' && *cp != '&')
                return "info count must be symbol target";  

        cp = findKeyword(line, "note");
        if(cp && *cp != '%' && *cp != '&')
                return "info note must be symbol target";

	if(!getOption(line, &idx))
		return "info requires file argument";

	if(getOption(line, &idx))
		return "info uses only one file argument";
	
	return NULL;
}	

const char *Checks::chkWrite(Line *line, ScriptImage *img)
{
	unsigned idx = 0;

	if(getMember(line))
		return "write has no members";

	if(!useKeywords(line, "=extension=prefix"))
		return "invalid keywords for write";

	if(!getOption(line, &idx))
		return "file missing for write";

	return NULL;
}

const char *Checks::chkMove(Line *line, ScriptImage *img)
{
	unsigned idx = 0;

	if(getMember(line))
		return "command has no members";

	if(!useKeywords(line, "=extension=prefix"))
		return "invalid keyword for this command";

	if(!getOption(line, &idx))
		return "source argument missing";

	if(!getOption(line, &idx))
		return "target argument missing";

	if(getOption(line, &idx))
		return "only two arguments used";
	
	return NULL;
}	

const char *Checks::chkBuild(Line *line, ScriptImage *img)
{
        unsigned idx = 0;

	if(getMember(line))
		return "member not used in build";

	if(!useKeywords(line, "=extension=encoding=framing=prefix=voice=note"))
		return "invalid keyword for build";


	if(!getOption(line, &idx))
		return "build destination missing";

	if(!getOption(line, &idx))
		return "build requires at least one file source";	

	return NULL;
}

const char *Checks::chkRecord(Line *line, ScriptImage *img)
{
	unsigned idx = 0;
	const char *cp;

	cp = getMember(line);
	if(cp && stricmp(cp, "vox"))
		return "use no member or .vox only";

	if(!useKeywords(line, "=extension=encoding=prefix=framing=silence=intersilence=note=position=exit=timeout=menu"))
		return "invalid keyword used";

	if(!getOption(line, &idx))
		return "requires one file argument";

	if(getOption(line, &idx) && getOption(line, &idx))
		return "uses only up to two file argument";

	return NULL;
}

const char *Checks::chkReplay(Line *line, ScriptImage *img)
{
        unsigned idx = 0;

        if(getMember(line))
		return "no member used in replay";

        if(!useKeywords(line, "=extension=encoding=prefix=framing=exit=menu=position"))
                return "invalid keyword used for replay";

        if(!getOption(line, &idx))
                return "replay requires one file argument";

        if(getOption(line, &idx))
                return "replay uses only one file argument";

        return NULL;
}

const char *Checks::chkAppend(Line *line, ScriptImage *img)
{
	unsigned idx = 0;
	const char *cp;

	cp = getMember(line);
	if(cp && stricmp(cp, "vox"))
		return "use no member or .vox only";

	if(!useKeywords(line, "=extension=encoding=prefix=framing=silence=intersilence=exit=timeout"))
		return "invalid keyword used";

	if(!getOption(line, &idx))
		return "requires one file argument";

	if(getOption(line, &idx) && getOption(line, &idx))
		return "uses up to two file argument";

	return NULL;
}

const char *Checks::chkPathname(Line *line, ScriptImage *img)
{
	unsigned idx = 0;
	
	if(!useKeywords(line, "=extension=voice=encoding=prefix"))
		return "invalid keyword used";

	if(!getOption(line, &idx))
		return "requires at least one prompt argument";

	return NULL;
}		

const char *Checks::chkCollect(Line *line, ScriptImage *img)
{
	unsigned idx = 0;
	const char *cp;

	if(!useKeywords(line, "=count=timeout=lastdigit=interdigit=ignore=exit=format"))
		return "invalid keyword for collect";

	cp = getOption(line, &idx);

	if(cp && *cp != '%' && *cp != '&')
		return "collect argument must be variable";

	return NULL;
}

const char *Checks::chkInput(Line *line, ScriptImage *img)
{
	unsigned idx = 0;
	const char *cp;

	if(getMember(line))
		return "input has no members";

	if(!useKeywords(line, "=count=timeout=interdigit=lastdigit=exit=format"))
		return "invalid keyword for input, use timeout, interdigit, count, format, and exit";

	cp = getOption(line, &idx);	
	if(!cp)
		return "input missing variable argument";

	if(*cp != '%' && *cp != '&')
		return "invalid argument for variable name";

	return NULL;
}

const char *Checks::chkKeyinput(Line *line, ScriptImage *img)
{
	const char *cp;
	unsigned idx = 0;

	if(getMember(line))
		return "inkey has no members";

	if(!useKeywords(line, "=timeout=menu"))
		return "invalid keyword for keyinput, use timeout and menu";

	cp = getOption(line, &idx);
	if(!cp)
		return NULL;

	if(*cp != '%' && *cp != '&')
		return "optional keyinput argument must be variable"; 

	return NULL;
}

const char *Checks::chkCleardigits(Line *line, ScriptImage *img)
{
	if(getMember(line))
		return "cleardigits does not use members";

	if(line->argc > 1)
		return "cleardigits uses one optional timeout";

	return NULL;
}

const char *Checks::chkRoute(Line *line, ScriptImage *img)
{
	if(hasKeywords(line))
		return "no keywords used for digits routing";

	if(!line->argc)
		return "route requires at least one route value";
	
	return NULL;
}

const char *Checks::chkSleep(Line *line, ScriptImage *img)
{
	if(getMember(line))
		return "member not used for this command";

	if(!line->argc || line->argc > 1)
		return "requires one timeout and uses no keywords";

	return NULL;
}

} // end namespace

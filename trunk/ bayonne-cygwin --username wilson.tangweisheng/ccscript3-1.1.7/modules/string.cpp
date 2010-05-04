// Copyright (C) 1999-2005 Open Source Telecom Corporation.
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
//
// As a special exception, you may use this file as part of a free software
// library without restriction.  Specifically, if other files instantiate
// templates or use macros or inline functions from this file, or you compile
// this file and link it with other files to produce an executable, this
// file does not by itself cause the resulting executable to be covered by
// the GNU General Public License.  This exception does not however
// invalidate any other reasons why the executable file might be covered by
// the GNU General Public License.
//
// This exception applies only to the code released under the name GNU
// ccScript.  If you copy code from other releases into a copy of GNU
// ccScript, as the General Public License permits, the exception does
// not apply to the code that you add in this way.  To avoid misleading
// anyone as to the status of such modified files, you must delete
// this exception notice from them.
//
// If you write modifications of your own for GNU ccScript, it is your choice
// whether to permit this exception to apply to your modifications.
// If you do not wish that, delete this exception notice.
//

#include "script3.h"
#include <stdlib.h>

namespace ccscript3Extension {

using namespace std;
using namespace ost;

class StringMethods : public ScriptMethods
{
public:
	bool scrUpper(void);
	bool scrLower(void);
	bool scrStrip(void);
	bool scrResize(void);
	bool scrChop(void);
	bool scrTrim(void);
};

class StringChecks : public ScriptChecks
{
public:
	const char *chkValid(Line *line, ScriptImage *img);
	const char *chkResize(Line *line, ScriptImage *img);
	const char *chkStrip(Line *line, ScriptImage *img);
	const char *chkChop(Line *line, ScriptImage *img);
};

static Script::Define runtime[] = {
	{"resize", true, (Script::Method)&StringMethods::scrResize,
		(Script::Check)&StringChecks::chkResize},
	{"strip", true, (Script::Method)&StringMethods::scrStrip,
		(Script::Check)&StringChecks::chkStrip},
	{"lower", true, (Script::Method)&StringMethods::scrLower,
		(Script::Check)&StringChecks::chkValid},
	{"upper", true, (Script::Method)&StringMethods::scrUpper,
		(Script::Check)&StringChecks::chkValid},
	{"chop", true, (Script::Method)&StringMethods::scrChop,
		(Script::Check)&StringChecks::chkChop},
	{"trim", true, (Script::Method)&StringMethods::scrTrim,
		(Script::Check)&StringChecks::chkChop},
	{NULL, false, NULL, NULL}};

static ScriptBinder bindString(runtime);

const char *StringChecks::chkChop(Line *line, ScriptImage *img)
{
	if(getMember(line))
		return "member not used for chop";

	if(!useKeywords(line, "=count=token"))
		return "invalid keyword for chop";

	return chkAllVars(line, img);
}

const char *StringChecks::chkStrip(Line *line, ScriptImage *img)
{
	if(getMember(line))
		return "member not used for strip";

	if(!useKeywords(line, "=prefix=suffix=allow=deny"))
		return "invalid keyword for strip";

	return chkAllVars(line, img);
}

const char *StringChecks::chkResize(Line *line, ScriptImage *img)
{
	if(getMember(line))
		return "member not used for resize";

	if(!useKeywords(line, "=size=offset"))
		return "invalid keyword for resize";

	return chkAllVars(line, img);
}

const char *StringChecks::chkValid(Line *line, ScriptImage *img)
{
	if(getMember(line))
		return "member not used for case";

	if(hasKeywords(line))
		return "keywords not used for case";

	return chkAllVars(line, img);
}

bool StringMethods::scrResize(void)
{
	const char *cp;
	Symbol *sym;
	char *f, *t;
	unsigned len;
	int max = 0;
	int pos = 0;

	cp = getKeyword("size");
	if(cp)
		max = atoi(cp);

	cp = getKeyword("offset");
	if(cp)
		pos = atoi(cp);

	if(max < 0) {
		pos += max;
		max = -max;
	}

	while(NULL != (cp = getOption(NULL))) {
		sym = mapSymbol(cp, 0);
		if(!sym || sym->type != symNORMAL)
			continue;

		len = (unsigned)strlen(sym->data);
		if(!len)
			continue;

		if((int)len < max)
			max = len;

		if(!max)
			max = len;

		if(!len)
			continue;

		t = sym->data;
		if(pos >= (int)len) {
			sym->data[0] = 0;
			continue;
		}

		if(pos < 0 && -pos >= (int)len) {
			sym->data[0] = 0;
			continue;
		}

		if(pos < 0)
			f = t + len + pos;
		else
			f = t + pos;

		if(f == t) {
			sym->data[max] = 0;
			continue;
		}

		while(max-- && *f)
			*(t++) = *(f++);
		*t = 0;
	}
	advance();
	return true;
}

bool StringMethods::scrChop(void)
{
	const char *cp;
	Symbol *sym;
	char *f, *t;
	unsigned count = 1;
	char token = ',';

	cp = getKeyword("token");
	if(cp)
		token = *cp;

	cp = getKeyword("count");
	if(cp)
		count = atoi(cp);

	while(NULL != (cp = getOption(NULL))) {
		sym = mapSymbol(cp, 0);
		if(!sym || sym->type != symNORMAL)
			continue;

		f = t = sym->data;

		if(!count)
			f = strrchr(f, token);
		else while(count-- > 0)
		{
			f = strchr(f, token);
			if(f)
				++f;
			else
				break;
		}

		if(!f)
			continue;

		while(*f)
			*(t++) = *(f++);
		*t = 0;
	}
	advance();
	return true;
}

bool StringMethods::scrTrim(void)
{
	const char *cp;
	Symbol *sym;
	char *f;
	unsigned count = 1;
	char token = ',';

	cp = getKeyword("token");
	if(cp)
		token = *cp;

	cp = getKeyword("count");
	if(cp)
		count = atoi(cp);

	while(NULL != (cp = getOption(NULL))) {
		sym = mapSymbol(cp, 0);
		if(!sym || sym->type != symNORMAL)
			continue;

		if(!count) {
			f = strchr(sym->data, token);
			if(f)
				*f = 0;
		}
		else while(count-- > 0)
		{
			f = strrchr(sym->data, token);
			if(f)
				*f = 0;
			else
				break;
		}
	}
	advance();
	return true;
}

bool StringMethods::scrStrip(void)
{
	const char *prefix = getKeyword("prefix");
	const char *suffix = getKeyword("suffix");
	const char *allow = getKeyword("allow");
	const char *deny = getKeyword("deny");

	const char *cp;
	Symbol *sym;
	char *f, *t;
	unsigned len, ol = 0;

	while(NULL != (cp = getOption(NULL))) {
		sym = mapSymbol(cp, 0);
		if(!sym || sym->type != symNORMAL)
			continue;

		len = (unsigned)strlen(sym->data);
		if(!len)
			continue;

		f = sym->data;
		t = sym->data;

		while(*f) {
			if(allow && strchr(allow, *f))
				*(t++) = *(f++);
			else if(deny && strchr(deny, *f))
				++f;
			else if(allow)
				++f;
			else
				*(t++) = *(f++);
		}
		*t = 0;
		f = t = sym->data;
		len = (unsigned)strlen(f);

		if(suffix)
			ol = (unsigned)strlen(suffix);

		if(suffix && ol <= len && !strnicmp(suffix, f + len - ol, ol)) {
			len -= ol;
			f[len] = 0;
			if(!len)
				continue;
		}

		if(prefix)
			ol = (unsigned)strlen(prefix);

		if(prefix && !strnicmp(prefix, f, ol))
			f += ol;

		if(f == t)
			continue;

		while(*f)
			*(t++) = *(f++);

		*t = 0;
	}
	advance();
	return true;
}



bool StringMethods::scrUpper(void)
{
	const char *cp;
	Symbol *sym;
	char *cc;

	while(NULL != (cp = getOption(NULL))) {
		sym = mapSymbol(cp, 0);
		if(!sym)
			continue;

		cc = NULL;
		switch(sym->type) {
		case symPROPERTY:
			cc = sym->data + sizeof(ScriptProperty *);
			break;
		case symNORMAL:
			cc = sym->data;
		default:
			break;
		}

		while(cc && *cc) {
			*cc = toupper(*cc);
			++cc;
		}
	}
	advance();
	return true;
}

bool StringMethods::scrLower(void)
{
	const char *cp;
	Symbol *sym;
	char *cc;

	while(NULL != (cp = getOption(NULL))) {
		sym = mapSymbol(cp, 0);
		if(!sym)
			continue;

		cc = NULL;
		switch(sym->type) {
		case symPROPERTY:
			cc = sym->data + sizeof(ScriptProperty *);
			break;
		case symNORMAL:
			cc = sym->data;
		default:
			break;
		}

		while(cc && *cc) {
			*cc = tolower(*cc);
			++cc;
		}
	}
	advance();
	return true;
}

}; // namespace


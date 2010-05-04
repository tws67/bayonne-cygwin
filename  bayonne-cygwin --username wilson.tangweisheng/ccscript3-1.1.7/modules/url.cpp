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

class URLMethods : public ScriptMethods
{
public:
	bool urlProtocol(void);
	bool urlExtension(void);
	bool urlDecode(void);
	bool urlEncode(void);
	bool urlEscape(void);
	bool scrURL(void);
	bool scrQuery(void);
};

class URLChecks : public ScriptChecks
{
public:
	const char *chkURL(Line *line, ScriptImage *img);
	const char *chkURLBuild(Line *line, ScriptImage *img);
	const char *chkURLQuery(Line *line, ScriptImage *img);
	const char *chkDecode(Line *line, ScriptImage *img);
};

static Script::Define runtime[] = {
	{"urlprotocol", true, (Script::Method)&URLMethods::urlProtocol,
		(Script::Check)&URLChecks::chkURL},
	{"urlextension", true, (Script::Method)&URLMethods::urlExtension,
		(Script::Check)&URLChecks::chkURL},
	{"urldecode", true, (Script::Method)&URLMethods::urlDecode,
		(Script::Check)&URLChecks::chkDecode},
	{"urlescape", true, (Script::Method)&URLMethods::urlEscape,
		(Script::Check)&URLChecks::chkDecode},
	{"urlencode", true, (Script::Method)&URLMethods::urlEncode,
		(Script::Check)&URLChecks::chkDecode},
	{"url", true, (Script::Method)&URLMethods::scrURL,
		(Script::Check)&URLChecks::chkURLBuild},
	{"urlquery", true, (Script::Method)&URLMethods::scrQuery,
		(Script::Check)&URLChecks::chkURLQuery},
	{NULL, false, NULL, NULL}};

static ScriptBinder bindString(runtime);

const char *URLChecks::chkURLBuild(Line *line, ScriptImage *img)
{
	unsigned idx = 0;
	const char *cp;

	if(getMember(line))
		return "url builder has no members";

	cp = getOption(line, &idx);
	if(!cp)
		return "targat variable missing";

	if(*cp != '&' && *cp != '%')
		return "target must be variable";

	return NULL;
}

const char *URLChecks::chkURLQuery(Line *line, ScriptImage *img)
{
	unsigned idx = 0;
	const char *cp;

	if(getMember(line))
		return "url builder has no members";

	cp = getOption(line, &idx);
	if(!cp)
		return "targat variable missing";

	if(*cp != '&' && *cp != '%')
		return "target must be variable";

	return chkAllVars(line, img);
}


const char *URLChecks::chkDecode(Line *line, ScriptImage *img)
{
	if(getMember(line))
		return "member not used for urldecode";

	if(hasKeywords(line))
		return "invalid keyword for urldecode";

	if(line->argc < 1)
		return "urldecode uses variable arguments";

	return chkAllVars(line, img);
}

const char *URLChecks::chkURL(Line *line, ScriptImage *img)
{
	if(getMember(line))
		return "member not used for url";

	if(hasKeywords(line))
		return "invalid keyword for url";

	if(line->argc < 2)
		return "requires url and at least one check";

	return NULL;
}

bool URLMethods::scrURL(void)
{
	Line *line = getLine();
	unsigned idx = 0;
	Symbol *sym;
	const char *opt = getOption(NULL);
	unsigned len = 0;
	bool slash = false, dslash = false;
	char *dp;
	char token = '?';
	const char *cp;

	sym = mapSymbol(opt, 256);
	if(!sym) {
		error("target-missing");
		return true;
	}

	if(sym->type != symNORMAL && sym->type != symINITIAL) {
		error("target-invalid");
		return true;
	}

	dp = sym->data;

	while((NULL != (opt = getValue(NULL))) && len < (unsigned)(sym->size - 3)) {
		while(*opt && len < (unsigned)(sym->size - 3)) {
			if(isalnum(*opt) || strchr(":@;.#~_", *opt)) {
				slash = false;
				*(dp++) = *(opt++);
				++len;
				continue;
			}
			if(*opt == '/') {
				if(!slash || (slash && !dslash)) {
					if(slash)
						dslash = true;
					slash = true;
					*(dp++) = *(opt++);
					++len;
				}
				else
					++opt;
				continue;
			}
			slash = false;
			*(dp++) = '%';
			snprintf(dp, 3, "%02x", *(opt++));
			dp += 2;
			len += 3;
		}
	}

	while(idx < line->argc && len < (unsigned)(sym->size - 3)) {
		cp = line->args[idx++];
		if(*cp != '=')
			continue;
		++cp;
		*(dp++) = token;
		token = '&';
		++len;
		while(*cp && len < (unsigned)(sym->size - 3)) {
			if(isalnum(*cp)) {
				*(dp++) = tolower(*cp);
				++cp;
				++len;
			}
			else if(*cp == '.' || *cp == '_')
			{
				*(dp++) = '_';
				++cp;
				++len;
			}
			else
				++cp;
		}
		++len;
		*(dp++) = '=';
		cp = getContent(line->args[idx++]);
		if(!cp)
			cp = "";
		while(*cp && len < (unsigned)(sym->size - 3)) {
			if(isalnum(*cp)) {
				*(dp++) = *(cp++);
				++len;
				continue;
			}
			*(dp++) = '%';
			snprintf(dp, 3, "%02x", *(cp++));
			dp += 2;
			len += 3;
		}
	}
	*dp = 0;
	sym->type = symNORMAL;
	advance();
	return true;
}

bool URLMethods::scrQuery(void)
{
	Line *line = getLine();
	unsigned idx = 0;
	Symbol *sym;
	const char *opt = getOption(NULL);
	unsigned len;
	char *dp;
	char token = '?';
	const char *cp;

	sym = mapSymbol(opt, 256);
	if(!sym) {
		error("target-missing");
		return true;
	}

	if(sym->type != symNORMAL && sym->type != symINITIAL) {
		error("target-invalid");
		return true;
	}

	len = strlen(sym->data);
	dp = sym->data + len;
	if(strchr(sym->data, '?'))
		token = '&';

	while((NULL != (opt = getOption(NULL))) && len < (unsigned)(sym->size - 3)) {
		if(len) {
			*(dp++) = token;
			token = '&';
			++len;
		}
		cp = opt;
		while(*opt && len < (unsigned)(sym->size - 3)) {
			if(isalnum(*opt)) {
				*(dp++) = tolower(*opt);
				++opt;
				++len;
			}
			else if(*opt == '.' || *opt == '_')
			{
				*(dp++) = '_';
				++opt;
				++len;
			}
			else
				++opt;
		}
		*(dp++) = '=';
		++len;
		cp = getContent(cp);
		if(!cp)
			cp = "";

		while(*cp && len < (unsigned)(sym->size - 3)) {
			if(isalnum(*cp)) {
				*(dp++) = *(cp++);
				++len;
				continue;
			}
			*(dp++) = '%';
			snprintf(dp, 3, "%02x", *(cp++));
			dp += 2;
			len += 3;
		}
	}

	while(idx < line->argc && len < (unsigned)(sym->size - 3)) {
		cp = line->args[idx++];
		if(*cp != '=')
			continue;
		++cp;
		*(dp++) = token;
		token = '&';
		++len;
		while(*cp && len < (unsigned)(sym->size - 3)) {
			if(isalnum(*cp)) {
				*(dp++) = tolower(*cp);
				++cp;
				++len;
			}
			else if(*cp == '.' || *cp == '_')
			{
				*(dp++) = '_';
				++cp;
				++len;
			}
			else
				++cp;
		}
		++len;
		*(dp++) = '=';
		cp = getContent(line->args[idx++]);
		if(!cp)
			cp = "";
		while(*cp && len < (unsigned)(sym->size - 3)) {
			if(isalnum(*cp)) {
				*(dp++) = *(cp++);
				++len;
				continue;
			}
			*(dp++) = '%';
			snprintf(dp, 3, "%02x", *(cp++));
			dp += 2;
			len += 3;
		}
	}
	*dp = 0;
	sym->type = symNORMAL;
	advance();
	return true;
}


bool URLMethods::urlDecode(void)
{
	const char *opt;
	Symbol *sym;
	char *dp, *sp;
	char hex[3];
	char c;

	while(NULL != (opt = getOption(NULL))) {
		sym = mapSymbol(++opt, 0);
		if(!sym)
			continue;

		if(sym->type != symNORMAL && sym->type != symINITIAL)
			continue;

		dp = sp = sym->data;
		while(*sp) {
			if(*sp == '+') {
				*(dp++) = ' ';
				++sp;
			}
			else if(*sp == '%')
			{
				hex[0] = *(++sp);
				hex[1] = *(++sp);
				hex[2] = 0;
				++sp;
				c = (char)strtol(hex, NULL, 16);
				*(dp++) = c;
			}
			else
				*(dp++) = *(sp++);
		}
		*dp = 0;
		sym->type = symNORMAL;
	}
	advance();
	return true;
}

bool URLMethods::urlEscape(void)
{
	const char *opt;
	Symbol *sym;
	char *dp, *sp, *ns;
	unsigned len;

	while(NULL != (opt = getOption(NULL))) {
		sym = mapSymbol(++opt, 0);
		if(!sym)
			continue;

		if(sym->type != symNORMAL && sym->type != symINITIAL)
			continue;

		sp = ns = newString(sym->data);
		dp = sym->data;
		len = 0;
		while(*sp && len <= (unsigned)(sym->size - 3)) {
			if(isalnum(*sp)) {
				++len;
				*(dp++) = *(sp++);
				continue;
			}
			*(dp++) = '%';
			snprintf(dp, 3, "%02x", *(sp++));
			dp += 2;
			len += 3;
		}
		*dp = 0;
		sym->type = symNORMAL;
		delString(ns);
	}
	advance();
	return true;
}


bool URLMethods::urlEncode(void)
{
	const char *opt;
	Symbol *sym;
	char *dp, *sp, *ns;
	unsigned len;
	bool query = false;
	bool equal = false;

	while(NULL != (opt = getOption(NULL))) {
		sym = mapSymbol(++opt, 0);
		if(!sym)
			continue;

		if(sym->type != symNORMAL && sym->type != symINITIAL)
			continue;

		sp = ns = newString(sym->data);
		dp = sym->data;
		len = 0;
		while(*sp && len <= (unsigned)(sym->size - 3)) {
			if(isalnum(*sp) && query && !equal) {
				++len;
				*(dp++) = tolower(*sp);
				++sp;
				continue;
			}
			if((*sp == '.' || *sp == '_')&& query && !equal) {
				*(dp++) = '_';
				++sp;
				++len;
				continue;
			}
			if(*sp == '&' && query) {
				*(dp++) = *(sp++);
				equal = false;
				++len;
				continue;
			}
			if(isalnum(*sp)) {
				++len;
				*(dp++) = *(sp++);
				continue;
			}
			if(*sp == '?' && !query) {
				query = true;
				*(dp++) = *(sp++);
				++len;
				continue;
			}
			if(*sp == '=' && query) {
				equal = true;
				*(dp++) = *(sp++);
				++len;
				continue;
			}
			if(strchr(":@#/;.", *sp) && !query) {
				*(dp++) = *(sp++);
				++len;
				continue;
			}
			*(dp++) = '%';
			snprintf(dp, 3, "%02x", *(sp++));
			dp += 2;
			len += 3;
		}
		*dp = 0;
		sym->type = symNORMAL;
		delString(ns);
	}
	advance();
	return true;
}


bool URLMethods::urlExtension(void)
{
	const char *opt = getValue(NULL);
	const char *ext = opt;
	unsigned elen = 0;
	char ebuf[65];

	if(!opt) {
missing:
		if(!scriptEvent("url:extension-missing"))
			error("extension-missing");
		return true;
	}

	ext = strchr(opt, '?');
	if(!ext)
		ext = opt + strlen(opt);

	while(--ext >= opt && ++elen < 32) {
		if(*ext == '.')
			break;
	}
	if(ext == opt || *ext != '.' || elen < 2)
		goto missing;

	setString(ebuf, elen, ++ext);

	while(NULL != (opt = getValue(NULL))) {
		if(*opt == '.')
			++opt;

		if(!stricmp(opt, ebuf)) {
			snprintf(ebuf, sizeof(ebuf), "url:extension-%s", opt);
 			if(!scriptEvent(ebuf))
				advance();
			return true;
		}
	}
	if(!scriptEvent("url:extension-invalid"))
		error("extension-invalid");
	return true;
}

bool URLMethods::urlProtocol(void)
{
	char pname[65];
	const char *opt = getValue(NULL);
	char *cp = pname, count = 0;

	if(!opt) {
missing:
		if(!scriptEvent("url:protocol-missing"))
			error("protocol-missing");
		return true;
	}

	while(isalpha(*opt) && ++count < 32)
		*(cp++) = *(opt++);

	if(*opt != ':')
		goto missing;

	*cp = 0;

	while(NULL != (opt = getValue(NULL))) {
		if(!stricmp(opt, pname)) {
			snprintf(pname, sizeof(pname), "url:protocol-%s", opt);
			if(!scriptEvent(pname))
				advance();
			return true;
		}
	}
	if(!scriptEvent("url:protocol-invalid"))
		error("protocol-invalid");
	return true;
}

}; // namespace


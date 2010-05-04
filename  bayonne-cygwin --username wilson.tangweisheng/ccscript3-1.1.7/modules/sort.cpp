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

class SortMethods : public ScriptMethods
{
public:
	bool scrSort(void);
};

class SortChecks : public ScriptChecks
{
public:
	const char *chkSort(Line *line, ScriptImage *img);
};

static Script::Define runtime[] = {
	{"sort", false, (Script::Method)&SortMethods::scrSort,
		(Script::Check)&SortChecks::chkSort},
	{"revsort", false, (Script::Method)&SortMethods::scrSort,
		(Script::Check)&SortChecks::chkSort},
	{NULL, false, NULL, NULL}};

static ScriptBinder bindSort(runtime);
static Mutex _lock;
static bool _rev = false;
static char _pack = 0;
static unsigned _offset = 0;

extern "C" {

static int compare(const void *x, const void *y)
{
	unsigned offset = _offset;
	char *s1 = (char *)x;
	char *s2 = (char *)y;

	while(offset && s1 && s2) {
		s1 = strchr(s1, _pack);
		if(s1)
			++s1;
		s2 = strchr(s2, _pack);
		if(s2)
			++s2;

		--offset;
	}

	if(!s1)
		s1 = "";
	if(!s2)
		s2 = "";

	if(_rev)
		return stricmp(s2, s1);

	return stricmp(s1, s2);
	}

};

const char *SortChecks::chkSort(Line *line, ScriptImage *img)
{
	unsigned idx = 0;
	const char *cp;

	if(getMember(line))
		return "member not used for sort";

	if(!useKeywords(line, "=field=token=offset"))
		return "invalid keyword used for sort";

	cp = getOption(line, &idx);
	if(!cp)
		return "variable missing to sort";

	if(*cp != '%' && *cp != '&')
		return "invalid sort argument";

	if(getOption(line, &idx))
		return "sort only one variable at a time";

	return NULL;
}



bool SortMethods::scrSort(void)
{
	Line *line = getLine();
	bool rev = false;
	const char *opt;
	char pack = getPackToken();
	unsigned offset = 0;
	Symbol *sym = mapSymbol(getOption(NULL));
	size_t count = 0;
	Array *a = (Array *)&sym->data;
	char *base = sym->data + sizeof(Array);
	size_t size = a->rec + 1;

	if(!sym) {
		error("sort-missing-symbol");
		return true;
	}

	opt = getKeyword("offset");
	if(!opt)
		opt = getKeyword("field");
	if(opt)
		offset = atoi(opt);

	opt = getKeyword("token");
	if(opt && *opt)
		pack = *opt;

	if(*(line->cmd) == 'r')
		rev = true;

	switch(sym->type) {
	case symARRAY:
		count = a->tail;
		break;
	default:
		error("sort-invalid-type");
		return true;
	}

	_lock.enter();
	_rev = rev;
	_pack = pack;
	_offset = offset;

	qsort((void *)base, count, size, &compare);

	_lock.leave();

	skip();
	return true;
}

};


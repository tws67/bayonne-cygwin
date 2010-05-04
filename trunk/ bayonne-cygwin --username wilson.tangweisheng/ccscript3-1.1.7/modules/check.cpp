// Copyright (C) 2005 David Sugar, Tycho Softworks.
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

class CheckRuntime : public ScriptBinder
{
private:
	static bool testTrue(ScriptInterp *interp, const char *v);
	static bool testFalse(ScriptInterp *interp, const char *v);
	static bool testDigits(ScriptInterp *interp, const char *v);
	static bool testNumber(ScriptInterp *interp, const char *v);
	static bool testCCN(ScriptInterp *interp, const char *v);
	static bool testSSN(ScriptInterp *interp, const char *v);

public:
	CheckRuntime();
};

static CheckRuntime check;

CheckRuntime::CheckRuntime() : ScriptBinder()
{
	ScriptInterp::addConditional("number", &testNumber);
	ScriptInterp::addConditional("digits", &testDigits);
	ScriptInterp::addConditional("true", &testTrue);
	ScriptInterp::addConditional("false", &testFalse);
	ScriptInterp::addConditional("ccn", &testCCN);
	ScriptInterp::addConditional("ssn", &testSSN);
}

bool CheckRuntime::testSSN(ScriptInterp *interp, const char *v)
{
	char num[10];
	unsigned len = 0;

	if(!v)
		return  false;

	while(len < 9 && *v) {
		if(isdigit(*v))
			num[len++] = *v;
		++v;
	}

	while(*v) {
		if(isdigit(*v))
			return false;
		++v;
	}

	if(len < 9)
		return false;

	if(!strncmp(num + 5, "0000", 4))
		return false;

	if(!strncmp(num + 3, "00", 2))
		return false;

	num[3] = 0;
	if(atoi(num) > 770)
		return false;

	return true;
}

bool CheckRuntime::testCCN(ScriptInterp *interp, const char *v)
{
	bool d = true;
	unsigned len = 0;
	unsigned tot = 0;
	char dig;
	const char *vp = v;

	if(!v)
		return false;

	while(*v) {
		if(isdigit(*v))
			++len;
		++v;
	}

	if(len % 2)
		d = false;

	if(!len)
		return false;

	v = vp;
	while(*v) {
		if(!isdigit(*v)) {
			++v;
			continue;
		}
		dig = *v - '0';

		++v;

		if(d) {
			dig *= 2;
			d = false;
		}
		else
			d = true;

		if(dig > 9)
			dig -= 9;

		tot += dig;
	}

	if(tot % 10)
		return false;

	return true;
}

bool CheckRuntime::testFalse(ScriptInterp *interp, const char *v)
{
	if(!v)
		return false;

	return !testTrue(interp, v);
}

bool CheckRuntime::testTrue(ScriptInterp *interp, const char *v)
{
	if(!v)
		return false;

	if(atoi(v) > 0)
		return true;

	switch(*v) {
	case 't':
	case 'T':
	case 'y':
	case 'Y':
		return true;
	}

	return false;
}

bool CheckRuntime::testDigits(ScriptInterp *interp, const char *v)
{
	if(!v)
		return false;

	while(*v) {
		if(*v < '0' || *v > '9')
			return false;
		++v;
	}
	return true;
}

bool CheckRuntime::testNumber(ScriptInterp *interp, const char *v)
{
	bool dot = false;

	if(!v)
		return false;

	if(*v == '-')
		++v;

	while(*v) {
		if(*v == '.' && !dot)
			dot = true;
		else if(*v < '0' || *v > '9')
			return false;
		++v;
	}
	return true;
}

};


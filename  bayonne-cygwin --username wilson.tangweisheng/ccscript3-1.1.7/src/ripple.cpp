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

#include "engine.h"

using namespace std;
using namespace ost;

ScriptRipple::ScriptRipple() :
ScriptCommand()
{
	static Script::Define interp[] = {
		{"use", true, (Method)&ScriptMethods::scrNop,
			(Check)&ScriptChecks::chkUse},
		{"if", true, (Method)&ScriptMethods::scrIf,
			(Check)&ScriptChecks::chkConditional},
		{"error", true, (Method)&ScriptMethods::scrError,
			(Check)&ScriptChecks::chkError},
		{"exit", true, (Method)&ScriptMethods::scrExit,
			(Check)&ScriptChecks::chkOnlyCommand},
		{"counter", true, (Method)&ScriptMethods::scrCounter,
			(Check)&ScriptChecks::chkCounter},
		{"expr", true, (Method)&ScriptMethods::scrExpr,
			(Check)&ScriptChecks::chkExpr},
		{"string", true, (Method)&ScriptMethods::scrVar,
			(Check)&ScriptChecks::chkString},
		{"char", true, (Method)&ScriptMethods::scrVar,
			(Check)&ScriptChecks::chkVar},
		{"label", true, (Method)&ScriptMethods::scrNop,
			(Check)&ScriptChecks::chkLabel},
		{"goto", true, (Method)&ScriptMethods::scrGoto,
			(Check)&ScriptChecks::chkGoto},
		{"restart", true, (Method)&ScriptMethods::scrRestart,
			(Check)&ScriptChecks::chkRestart},
		{"sequence", true, (Method)&ScriptMethods::scrSequence,
			(Check)&ScriptChecks::chkSequence},
		{"const", true, (Method)&ScriptMethods::scrConst,
			(Check)&ScriptChecks::chkConst},
		{"stack", true, (Method)&ScriptMethods::scrArray,
			(Check)&ScriptChecks::chkArray},
		{"fifo", true, (Method)&ScriptMethods::scrArray,
			(Check)&ScriptChecks::chkArray},
		{"lifo", true, (Method)&ScriptMethods::scrArray,
			(Check)&ScriptChecks::chkArray},
		{"init", true, (Method)&ScriptMethods::scrSet,
			(Check)&ScriptChecks::chkSet},
		{"set", true, (Method)&ScriptMethods::scrSet,
			(Check)&ScriptChecks::chkSet},
		{"add", true, (Method)&ScriptMethods::scrSet,
			(Check)&ScriptChecks::chkCat},
		{"cat", true, (Method)&ScriptMethods::scrSet,
			(Check)&ScriptChecks::chkCat},
		{"clear", true, (Method)&ScriptMethods::scrClear,
			(Check)&ScriptChecks::chkClear},
		{NULL, false, NULL, NULL}};

	// add special default exit and error traps

	trap("exit");
	trap("error", false);
	load(interp);

#ifdef	SQL_MODULE
	aliasModule("sql", SQL_MODULE);
#endif
	aliasModule("auth", "userauth");
	if(!runtime)
		runtime = this;
	ripple = true;
}


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

#ifdef	HAVE_STRCASECMP
#ifndef	stristr
#define	stristr(x, y) strcasestr(x, y)
#endif
#endif

ScriptRuntime::ScriptRuntime() :
ScriptCommand()
{
	static Script::Define interp[] = {
		{"use", true, (Method)&ScriptMethods::scrNop,
			(Check)&ScriptChecks::chkUse},
		{"unpack", true, (Method)&ScriptMethods::scrPack,
			(Check)&ScriptChecks::chkPack},
		{"pack", true, (Method)&ScriptMethods::scrPack,
			(Check)&ScriptChecks::chkPack},
		{"construct", true, (Method)&ScriptMethods::scrConstruct,
			(Check)&ScriptChecks::chkConstruct},
		{"deconstruct", true, (Method)&ScriptMethods::scrDeconstruct,
			(Check)&ScriptChecks::chkConstruct},
		{"slog", false, (Method)&ScriptMethods::scrSlog,
			(Check)&ScriptChecks::chkSlog},
		{"syslog", false, (Method)&ScriptMethods::scrSlog,
			(Check)&ScriptChecks::chkSlog},
		{"first", false, (Method)&ScriptMethods::scrRemove,
			(Check)&ScriptChecks::chkCat},
		{"last", false, (Method)&ScriptMethods::scrRemove,
			(Check)&ScriptChecks::chkCat},
		{"remove", false, (Method)&ScriptMethods::scrRemove,
			(Check)&ScriptChecks::chkRemove},
		{"if", false, (Method)&ScriptMethods::scrIf,
			(Check)&ScriptChecks::chkConditional},
		{"then", false, (Method)&ScriptMethods::scrThen,
			(Check)&ScriptChecks::chkOnlyCommand},
		{"else", false, (Method)&ScriptMethods::scrElse,
			(Check)&ScriptChecks::chkOnlyCommand},
		{"endif", false, (Method)&ScriptMethods::scrEndif,
			(Check)&ScriptChecks::chkOnlyCommand},
		{"begin", false, (Method)&ScriptMethods::scrBegin,
			(Check)&ScriptChecks::chkOnlyCommand},
		{"end", false, (Method)&ScriptMethods::scrEnd,
			(Check)&ScriptChecks::chkOnlyCommand},
		{"case", false, (Method)&ScriptMethods::scrCase,
			(Check)&ScriptChecks::chkConditional},
		{"otherwise", false, (Method)&ScriptMethods::scrCase,
			(Check)&ScriptChecks::chkOnlyCommand},
		{"endcase", false, (Method)&ScriptMethods::scrEndcase,
			(Check)&ScriptChecks::chkOnlyCommand},
		{"offset", false, (Method)&ScriptMethods::scrOffset,
			(Check)&ScriptChecks::chkRepeat},
		{"repeat", false, (Method)&ScriptMethods::scrRepeat,
			(Check)&ScriptChecks::chkRepeat},
		{"for", false, (Method)&ScriptMethods::scrFor,
			(Check)&ScriptChecks::chkFor},
		{"foreach", false, (Method)&ScriptMethods::scrForeach,
			(Check)&ScriptChecks::chkForeach},
		{"do", false, (Method)&ScriptMethods::scrDo,
			(Check)&ScriptChecks::chkConditional},
		{"loop", false, (Method)&ScriptMethods::scrLoop,
			(Check)&ScriptChecks::chkConditional},
		{"continue", false, (Method)&ScriptMethods::scrContinue,
			(Check)&ScriptChecks::chkConditional},
		{"break", false, (Method)&ScriptMethods::scrBreak,
			(Check)&ScriptChecks::chkConditional},
		{"error", false, (Method)&ScriptMethods::scrError,
			(Check)&ScriptChecks::chkError},
		{"decimal", true, (Method)&ScriptMethods::scrDecimal,
			(Check)&ScriptChecks::chkDecimal},
		{"exit", false, (Method)&ScriptMethods::scrExit,
			(Check)&ScriptChecks::chkOnlyCommand},
		{"counter", true, (Method)&ScriptMethods::scrCounter,
			(Check)&ScriptChecks::chkCounter},
		{"timer", true, (Method)&ScriptMethods::scrTimer,
			(Check)&ScriptChecks::chkTimer},
		{"index", false, (Method)&ScriptMethods::scrIndex,
			(Check)&ScriptChecks::chkIndex},
		{"num", true, (Method)&ScriptMethods::scrNumber,
			(Check)&ScriptChecks::chkNumber},
		{"int", true, (Method)&ScriptMethods::scrNumber,
			(Check)&ScriptChecks::chkClear},
		{"integer", true, (Method)&ScriptMethods::scrNumber,
			(Check)&ScriptChecks::chkClear},
		{"number", true, (Method)&ScriptMethods::scrNumber,
			(Check)&ScriptChecks::chkNumber},
		{"expr", false, (Method)&ScriptMethods::scrExpr,
			(Check)&ScriptChecks::chkExpr},
		{"var", true, (Method)&ScriptMethods::scrVar,
			(Check)&ScriptChecks::chkVar},
		{"type", true, (Method)&ScriptMethods::scrType,
			(Check)&ScriptChecks::chkVarType},
		{"vartype", true, (Method)&ScriptMethods::scrType,
			(Check)&ScriptChecks::chkVarType},
		{"define", true, (Method)&ScriptMethods::scrDefine,
			(Check)&ScriptChecks::chkDefine},
		{"symbols", true, (Method)&ScriptMethods::scrDefine,
			(Check)&ScriptChecks::chkDefine},
  		{"string", true, (Method)&ScriptMethods::scrVar,
			(Check)&ScriptChecks::chkString},
		{"str", true, (Method)&ScriptMethods::scrVar,
			(Check)&ScriptChecks::chkString},
		{"char", true, (Method)&ScriptMethods::scrVar,
			(Check)&ScriptChecks::chkChar},
		{"bool", true, (Method)&ScriptMethods::scrVar,
			(Check)&ScriptChecks::chkChar},
		{"signal", false, (Method)&ScriptMethods::scrSignal,
			(Check)&ScriptChecks::chkSignal},
		{"throw", false, (Method)&ScriptMethods::scrThrow,
			(Check)&ScriptChecks::chkThrow},
		{"goto", false, (Method)&ScriptMethods::scrGoto,
			(Check)&ScriptChecks::chkGoto},
		{"call", false, (Method)&ScriptMethods::scrCall,
			(Check)&ScriptChecks::chkCall},
		{"gosub", false, (Method)&ScriptMethods::scrCall,
			(Check)&ScriptChecks::chkCall},
		{"source", false, (Method)&ScriptMethods::scrCall,
			(Check)&ScriptChecks::chkLabel},
		{"return", false, (Method)&ScriptMethods::scrReturn,
			(Check)&ScriptChecks::chkReturn},
		{"restart", false, (Method)&ScriptMethods::scrRestart,
			(Check)&ScriptChecks::chkRestart},
		{"ref", true,  (Method)&ScriptMethods::scrRef,
			(Check)&ScriptChecks::chkRefArgs},
		{"fconst", true, (Method)&ScriptMethods::scrInit,
			(Check)&ScriptChecks::chkIgnore},
		{"options", false, (Method)&ScriptMethods::scrNop,
			(Check)&ScriptChecks::chkIgnore},
		{"keywords", false, (Method)&ScriptMethods::scrNop,
			(Check)&ScriptChecks::chkKeywords},
		{"keyword", false, (Method)&ScriptMethods::scrNop,
			(Check)&ScriptChecks::chkKeywords},
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
		{"array", true, (Method)&ScriptMethods::scrArray,
			(Check)&ScriptChecks::chkArray},
		{"init", true, (Method)&ScriptMethods::scrSet,
			(Check)&ScriptChecks::chkSet},
		{"pset", true, (Method)&ScriptMethods::scrSet,
			(Check)&ScriptChecks::chkSet},
		{"set", true, (Method)&ScriptMethods::scrSet,
			(Check)&ScriptChecks::chkSet},
		{"add", true, (Method)&ScriptMethods::scrSet,
			(Check)&ScriptChecks::chkCat},
		{"cat", true, (Method)&ScriptMethods::scrSet,
			(Check)&ScriptChecks::chkCat},
		{"clear", false, (Method)&ScriptMethods::scrClear,
			(Check)&ScriptChecks::chkClear},
		{"lock", false, (Method)&ScriptMethods::scrLock,
			(Check)&ScriptChecks::chkLock},
		{"session", false, (Method)&ScriptMethods::scrSession,
			(Check)&ScriptChecks::chkSession},
		{NULL, false, NULL, NULL}};

	// add special default exit and error traps

	trap("exit");
	trap("error", false);
	load(interp);

	// add common module aliasing
#ifdef	SQL_MODULE
	aliasModule("sql", SQL_MODULE);
#endif
	aliasModule("auth", "userauth");
	if(!runtime)
		runtime = this;
}


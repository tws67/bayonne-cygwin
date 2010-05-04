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

#include "module.h"
#include <sql.h>

using namespace ccscript3Database;
using namespace std;
using namespace ost;

static Script::Define runtime[] = {
	{"sql", false, (Script::Method)&SQLMethods::scrSQL,
		(Script::Check)&SQLChecks::chkSQL},
	{NULL, false, NULL, NULL}};

void SQLBinder::down(void)
{
	if(hODBC) {
		SQLFreeEnv(hODBC);
		hODBC = NULL;
	}
}

SQLBinder::SQLBinder() :
ScriptBinder("odbc")
{
	long err = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hODBC);

	if(err != SQL_SUCCESS && err != SQL_SUCCESS_WITH_INFO) {
		slog.critical("odbc: cannot allocate");
		return;
	}

	err = SQLSetEnvAttr(hODBC, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
	if(err != SQL_SUCCESS && err != SQL_SUCCESS_WITH_INFO) {
		slog.critical("odbc: wrong version");
		SQLFreeHandle(SQL_HANDLE_ENV, hODBC);
		hODBC = NULL;
		return;
	}
	bind(runtime);
}

void SQLBinder::attach(ScriptInterp *interp)
{
	interp->setConst("sql.driver", "odbc");
	interp->setSymbol("sql.database", "none", 48);
	interp->setSymbol("sql.error", "none", 32);
	interp->setNumber("sql.rows", "0");
}

void SQLBinder::detach(ScriptInterp *interp)
{
	const char *name = interp->getSymbol("sql.database");
	ScriptImage *img = interp->getImage();
	SQLDatabase *sql;
	char dbname[65];

	if(!name || !*name)
		return;

	if(!stricmp(name, "none"))
		return;

	snprintf(dbname, sizeof(dbname), "odbc.%s", name);
	sql = (SQLDatabase *)img->getPointer(dbname);
	if(!sql)
		return;
}

const char *SQLBinder::use(Line *line, ScriptImage *img)
{
	SQLDatabase *sql;
	const char *dsn;

	char name[128];
	char dbname[128];

	if(img->isRipple())
		return "odbc not supported under ripple";

	if(ScriptCommand::getCount(line) > 0)
		return "command uses keywords, not arguments";

	dsn = ScriptCommand::findKeyword(line, "database");
	if(!dsn)
		return "database name missing";

	setString(dbname, sizeof(dbname), "odbc.");
	addString(dbname, sizeof(dbname), dsn);
	setString(name, sizeof(name), "odbc.");
	addString(name, sizeof(name), img->getCurrent()->filename);
/*	cp = strchr(name, ':');
	if(cp)
		*cp = 0;
*/
	if(img->getLast(name))
		return "already registered for script file";

	img->setValue(name, dbname);
	sql = (SQLDatabase *)img->getPointer(dbname);
	if(!sql) {
		sql = new SQLDatabase(img, line);
		img->setPointer(dbname, (void *)sql);
	}
	return "";
}

static SQLBinder sql;
SQLHENV ccscript3Database::hODBC = NULL;

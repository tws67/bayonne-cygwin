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

using namespace ccscript3Database;
using namespace std;
using namespace ost;

Mutex SQLDatabase::locker;

SQLDatabase::SQLDatabase(ScriptImage *img, Line *line) :
ScriptObject(img)
{
	char buffer[256];
	long err;

	ScriptCommand *cmd = img->getCommand();

	name = ScriptCommand::findKeyword(line, "database");
	user = ScriptCommand::findKeyword(line, "user");

	instance = img->getInstance();

	if(!user)
		user = ScriptCommand::findKeyword(line, "username");
#ifndef	WIN32
	if(!user)
		user = Script::access_user;
#endif
	pass = ScriptCommand::findKeyword(line, "password");
	if(!pass)
		pass = ScriptCommand::findKeyword(line, "pass");
#ifndef	WIN32
	if(!pass)
		pass = Script::access_pass;
#endif

	hdbc = NULL;
	count = 0;
	errmsg[0] = 0;

	err = SQLAllocHandle(SQL_HANDLE_DBC, hODBC, &hdbc);
	if(err != SQL_SUCCESS && err != SQL_SUCCESS_WITH_INFO) {
		slog.error("odbc: database '%s' failed to create handle; serial=%ld",
			name, instance);
		hdbc = NULL;
		return;
	}

	SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER *)5, 0);
	err = SQLConnect(hdbc, (SQLCHAR *)name, SQL_NTS, (SQLCHAR *)user, SQL_NTS, (SQLCHAR *)pass, SQL_NTS);
	if(err != SQL_SUCCESS && err != SQL_SUCCESS_WITH_INFO) {
			SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
		hdbc = NULL;
		snprintf(buffer, sizeof(buffer), "%s: connection failed", name);
		cmd->errlog("error", buffer);
		slog.debug("odbc: database '%s' failed connect; serial=%ld",
			name, instance);
		return;
	}

	snprintf(buffer, sizeof(buffer), "%s: database connected", name);
	slog.debug("odbc: database '%s' active; serial=%ld",
		name, instance);
	cmd->errlog("notice", buffer);
}

SQLDatabase::~SQLDatabase()
{
	if(hdbc) {
 	        SQLDisconnect(hdbc);
			SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
		hdbc = NULL;
	}
	slog.debug("odbc: released database '%s'; serial=%ld",
		name, instance);
}

bool SQLDatabase::isConnected(void)
{
	if(hdbc)
		return true;

	return false;
}


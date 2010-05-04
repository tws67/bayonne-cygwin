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

#include "../../src/engine.h"

#ifdef  HAVE_ODBC_SQL_H
#include <odbc/sql.h>
#include <odbc/sqlext.h>
#include <odbc/sqltypes.h>
#else
//#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>
#endif

namespace ccscript3Database {

using namespace std;
using namespace ost;

extern SQLHENV hODBC;

class SQLDatabase : public ScriptObject
{
private:
	friend class SQLThread;

	static Mutex locker;
	const char *name;
	const char *user;
	const char *pass;
	SQLHDBC hdbc;
	SQLCHAR errmsg[128];
	unsigned count;
	unsigned long instance;

public:
	SQLDatabase(ScriptImage *img, Line *dsn);
	~SQLDatabase();

	bool isConnected(void);

	inline const char *getName(void)
		{return name;}
};

class SQLMethods : public ScriptMethods
{
public:
	bool scrSQL(void);
};

class SQLChecks : public ScriptChecks
{
public:
	const char *chkSQL(Line *line, ScriptImage *img);
};

class SQLBinder : public ScriptBinder
{
private:
	const char *use(Line *line, ScriptImage *img);

	void attach(ScriptInterp *interp);
	void detach(ScriptInterp *interp);
	void down(void);

public:
	SQLBinder();
};

class SQLThread : public ScriptThread
{
private:
	friend class SQLDatabase;

	SQLHSTMT hstmt;
	SQLDatabase *current;
	Symbol *sym;

	void run(void);

public:
	SQLThread(ScriptInterp *interp, SQLDatabase *current, Symbol *results);
	~SQLThread();
};

}


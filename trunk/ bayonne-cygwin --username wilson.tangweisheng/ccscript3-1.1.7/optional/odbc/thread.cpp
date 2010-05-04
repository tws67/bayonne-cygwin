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

SQLThread::SQLThread(ScriptInterp *interp, SQLDatabase *c, Symbol *r) :
ScriptThread(interp, 0, 64000l + 32000l * sizeof(void *))
{
	current = c;
	sym = r;
	hstmt = NULL;
}

SQLThread::~SQLThread()
{
	terminate();
	if(hstmt)
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
}

void SQLThread::run(void)
{
	const char *opt;
	SQLINTEGER err;
	SQLCHAR stat[10];
	SQLCHAR errmsg[128];
	SQLSMALLINT mlen, col, cols;
#if ODBCVER >= 0x0300 && !defined(__ppc__)
	SQLINTEGER rowcnt;
#else
	long int rowcnt;
#endif
	char buf[1024];
	char nbuf[12];
	unsigned row = 0, len = 0;

	if(!current->isConnected()) {
		lock();
		interp->setSymbol("sql.error", "sql-connect-failed");
		release();
		exit("sql-connect-failed");
	}
	lock();
	interp->setSymbol("sql.database", current->getName());
	interp->setSymbol("sql.error", "none");
	interp->setSymbol("sql.rows", "0");
	release();

	err = SQLAllocHandle(SQL_HANDLE_STMT, current->hdbc, &hstmt);
	if(err != SQL_SUCCESS && err != SQL_SUCCESS_WITH_INFO) {
		hstmt = NULL;
		SQLGetDiagRec(SQL_HANDLE_DBC, current->hdbc, 1,
			stat, &err, errmsg, sizeof(errmsg), &mlen);
		lock();
		interp->setSymbol("sql.error", (char *)errmsg + 4);
		release();
		slog.error("odbc: %s: %s", current->name, errmsg);
		exit("sql-prep-failed");
	}

	buf[0] = 0;
	while(NULL != (opt = interp->getValue(NULL))) {
		Thread::yield();
		addString(buf, sizeof(buf), opt);
	}

	Thread::yield();

	err = SQLExecDirect(hstmt, (SQLCHAR *)buf, SQL_NTS);
	if(err != SQL_SUCCESS && err != SQL_SUCCESS_WITH_INFO) {
		strcpy((char *)errmsg, "sql.");
		SQLGetDiagRec(SQL_HANDLE_DBC, current->hdbc, 1,
			stat, &err, errmsg, sizeof(errmsg) / sizeof(SQLCHAR), &mlen);
		lock();
		interp->setSymbol("sql.error", (char *)errmsg);
		release();
		slog.error("odbc: %s: %s", current->name, errmsg);
		exit("sql-query-failed");
	}

	Thread::yield();

	SQLRowCount(hstmt, &rowcnt);
	snprintf(buf, sizeof(buf), "%d", (int)rowcnt);
	lock();
	interp->setNumber("sql.rows", buf);
	release();

	if(!sym)
		exit(NULL);

	while(row < count(sym)) {
		err = SQLFetch(hstmt);
	        if(err != SQL_SUCCESS && err != SQL_SUCCESS_WITH_INFO)
			break;

		Thread::yield();
		snprintf(nbuf, sizeof(nbuf), "%d", ++row);
		SQLNumResultCols(hstmt, &cols);
		if(cols < 1)
			break;

		col = 0;
		len = 0;
		while(col < cols && len < sizeof(buf) - 3) {
			if(len)
				buf[len++] = ',';
			buf[len++] = '\'';
			SQLGetData(hstmt, ++col, SQL_C_CHAR, buf + len, (SQLINTEGER)(256 - len), &rowcnt);
 			len = (unsigned)strlen(buf);
			buf[len++] = '\'';
			Thread::yield();
		}
		commit(sym, buf);
		Thread::yield();
	}

	// post actual row count
	snprintf(nbuf, sizeof(nbuf), "%d", row);
	lock();
	interp->setNumber("sql.rows", nbuf);
	release();

	exit(NULL);
}

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
#include <cc++/slog.h>
#include <cstdio>
#include <stdlib.h>

namespace ccscript3Extension {

using namespace std;
using namespace ost;

class CSVArrayThread : public ScriptThread
{
private:
	Symbol *sym;
	char csvbuf[1024];
#ifdef	WIN32
	HANDLE fh;
#else
	int fd;
#endif

	void run(void);
public:
	CSVArrayThread(ScriptInterp *interp, Symbol *s);
	~CSVArrayThread();
};

class CSVThread : public ScriptThread
{
private:
	char csvbuf[1024];
#ifdef	WIN32
	HANDLE fh;
#else
	int fd;
#endif

	void run(void);

public:
	CSVThread(ScriptInterp *interp);
	~CSVThread();
};

class CSVChecks : public ScriptChecks
{
public:
	const char *chkCSV(Line *line, ScriptImage *img);
	const char *chkCSVArray(Line *line, ScriptImage *img);
	const char *chkCSVExpand(Line *line, ScriptImage *img);
};

class CSVMethods : public ScriptMethods
{
public:
	bool scrCSV(void);
	bool scrCSVArray(void);
	bool scrCSVExpand(void);
};

CSVThread::CSVThread(ScriptInterp *interp) :
ScriptThread(interp, 0)
{
#ifdef	WIN32
	fh = INVALID_HANDLE_VALUE;
#else
	fd = -1;
#endif
}

CSVArrayThread::CSVArrayThread(ScriptInterp *interp, Symbol *s) :
ScriptThread(interp, 0)
{
#ifdef	WIN32
	fh = INVALID_HANDLE_VALUE;
#else
	fd = -1;
#endif
	sym = s;
}

CSVThread::~CSVThread()
{
	terminate();
#ifdef	WIN32
	if(fh != INVALID_HANDLE_VALUE)
		CloseHandle(fh);
#else
	if(fd > -1)
		::close(fd);
#endif
}

CSVArrayThread::~CSVArrayThread()
{
	terminate();
#ifdef	WIN32
	if(fh != INVALID_HANDLE_VALUE)
		CloseHandle(fh);
#else
	if(fd > -1)
		::close(fd);
#endif
}

static Script::Define runtime[] = {
	{"csv", false, (Script::Method)&CSVMethods::scrCSV,
		(Script::Check)&CSVChecks::chkCSV},
	{"csvarray", false, (Script::Method)&CSVMethods::scrCSVArray,
		(Script::Check)&CSVChecks::chkCSVArray},
	{"csvexpand", false, (Script::Method)&CSVMethods::scrCSVExpand,
		(Script::Check)&CSVChecks::chkCSVExpand},
	{NULL, false, NULL, NULL}};

static ScriptBinder bindCSV(runtime);

void CSVArrayThread::run(void)
{
	unsigned len = 0;
	const char *v;
	char path[128];
	unsigned idx = 0;
	Array *a = (Array *)&sym->data;

	for(;;)
	{
		v = NULL;
		switch(sym->type) {
		case symARRAY:
			if(idx < a->count && idx < a->tail)
				v = sym->data + sizeof(Array) + idx * (a->rec + 1);
			++idx;
			break;
		case symSTACK:
			if(a->tail != a->head) {
				idx = a->tail;
				if(a->tail == 0)
					a->tail = a->count - 1;
				else
					--a->tail;
				v = sym->data + sizeof(Array) + idx * (a->rec + 1);
			}
			break;
		case symFIFO:
			if(a->head != a->tail) {
				v = sym->data + a->head * (a->rec + 1) + sizeof(Array);
				if(++a->head >= a->count)
					a->head = 0;
			}
		default:
			break;
		}
		if(!v)
			break;

		if(len)
			snprintf(csvbuf + len, sizeof(csvbuf) - len - 3,
				",\'%s\'", v);
		else
			snprintf(csvbuf, sizeof(csvbuf) - 3, "\'%s\'", v);
		len = (unsigned)strlen(csvbuf);
	}
	if(!len)
		exit("csv-empty");

	csvbuf[len++] = '\r';
	csvbuf[len++] = '\n';
	snprintf(path, sizeof(path), "%s/%s.csv",
	Script::log_prefix, interp->getMember());
#ifdef	WIN32
	DWORD wc;
	fh = CreateFile(path, GENERIC_WRITE, 0,
		NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(fh == INVALID_HANDLE_VALUE)
			exit("csv-failed");
	SetFilePointer(fh, 0, NULL, FILE_END);
	WriteFile(fh, csvbuf, len, &wc, NULL);
#else
	fd = ::open(path, O_APPEND | O_CREAT | O_WRONLY, 0660);
	if(fd < 0)
	exit("csv-failed");
	if(::write(fd, csvbuf, len) < (int)len)
	exit("csv-failed");
#endif
	exit(NULL);
}

void CSVThread::run(void)
{
	unsigned len = 0;
	const char *v;
	char path[128];

	lock();
	while(NULL != (v = interp->getValue(NULL))) {
		if(len)
			snprintf(csvbuf + len, sizeof(csvbuf) - len - 3,
				",\'%s\'", v);
		else
			snprintf(csvbuf, sizeof(csvbuf) - 3, "\'%s\'", v);
		len = (unsigned)strlen(csvbuf);
		release();
		Thread::yield();
		lock();
	}
	release();
	if(!len)
		exit("csv-empty");
	csvbuf[len++] = '\r';
	csvbuf[len++] = '\n';
	snprintf(path, sizeof(path), "%s/%s.csv", Script::log_prefix, interp->getMember());
#ifdef	WIN32
	DWORD wc;
	fh = CreateFile(path, GENERIC_WRITE, 0,
		NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(fh == INVALID_HANDLE_VALUE)
			exit("csv-failed");
	SetFilePointer(fh, 0, NULL, FILE_END);
	WriteFile(fh, csvbuf, len, &wc, NULL);
#else
	fd = ::open(path, O_APPEND | O_CREAT | O_WRONLY, 0660);
	if(fd < 0)
		exit("csv-failed");
	if(::write(fd, csvbuf, len) < (int)len)
		exit("csv-failed");
#endif
	exit(NULL);
}

bool CSVMethods::scrCSV(void)
{
	release();
	new CSVThread(dynamic_cast<ScriptInterp*>(this));
	return false;
}

bool CSVMethods::scrCSVExpand(void)
{
	const char *cp = getOption();
	Symbol *sym = mapSymbol(cp, 0);
	Symbol *dest;
	unsigned size = symsize;
	unsigned offset = 0;
	unsigned len = 0, idx = 0;
	Array *a;

	cp = getKeyword("size");
	if(cp)
		size = atoi(cp);

	cp = getKeyword("offset");
	if(cp)
		offset = atoi(cp);

	if(!sym) {
		error("no-array");
		return true;
	}

	switch(sym->type) {
		case symARRAY:
		case symFIFO:
		case symSTACK:
		break;
		default:
		error("invalid-array");
		return true;
	}

	cp = getOption();
	dest = mapSymbol(cp, size);
	if(!dest) {
		error("destination-missing");
		return true;
	}

	if(dest->type != symINITIAL && dest->type != symNORMAL) {
		error("destination-invalid");
		return true;
	}

	dest->type = symNORMAL;
	dest->data[0] = 0;
	a = (Array *)&sym->data;
	for(;;)
	{
		cp = NULL;
		switch(sym->type) {
		case symARRAY:
			if(idx < a->count && idx < a->tail)
						cp = sym->data + sizeof(Array) + idx * (a->rec + 1);
			++idx;
			break;
				case symSTACK:
			if(a->tail != a->head) {
				idx = a->tail;
				if(a->tail == 0)
					a->tail = a->count - 1;
				else
					--a->tail;
						cp = sym->data + sizeof(Array) + idx * (a->rec + 1);
			}
			break;
				case symFIFO:
			if(a->head != a->tail) {
						cp = sym->data + a->head * (a->rec + 1) + sizeof(Array);
				if(++a->head >= a->count)
					a->head = 0;
			}
		default:
			break;
		}
		if(!cp)
			break;
		if(offset) {
			--offset;
			continue;
		}
		if(len)
			snprintf(dest->data + len, dest->size - len,
				",\'%s\'", cp);
		else
			snprintf(dest->data, dest->size,
				"\'%s\'", cp);
		len = (unsigned)strlen(dest->data);
	}
	advance();
	return true;
}

bool CSVMethods::scrCSVArray(void)
{
	const char *cp = getOption();
	Symbol *sym = mapSymbol(cp, 0);
	if(!sym) {
		error("no-array");
		return true;
	}
	switch(sym->type) {
	case symARRAY:
	case symFIFO:
	case symSTACK:
		break;
	default:
		error("invalid-array");
		return true;
	}
	release();
	new CSVArrayThread(dynamic_cast<ScriptInterp *>(this), sym);
	return false;
}

const char *CSVChecks::chkCSV(Line *line, ScriptImage *img)
{
	if(!getMember(line))
		return "csv requires .filename";

	if(hasKeywords(line))
		return "csv uses no keywords";

	if(!line->argc)
		return "csv requires arguments";

	return NULL;
}

const char *CSVChecks::chkCSVExpand(Line *line, ScriptImage *img)
{
	const char *cp;
	unsigned idx = 0;
	if(getMember(line))
		return "csvexpand has no members";

	if(!useKeywords(line, "=size=offset"))
		return "invalid keyword for csvexpand";

	cp = getOption(line, &idx);
	if(!cp)
		return "csvexpand requires two arguments";

	if(*cp != '%' && *cp != '&')
		return "csvexpand requires variable argument";

	cp = getOption(line, &idx);
	if(!cp)
		return "csvexpand requires destination variable";

	if(*cp != '%' && *cp != '&')
		return "csvexpand requires variable destination";

	cp = getOption(line, &idx);
	if(cp)
		return "too many arguments for csvexpand";

	return NULL;
}

const char *CSVChecks::chkCSVArray(Line *line, ScriptImage *img)
{
	const char *cp;
	if(!getMember(line))
		return "csvarray requires .filename";

	if(hasKeywords(line))
		return "csvarray uses no keywords";

	if(line->argc != 1)
		return "csvarray requires one arguments";

	cp = line->args[0];
	if(*cp != '%' && *cp != '&')
		return "csvarray requires variable argument";

	return NULL;
}

};


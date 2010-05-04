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
#include <cstdio>
#include <crypt.h>
#include <stdlib.h>


#ifdef	WIN32
#define	EXT_AUTH	".pwd"
#define	PRE_AUTH	"\\Script Authorization\\"
#define	PRE_MAKE	"\\Script Authorization"
#else
#define	EXT_AUTH	""
#define PRE_AUTH	"/.access/"
#define	PRE_MAKE	"/.access"
#endif

namespace ccscript3Extension {

using namespace std;
using namespace ost;

static Mutex authlock;

/* From local_passwd.c (C) Regents of Univ. of California blah blah */
static unsigned char itoa64[] =         /* 0 ... 63 => ascii - 64 */
	"./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

static void to64(char *s, long v, int n)
{
	while (--n >= 0) {
		*s++ = itoa64[v&0x3f];
		v >>= 6;
	}
}

class LookupThread : public ScriptThread
{
private:
	FILE *fp;
	char path[128];
	const char *key;
	const char *prefix;
	Symbol *sym;

	void run(void);
public:
	LookupThread(ScriptInterp *interp, const char *group, Symbol *sym);
	~LookupThread();
};

class AuthorizeThread : public ScriptThread
{
private:
	FILE *fp;
	char path[128];
	const char *user;
	const char *pass;
	const char *prefix;
	Symbol *sym;

	void run(void);

public:
	AuthorizeThread(ScriptInterp *interp, const char *group, Symbol *sym);
	~AuthorizeThread();
};

class PasswordThread : public ScriptThread
{
private:
	FILE *fp;
	char path[128];
	const char *user;
	const char *pass;
	const char *save;

	void run(void);

public:
	PasswordThread(ScriptInterp *interp, const char *group);
	~PasswordThread();
};

class IdentifyThread : public ScriptThread
{
private:
	FILE *fp;
	const char *user;
	char path[128];
	const char *prefix;

	void run(void);

public:
	IdentifyThread(ScriptInterp *interp, const char *group);
	~IdentifyThread();
};

class UserauthChecks : public ScriptChecks
{
public:
	const char *chkAuthorize(Line *line, ScriptImage *img);
	const char *chkIdentify(Line *line, ScriptImage *img);
	const char *chkLogout(Line *line, ScriptImage *img);
	const char *chkLookup(Line *line, ScriptImage *img);
	const char *chkPassword(Line *line, ScriptImage *img);
};

class UserauthMethods : public ScriptMethods
{
public:
	bool scrAuthorize(void);
	bool scrIdentify(void);
	bool scrLogout(void);
	bool scrLookup(void);
	bool scrPassword(void);
};

class UserauthBinder : public ScriptBinder
{
public:
	UserauthBinder(Script::Define *run);
};

static Script::Define runtime[] = {
	{"authorize", false, (Script::Method)&UserauthMethods::scrAuthorize,
		(Script::Check)&UserauthChecks::chkAuthorize},
	{"lookup", false, (Script::Method)&UserauthMethods::scrLookup,
		(Script::Check)&UserauthChecks::chkLookup},
	{"identify", false, (Script::Method)&UserauthMethods::scrIdentify,
		(Script::Check)&UserauthChecks::chkIdentify},
	{"password", false, (Script::Method)&UserauthMethods::scrPassword,
			(Script::Check)&UserauthChecks::chkPassword},
	{"logout", true, (Script::Method)&UserauthMethods::scrLogout,
		(Script::Check)&UserauthChecks::chkLogout},
	{NULL, false, NULL, NULL}};

UserauthBinder::UserauthBinder(Script::Define *run) :
ScriptBinder(run)
{
	char path[128];

	if(*Script::var_prefix != '.') {
		snprintf(path, sizeof(path), "%s" PRE_MAKE,
			Script::var_prefix);
		Dir::create(path);
	}
}

static UserauthBinder bindUserauth(runtime);

LookupThread::LookupThread(ScriptInterp *interp, const char *group, Symbol *s) :
ScriptThread(interp, 0)
{
	key = interp->getValue(NULL);
	prefix = interp->getKeyword("prefix");
	sym = s;
	if(*Script::var_prefix == '.')
		setString(path, sizeof(path), ".access");
	else
		snprintf(path, sizeof(path), "%s" PRE_AUTH "%s" EXT_AUTH,
			Script::var_prefix, group);

	fp = NULL;
}

AuthorizeThread::AuthorizeThread(ScriptInterp *interp, const char *group, Symbol *s) :
ScriptThread(interp, 0)
{
	user = interp->getValue(NULL);
	pass = interp->getValue(NULL);
	prefix = interp->getKeyword("prefix");
	sym = s;

	if(*Script::var_prefix == '.')
		setString(path, sizeof(path), ".access");
	else
		snprintf(path, sizeof(path), "%s" PRE_AUTH "%s" EXT_AUTH,
			Script::var_prefix, group);

	fp = NULL;
}

PasswordThread::PasswordThread(ScriptInterp *interp, const char *group) :
ScriptThread(interp, 0)
{
	user = interp->getValue(NULL);
	pass = interp->getValue(NULL);
	save = interp->getValue(NULL);

	if(*Script::var_prefix == '.')
		setString(path, sizeof(path), ".access");
	else
		snprintf(path, sizeof(path), "%s" PRE_AUTH "%s" EXT_AUTH,
			Script::var_prefix, group);

	fp = NULL;
}


IdentifyThread::IdentifyThread(ScriptInterp *interp, const char *group) :
ScriptThread(interp, 0)
{
	user = interp->getValue(NULL);
	prefix = interp->getKeyword("prefix");

	if(*Script::var_prefix == '.')
		setString(path, sizeof(path), ".access");
	else
		snprintf(path, sizeof(path), "%s" PRE_AUTH "%s" EXT_AUTH,
			Script::var_prefix, group);
	fp = NULL;
}

AuthorizeThread::~AuthorizeThread()
{
	terminate();

	if(fp)
		::fclose(fp);
}

PasswordThread::~PasswordThread()
{
	terminate();

	if(fp)
		::fclose(fp);
}


IdentifyThread::~IdentifyThread()
{
	terminate();

	if(fp)
		::fclose(fp);
}

LookupThread::~LookupThread()
{
	terminate();

	if(fp)
		::fclose(fp);
}

void AuthorizeThread::run(void)
{
	char *tok, *uid = NULL, *pwd = NULL;
	time_t now;

	fp = ::fopen(path, "r");
	if(!fp) {
		exitEvent("authorize:missing");
		exit("authorize-missing");
	}

	while(fp) {
		if(!fgets(path, sizeof(path), fp) || feof(fp)) {
			exitEvent("authorize:missing");
			exit("authorize-missing");
		}

		uid = strtok_r(path, ":\r\n", &tok);
		if(!uid)
			continue;
		if(!stricmp(uid, user)) {
			pwd = strtok_r(NULL, ":\r\n", &tok);
			break;
		}
	}
#ifdef	WIN32
	if(stricmp(pass, pwd))
	    pass = NULL;
#else
	authlock.enter();
	pass = crypt(pass, pwd);
	if(stricmp(pass, pwd))
		pass = NULL;
	authlock.leave();
#endif

	if(!pass) {
		exitEvent("authorize:failed");
		exit("authorize-failed");
	}
	time(&now);
	--now;
	if(sym)
		snprintf(sym->data, 12, "%ld", (long)now);

	if(!prefix)
		exit(NULL);

	snprintf(path, sizeof(path), "%s/%s/%s",
		Script::var_prefix, prefix, uid);
	Dir::create(path);
	exit(NULL);
}

void PasswordThread::run(void)
{
	char *tok = NULL, *uid = NULL, *pwd = NULL;
	char buf[128];
	fpos_t fpos;
	time_t now;
	char salt[3];
	char *cp;

	fp = ::fopen(path, "r+");
	if(!fp) {
		exitEvent("password:missing");
		exit("password-missing");
	}

	while(fp) {
		fgetpos(fp, &fpos);
		if(!fgets(path, sizeof(path), fp) || feof(fp)) {
			exitEvent("password:missing");
			exit("password-missing");
		}

		setString(buf, sizeof(buf), path);
		uid = strtok_r(path, ":\r\n", &tok);
		if(!uid)
			continue;
		if(!stricmp(uid, user)) {
			pwd = strtok_r(NULL, ":\r\n", &tok);
			break;
		}
	}

#ifdef	WIN32
	pass = NULL;
#else
	if(stricmp(pass, "-")) {
		authlock.enter();
		pass = crypt(pass, pwd);
		if(strcmp(pass, pwd))
			pass = NULL;
		authlock.leave();
	}
#endif

	if(!pass) {
		exitEvent("password:failed");
		exit("password-failed");
	}

	time(&now);
	srand((int)now);
	to64(salt, rand(), 2);
	cp = strchr(buf, ':');
#ifndef	WIN32
	authlock.enter();
	strcpy(++cp, crypt(save, salt));
	authlock.leave();
#endif
	fsetpos(fp, &fpos);
	fputs(buf, fp);
	fflush(fp);
	exit(NULL);
}

void LookupThread::run(void)
{
	char *tok, *uid, *pwd, *tag, *val;
	unsigned len;
	unsigned count = 0;
	char buf[128];

	if(key && *key)
	        fp = ::fopen(path, "r");

	if(!fp || !sym) {
		exitEvent("lookup:missing");
		exit("lookup-missing");
	}

	len = strlen(key);

	for(;;)
	{
		if(!fgets(path, sizeof(path), fp) || feof(fp))
			break;

		uid = strtok_r(path, ":\r\n", &tok);
		pwd = strtok_r(NULL, ":\r\n", &tok);
		tag = strtok_r(NULL, ":\r\n", &tok);
		val = strtok_r(NULL, ":\r\n", &tok);

		if(!tag)
			continue;

		if(!val)
			val = tag;

		if(strnicmp(key, tag, len))
			continue;

		if(prefix) {
			snprintf(buf, sizeof(buf), "%s/%s/%s",
				Script::var_prefix, prefix, uid);
			Dir::create(buf);
		}

		snprintf(buf, sizeof(path), "%s,%s", uid, val);
		commit(sym, buf);
		Thread::yield();
		++count;
	}

	if(!count)
		exitEvent("lookup:missing");

	exit(NULL);
}

void IdentifyThread::run(void)
{
	char *tok, *uid;

	fp = ::fopen(path, "r");
	if(!fp) {
		exitEvent("identify:missing");
		exit("identify-missing");
	}

	for(;;)
	{
		if(!fgets(path, sizeof(path), fp) || feof(fp)) {
			exitEvent("identify:missing");
			exit("identify-missing");
		}

		uid = strtok_r(path, ":\r\n", &tok);
		if(!uid)
			continue;

		if(stricmp(uid, user))
			continue;

		if(prefix) {
			snprintf(path, sizeof(path), "%s/%s/%s",
				Script::var_prefix, prefix, uid);
			Dir::create(path);
		}

		exitEvent("identify:found");
		exit(NULL);
	}
}

bool UserauthMethods::scrLogout(void)
{
	Symbol *sym = mapSymbol("script.authorize", 0);
	if(sym)
		sym->data[0] = 0;
	advance();
	return true;
}

bool UserauthMethods::scrAuthorize(void)
{
	char buf[128];
	Name *scr = getName();
	const char *grp = getMember();
	char *cp;
	Symbol *sym = mapSymbol("script.authorize", 0);

	if(!grp) {
		setString(buf, sizeof(buf), scr->name);
		cp = strchr(buf, ':');
		if(cp)
			*cp = 0;
		grp = buf;
	}

	if(sym)
		sym->data[0] = 0;

	release();
	new AuthorizeThread(dynamic_cast<ScriptInterp*>(this), grp, sym);
	return false;
}

bool UserauthMethods::scrPassword(void)
{
	char buf[128];
	Name *scr = getName();
	const char *grp = getMember();
	char *cp;

	if(!grp) {
		setString(buf, sizeof(buf), scr->name);
		cp = strchr(buf, ':');
		if(cp)
			*cp = 0;
		grp = buf;
	}

	release();
	new PasswordThread(dynamic_cast<ScriptInterp*>(this), grp);
	return false;
}

bool UserauthMethods::scrLookup(void)
{
	char buf[128];
	Name *scr = getName();
	const char *grp = getMember();
	char *cp;
	Symbol *sym;

	if(!grp) {
		setString(buf, sizeof(buf), scr->name);
		cp = strchr(buf, ':');
		if(cp)
			*cp = 0;
		grp = buf;
	}

	sym = mapSymbol(getOption(), 0);
	if(!sym) {
		error("target-missing");
		return true;
	}

	clear(sym);

	release();

	new LookupThread(dynamic_cast<ScriptInterp*>(this), grp, sym);
	return false;
}

bool UserauthMethods::scrIdentify(void)
{
	char buf[128];
	Name *scr = getName();
	const char *grp = getMember();
	char *cp;

	if(!grp) {
		setString(buf, sizeof(buf), scr->name);
		cp = strchr(buf, ':');
		if(cp)
			*cp = 0;
		grp = buf;
	}

	release();
	new IdentifyThread(dynamic_cast<ScriptInterp*>(this), grp);
	return false;
}

const char *UserauthChecks::chkIdentify(Line *line, ScriptImage *img)
{
	unsigned idx = 0;

	if(!useKeywords(line, "=prefix"))
		return "invalid keyword for identify";

	if(!getOption(line, &idx))
		return "user id missing";

	if(getOption(line, &idx))
		return "too many arguments for identify";

	return NULL;
}

const char *UserauthChecks::chkLookup(Line *line, ScriptImage *img)
{
	unsigned idx = 0;
	const char *cp;

	if(!useKeywords(line, "=prefix"))
		return "invalid keyword for lookup";

	cp = getOption(line, &idx);
	if(!cp)
		return "destination array missing";

	if(*cp != '%' && *cp != '&')
		return "destination not variable";

	if(!getOption(line, &idx))
		return "lookup key missing";

	if(getOption(line, &idx))
		return "too many arguments for lookup";

	return NULL;
}

const char *UserauthChecks::chkAuthorize(Line *line, ScriptImage *img)
{
	unsigned idx = 0;

	if(!useKeywords(line, "=prefix"))
		return "invalid keyword for authorize";

	if(!getOption(line, &idx))
		return "user id missing";

	if(!getOption(line, &idx))
		return "password missing";

	if(getOption(line, &idx))
		return "too many arguments for authorize";

	return NULL;
}

const char *UserauthChecks::chkPassword(Line *line, ScriptImage *img)
{
	unsigned idx = 0;

	if(hasKeywords(line))
		return "password does not use keywords";

	if(!getOption(line, &idx))
		return "user id missing";

	if(!getOption(line, &idx))
		return "old password missing";

	if(!getOption(line, &idx))
		return "new password missing";

	if(getOption(line, &idx))
		return "too many arguments for authorize";

	return NULL;
}

const char *UserauthChecks::chkLogout(Line *line, ScriptImage *img)
{
	if(getMember(line))
		return "logout has no members";

	if(line->argc)
		return "logout has no arguments or keywords";

	return NULL;
}

};


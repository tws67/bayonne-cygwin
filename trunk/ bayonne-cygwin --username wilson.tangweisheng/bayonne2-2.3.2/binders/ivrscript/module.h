// Copyright (C) 2005 Open Source Telecom Corporation.
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

#include <bayonne.h>
#include <cc++/slog.h>
#include <cc++/socket.h>

#ifdef  WIN32
#define SCRIPT_EXTENSIONS ".bat.cmd.php.py.pl"
#else
#define SCRIPT_EXTENSIONS ".sh.py.pl.php"
#endif  

#ifndef	SCRIPT_BINDER_SELECT
#error "ccscript 0.8.1 or later required"
#endif

namespace binder {
using namespace ost;
using namespace std;

class Binder : public BayonneBinder, ScriptChecks
{
private:
        void attach(ScriptInterp *interp);
        void detach(ScriptInterp *interp);
	bool select(ScriptInterp *interp);
        bool reload(ScriptCompiler *img);
	void down(void);

        void compileDir(const char *prefix, ScriptCompiler *img);

	Name *getIncoming(ScriptImage *img, BayonneSession *s, Event *event);

	static bool testKey(ScriptInterp *interp, const char *v);
	static bool testRegistered(ScriptInterp *interp, const char *v);
	static bool testExternal(ScriptInterp *interp, const char *v);
	static bool testAvailable(ScriptInterp *interp, const char *v);
	static bool testReachable(ScriptInterp *interp, const char *v);
	static bool testDestination(ScriptInterp *interp, const char *v);

public:
        Binder();

        static Binder ivrscript;
}; 

class PersistProperty : public ScriptProperty, public Bayonne
{
private:
	static ScriptSymbols syms;
	static ThreadLock lock;
	static bool loaded;

#define	PERSIST_CACHE_SIZE 64

	typedef	struct
	{
		char cache[PERSIST_CACHE_SIZE];
		Symbol *sym;
	} persist_t;

public:
	PersistProperty();

	void set(const char *data, char *temp, unsigned size);

	static bool remap(const char *id, char *save, const char *val = "");

	static bool refresh(Symbol *sym, const char *ind, const char *val = "");

	static bool test(const char *key);

	static void save(void);

	static void load(void);

	static inline unsigned getSize(void)
		{return sizeof(persist_t) + sizeof(ScriptProperty *);};
};

class Checks : public ScriptChecks, public Bayonne
{
public:
        const char *chkCDR(Line *line, ScriptImage *img); 
        const char *chkRegister(Line *line, ScriptImage *img);
		const char *chkKeydata(Line *line, ScriptImage *img);
        const char *chkAssign(Line *line, ScriptImage *img);
        const char *chkSelect(Line *line, ScriptImage *img);   
        const char *chkKey(Line *line, ScriptImage *img);  
	const char *chkConnect(Line *line, ScriptImage *img);
	const char *chkStart(Line *line, ScriptImage *img);
	const char *chkStop(Line *line, ScriptImage *img);
	const char *chkJoin(Line *line, ScriptImage *img);
	const char *chkDial(Line *line, ScriptImage *img);
	const char *chkEndinput(Line *line, ScriptImage *img);
	const char *chkReconnect(Line *line, ScriptImage *img);
	const char *chkRecall(Line *line, ScriptImage *img);
	const char *chkRing(Line *line, ScriptImage *img);
	const char *chkFeature(Line *line, ScriptImage *img);
	const char *chkRedial(Line *line, ScriptImage *img);
	const char *chkErase(Line *line, ScriptImage *img);
	const char *chkGreeting(Line *line, ScriptImage *img);
};

class Methods : public BayonneSession
{
public:
	void msgAudio(void);
	bool scrKeydata(void);
	bool scrKey(void);
	bool scrRing(void);
	bool scrRelease(void);
	bool scrRecall(void);
	bool scrConnect(void);
	bool scrStart(void);
	bool scrStop(void);
	bool scrJoin(void);
	bool scrDial(void);
	bool scrForm(void);
	bool scrEndform(void);
	bool scrEndinput(void);
	bool scrReconnect(void);
	bool scrFeature(void);
	bool scrRedial(void);
	bool scrErase(void);
	bool scrGreeting(void);
};

} // namespace

	

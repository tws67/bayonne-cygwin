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
#include <cc++/xml.h>

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

class Compile : public Script
{
public:
	Name *script;
	Line *last[TRAP_BITS + 1];
	unsigned lnum;
	unsigned long addmask, submask, trapmask, addterm;
	unsigned char loopid[TRAP_BITS + 1], looplevel[TRAP_BITS + 1];
	unsigned trap;
	unsigned long termdig;
	const char *logname;
	bool repeated;
	unsigned long ttsmask;
	const char *ttsvoice;
	bool form;
	const char *submit;
	unsigned formcount;
};

class ParseImage : public ScriptImage, public Bayonne
{
public:
	ParseImage();

	const char *dupString(const char *str);
	unsigned getList(const char **args, const char *text, unsigned len, unsigned max);
	void getCompile(Compile *cc, const char *name = "1");
	void postCompile(Compile *cc, unsigned long mask = 0);
	void addCompile(Compile *cc, unsigned tidx, const char *cmd, 
		const char **args);
};

class ParseThread : public ScriptThread, protected XMLStream, public Bayonne
{
private:
	Compile main, block;
	Compile *current;
	ParseImage *img;
	ScriptImage **ip;
	char buffer[512];

	const char *voice;
	const char *lang;
	bool document;
	unsigned bcount;
	const char *bnext;

	typedef enum
	{
		TEXT_NONE,
		TEXT_DEBUG,
		TEXT_ERROR,
		TEXT_NOTICE,
		TEXT_ECHO,
		TEXT_TTS
	}	textstate_t;

	union
	{
		int fd;
		void *handle;
	}	file;

	textstate_t textstate[65];
	unsigned textstack;

	void run(void);

	int read(unsigned char *buffer, size_t len);

	void characters(const unsigned char *text, size_t len);
        void startElement(const unsigned char *name,
                const unsigned char **attrib);
        void endElement(const unsigned char *name);

	void doAssign(const char **attrib);
	void doLog(const char **attrib, textstate_t mode);
	void doHangup(void);
	void doAnswer(void);
	void doDelay(const char **attrib);
	void doText(const char **attrib);
	void doGoto(const char **attrib);
	void doReorder(void);

	void playAudio(const char **attrib);
	void playNumber(const char **attrib);
	void playTone(const char **attrib);
	void record(const char **attrib);
	void getDigits(const char **attrib);

	void startTerm(const char **attrib);
	void startTrap(const char **attrib, unsigned trap = 0);
	void endTrap(void);
	void endTerm(void);
	void startBlock(const char **attrib);
	void endBlock(void);
	void startDocument(const char **attrib);
	void endDocument(void);

public:
	ParseThread(ScriptInterp *interp, const char *url, ScriptImage **img );
	~ParseThread();
};

class Binder : public BayonneBinder, ScriptChecks
{
private:
        void attach(ScriptInterp *interp);
        void detach(ScriptInterp *interp);
	bool select(ScriptInterp *interp);
        bool reload(ScriptCompiler *img);
	void down(void);

    Name *getIncoming(ScriptImage *img, BayonneSession *s, Event *event);

	static bool testKey(ScriptInterp *interp, const char *v);

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
        const char *chkKeydata(Line *line, ScriptImage *img);
        const char *chkKey(Line *line, ScriptImage *img);  
	const char *chkEndinput(Line *line, ScriptImage *img);
	const char *chkParse(Line *line, ScriptImage *img);
	const char *chkStart(Line *line, ScriptImage *img);
	const char *chkSelect(Line *line, ScriptImage *img);
};

class Methods : public BayonneSession
{
public:
	bool scrKey(void);
	bool scrForm(void);
	bool scrEndform(void);
	bool scrEndinput(void);
	bool scrParse(void);
	bool scrStart(void);
	bool scrKeydata(void);

	bool xmlAssign(void);
	bool xmlDebug(void);
	bool xmlError(void);
	bool xmlNotice(void);
	bool xmlHangup(void);
	bool xmlVoice(void);
};

} // namespace

	

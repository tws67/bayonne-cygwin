// Copyright (C) 2005 Open Source Telecom Corp.
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

#include "bayonne.h"
#include <cc++/process.h>
#include <cc++/slog.h>

#ifndef	WIN32
#include "private.h"
#endif

#ifdef	HAVE_LIBEXEC
#include "libexec.h"
#endif

#ifndef	VERSION
#define	VERSION "0.3.1"
#endif

#ifdef	WIN32
#define	SCRIPT_EXTENSIONS ".bat.cmd.php.py.pl"
#else
#define	SCRIPT_EXTENSIONS ".sh.py.pl.php"
#endif

#if defined(_MSC_VER) && _MSC_VER >= 1300
#if defined(_WIN64_) || defined(__WIN64__)
#define RLL_SUFFIX ".x64"
#elif defined(_M_IX86)
#define RLL_SUFFIX ".x86"
#else
#define RLL_SUFFIX ".xlo"
#endif
#endif

#if defined(__MINGW32__) | defined(__CYGWIN32__)
#define RLL_SUFFIX ".dso"
#endif

#ifdef  W32
#ifndef RLL_SUFFIX
#define RLL_SUFFIX ".rll"
#endif
#endif

#ifndef RLL_SUFFIX
#define RLL_SUFFIX ".dso"
#endif

namespace server {
using namespace ost;
using namespace std;

class ListThread : public ScriptThread, public Dir
{
private:
	const char *prefix, *suffix, *after, *prior;
	Symbol *sym, *save;
	bool longinfo;
	char filepath[MAX_PATHNAME];
	unsigned fplen;
	AudioFile af;
	char last[65];

	void run(void);

public:
	ListThread(ScriptInterp *interp, const char *dir, Symbol *var);
	~ListThread();	
};
	
class InfoThread : public ScriptThread
{
private:
	BayonneAudio *audio;
	const char *path;
	const char *var_date, *var_time, *var_size, *var_info, *var_type;
	const char *var_coding, *var_rate, *var_bitrate, *var_count;
	struct stat ino;
	
	void run(void);
public:
	InfoThread(ScriptInterp *interp, BayonneAudio *a, const char *fn);
	~InfoThread();
};

class BuildThread : public ScriptThread, public Audio
{
private:
	char pathname[MAX_PATHNAME];
	char destname[MAX_PATHNAME];
	const char **paths;
	const char *dest;
	Encoded buffer;
	Linear lbuffer;
	Info from, to;
	AudioStream out;
	BayonneAudio *in;
	bool completed;

#ifdef	AUDIO_RATE_RESAMPLER
	AudioResample *resampler;
	Linear resample;
#endif

	void copyDirect(void);
	void copyConvert(void);

	void run(void);

public:
	BuildThread(ScriptInterp *interp, BayonneAudio *au, Audio::Info *inf, const char *d, const char **list);
	~BuildThread();
};

class CopyThread : public ScriptThread
{
private:
	const char *src, *dest;
	FILE *in, *out;
	char buf[1024];
	char fn1[MAX_PATHNAME];
	char fn2[MAX_PATHNAME];

	void run(void);
public:
	CopyThread(ScriptInterp *interp, const char *from, const char *to);
	~CopyThread();
};

class WriteThread : public ScriptThread
{
private:
	char text[512];
	char path[MAX_PATHNAME];
	const char *cp;
	FILE *fp;

public:
	WriteThread(ScriptInterp *interp, const char *from);
	~WriteThread();

	void run(void);
};

class PositionProperty : public ScriptProperty, public Audio
{
public:
	PositionProperty();

	void set(const char *data, char *temp, unsigned size);

	long getValue(const char *data);

	char token(void);

	unsigned prec(void);

	void clear(char *data, unsigned size);
};

class TimeslotProperty : public ScriptProperty, public Bayonne
{
public:
	TimeslotProperty();

	void set(const char *data, char *temp, unsigned size);

	long getValue(const char *data);

	long adjustValue(long value);

//	void adjust(char *data, size_t size, long offset);
};

class Runtime : public ScriptRuntime, public Bayonne
{
protected:
        unsigned long getTrapMask(const char *trapname);

	static bool testSafe(ScriptInterp *interp, const char *v);
	static bool testFile(ScriptInterp *interp, const char *v);
	static bool testDir(ScriptInterp *interp, const char *v);
	static bool testTimeslot(ScriptInterp *interp, const char *v);
	static bool testSpan(ScriptInterp *interp, const char *v);
	static bool testDriver(ScriptInterp *interp, const char *v);
	static bool testVoice(ScriptInterp *interp, const char *v);
	static bool testLang(ScriptInterp *interp, const char *v);
	static bool testLevel(ScriptInterp *interp, const char *v);
	static bool testPattern(ScriptInterp *interp, const char *v);
	static bool testDialed(ScriptInterp *interp, const char *v);
    static bool testCaller(ScriptInterp *interp, const char *v);
	static bool testDetach(ScriptInterp *interp, const char *v);

public:
	static void process(void);

	static bool command(char *line);

	const char *getExternal(const char *opt);

	void errlog(const char *level, const char *msg);

	bool isInput(Line *line);

	bool loadBinder(const char *id);

	Runtime();
};

class Methods : public BayonneSession
{
public:
	bool scrId(void);
	bool scrTrap(void);
	bool scrBGM(void);
	bool scrList(void);
	bool scrEcho(void);
	bool scrSay(void);
	bool scrPickup(void);
	bool scrAnswer(void);
	bool scrHangup(void);
	bool scrWaitkey(void);
	bool scrSleep(void);
	bool scrSync(void);
	bool scrCleardigits(void);
	bool scrKeyinput(void);
	bool scrCollect(void);
	bool scrInput(void);
	bool scrPath(void);
	bool scrRead(void);
	bool scrRoute(void);
	bool scrSRoute(void);
	bool scrReconnect(void);
	bool scrDetach(void);
	bool scrWrite(void);
	bool scrErase(void);
	bool scrMove(void);
	bool scrLink(void);
	bool scrCopy(void);
	bool scrTimeslot(void);
	bool scrPosition(void);
	bool scrPathname(void);
	bool scrReadpath(void);
	bool scrWritepath(void);
	bool scrVoicelib(void);
	bool scrParam(void);
	bool scrForm(void);
	bool scrEndform(void);
	bool scrEndinput(void);
	bool scrLibexec(void);
	bool scrPlay(void);
	bool scrReplay(void);
	bool scrRecord(void);
	bool scrAppend(void);
	bool scrPrompt(void);
	bool scrBuild(void);
	bool scrTone(void);
	bool scrTonegen(void);
	bool scrDTMF(void);
	bool scrMF(void);
	bool scrTransfer(void);
	bool scrExit(void);
};

class Checks : public ScriptChecks, public Bayonne
{
public:
	const char *chkId(Line *line, ScriptImage *img);
	const char *chkTrap(Line *line, ScriptImage *img);
	const char *chkSay(Line *line, ScriptImage *img);
        const char *chkEndinput(Line *line, ScriptImage *img); 
	const char *chkBGM(Line *line, ScriptImage *img);
	const char *chkWrite(Line *line, ScriptImage *img);
	const char *chkList(Line *line, ScriptImage *img);
	const char *chkConfig(Line *line, ScriptImage *img);
	const char *chkPaths(Line *line, ScriptImage *img);
	const char *chkTonegen(Line *line, ScriptImage *img);
	const char *chkSleep(Line *line, ScriptImage *img);
	const char *chkCleardigits(Line *line, ScriptImage *img);
	const char *chkKeyinput(Line *line, ScriptImage *img);
	const char *chkInput(Line *line, ScriptImage *img);
	const char *chkReconnect(Line *line, ScriptImage *img);
	const char *chkCollect(Line *line, ScriptImage *img);
	const char *chkRoute(Line *line, ScriptImage *img);
	const char *chkErase(Line *line, ScriptImage *img);
	const char *chkDetach(Line *line, ScriptImage *img);
	const char *chkInfo(Line *line, ScriptImage *img);
	const char *chkMove(Line *line, ScriptImage *img);
	const char *chkDir(Line *line, ScriptImage *img);
	const char *chkPathname(Line *line, ScriptImage *img);
	const char *chkReplay(Line *line, ScriptImage *img);
	const char *chkRecord(Line *line, ScriptImage *img);
	const char *chkAppend(Line *line, ScriptImage *img);
	const char *chkParam(Line *line, ScriptImage *img);
	const char *chkService(Line *line, ScriptImage *img);
	const char *chkVoicelib(Line *line, ScriptImage *img);
	const char *chkLibexec(Line *line, ScriptImage *img);
	const char *chkLang(Line *line, ScriptImage *img);
	const char *chkLoad(Line *line, ScriptImage *img);
	const char *chkBuild(Line *line, ScriptImage *img);
	const char *chkTone(Line *line, ScriptImage *img);
	const char *chkTransfer(Line *line, ScriptImage *img);
	const char *chkChildSignal(Line *line, ScriptImage *img);
	const char *chkDialer(Line *line, ScriptImage *img);
	const char *chkExit(Line *line, ScriptImage *img);
};

extern Runtime runtime;
extern StaticKeydata keypaths;
extern StaticKeydata keyserver;
extern StaticKeydata keyengine;
extern Keydata keyoptions;
extern BayonneConfig keylibexec;
extern BayonneConfig keysmtp;
extern BayonneConfig keyurl;

extern TimeslotProperty typeTimeslot;
extern PositionProperty typePosition;

extern bool flag_daemon;
extern bool flag_check;

void purgedir(const char *dir);
void loadConfig(void);
void liveConfig(void);
void dumpConfig(Keydata &keydef);
bool parseConfig(char **argv);

}


// Copyright (C) 2006-2008 David Sugar, Tycho Softworks.
//
// This file is part of GNU ccAudio2.
//
// GNU ccAudio2 is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published 
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// GNU ccAudio2 is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with GNU ccAudio2.  If not, see <http://www.gnu.org/licenses/>.

#include "tonetool.h"

#ifndef	VERSION
#define	VERSION	"1.3.0"
#endif

const char *delfile = NULL;

extern "C" {

#ifdef	W32

static BOOL WINAPI down(DWORD ctrltype)
{
	if(delfile) {
		remove(delfile);
		delfile = NULL;
	}
	exit(ctrltype);
	return TRUE;
	}

#else
static void down(int signo)
{
	if(delfile)
		remove(delfile);
	exit(signo);
	}
#endif

}

#ifdef	W32
void main(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
	char *cp;

	if(argc < 2) {
		cerr << "use: tonetool --option [args...]" << endl;
		exit(-1);
	}
	++argv;
	cp = *argv;
	if(!strncmp(cp, "--", 2))
		++cp;

#ifdef	W32
	SetConsoleCtrlHandler(down, TRUE);
#else
	signal(SIGINT, down);
	signal(SIGTERM, down);
	signal(SIGQUIT, down);
	signal(SIGABRT, down);
#endif

	if(!stricmp(cp, "-list"))
		Tool::list(++argv);
//	else if(!stricmp(cp, "-play"))
//		Tool::play(++argv);
	else if(!stricmp(cp, "-create"))
		Tool::write(++argv, false);
	else if(!stricmp(cp, "-append"))
		Tool::write(++argv, true);
	else if(!stricmp(cp, "-detect"))
		Tool::detect(++argv);

	cerr << "tonetool: " << *argv << ": unknown option" << endl;
	exit(-1);
}

using namespace ost;

bool Tool::isFile(const char *path)
{
#ifdef W32
	DWORD attr = GetFileAttributes(path);
	if(attr == (DWORD)~0l)
		return false;

	if(attr & FILE_ATTRIBUTE_DIRECTORY)
		return false;

	return true;

#else
	struct stat ino;

	if(stat(path, &ino))
		return false;

	if(S_ISREG(ino.st_mode))
		return true;

	return false;
#endif // WIN32
}

bool Tool::canAccess(const char *path)
{
#ifdef WIN32
	DWORD attr = GetFileAttributes(path);
	if(attr == (DWORD)~0l)
		return false;

	if(attr & FILE_ATTRIBUTE_SYSTEM)
		return false;

	if(attr & FILE_ATTRIBUTE_HIDDEN)
		return false;

	return true;
#else
	if(!access(path, R_OK))
		return true;

	return false;

#endif
}

AudioTone *Tool::getTone(char **argv, Level l, timeout_t framing, timeout_t interdigit)
{
	TelTone::tonekey_t *key;
	char *name, *locale;

	if(!stricmp(*argv, "dtmf"))
		return new DTMFTones(*(++argv), l, framing, interdigit);
	else if(!stricmp(*argv, "mf"))
		return new MFTones(*(++argv), l, framing, interdigit);

	name = *(argv++);
	locale = *(argv);
	key = TelTone::find(name, locale);
	if(!key)
		return NULL;

	return new TelTone(key, l, framing);
}


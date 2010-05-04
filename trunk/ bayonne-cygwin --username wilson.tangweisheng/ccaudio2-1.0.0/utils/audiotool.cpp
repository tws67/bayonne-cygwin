// Copyright (C) 1999-2005 Open Source Telecom Corporation.
// Copyright (C) 2006-2008 David Sugar, Tycho Softworks.
//
// This file is part of GNU ccAudio2.
//
// GNU ccAudio2 is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published 
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// GNU ccAuydio2 is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with GNU ccAudio2.  If not, see <http://www.gnu.org/licenses/>.

#include "audiotool.h"

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

static void version(void)
{
	cout << VERSION << endl;
	exit(0);
}

static void soundcard(unsigned index)
{
	AudioDevice *soundcard = Audio::getDevice(index);
	Audio::Info *info;

	if(!Audio::hasDevice(index) && !soundcard)
		cout << "Sound device inaccessible or unsupported" << endl;
	else if(!soundcard)
		cout << "Sound device unavailable" << endl;
	else {
		info = soundcard->getInfo();
		cout << "Soundcard Driver: " << info->annotation << endl;
		if(Audio::isStereo(info->encoding))
			cout << "Default Channels: 2" << endl;
		else
			cout << "Default Channels: 1" << endl;
		cout << "Default Encoding: " << Audio::getName(info->encoding) << endl;
		cout << "Default Buffers: " << info->framecount << endl;
		cout << "Default Framing: " << info->framing << "ms" << endl;
		cout << "Default Rate: " << info->rate << " samples per second" << endl;
	}
	exit(0);
}

#ifdef	CODEC_MODFLAGS
static void modflags(void)
{
	cout << CODEC_MODFLAGS << endl;
	exit(0);
}
#endif

#ifdef	AUDIO_LIBRARY
static void ldflags(void)
{
	cout << AUDIO_LIBRARY << endl;
	exit(0);
}
#endif

static void libpath(void)
{
	cout << Audio::getCodecPath() << endl;
	exit(0);
}

static void endian(void)
{
	if(__BYTE_ORDER == __BIG_ENDIAN)
		cout << "big" << endl;
	else
		cout << "little" << endl;
	exit(0);
}

#ifdef	W32
void main(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
	char *cp;

	if(argc < 2) {
		cerr << "use: audiotool --option [args...]" << endl;
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

	if(!stricmp(cp, "-version"))
		version();
	else if(!stricmp(cp, "-modpath") || !stricmp(cp, "-path"))
		libpath();
	else if(!stricmp(cp, "-endian"))
		endian();
	else if(!stricmp(cp, "-soundcard"))
	{
		if(*(++argv))
			soundcard(atoi(*argv));
		else
			soundcard(0);
	}
#ifdef	CODEC_MODFLAGS
	else if(!stricmp(cp, "-modflags"))
		modflags();
#endif
#ifdef	AUDIO_LIBRARY
	else if(!stricmp(cp, "-ldflags"))
		ldflags();
#endif
	else if(!stricmp(cp, "-info"))
		Tool::info(++argv);
	else if(!stricmp(cp, "-size"))
		Tool::size(++argv);
	else if(!stricmp(cp, "-notation") || !stricmp(cp, "-annotation"))
		Tool::notation(++argv);
	else if(!stricmp(cp, "-chart"))
		Tool::chart(++argv);
	else if(!stricmp(cp, "-strip"))
		Tool::strip(++argv);
	else if(!stricmp(cp, "-trim"))
		Tool::trim(++argv);
	else if(!stricmp(cp, "-build"))
		Tool::build(++argv);
	else if(!stricmp(cp, "-append"))
		Tool::append(++argv);
	else if(!stricmp(cp, "-play"))
		Tool::play(++argv);
	else if(!stricmp(cp, "-packets"))
		Tool::packetdump(++argv);

	cerr << "audiotool: " << *argv << ": unknown option" << endl;
	exit(-1);
}

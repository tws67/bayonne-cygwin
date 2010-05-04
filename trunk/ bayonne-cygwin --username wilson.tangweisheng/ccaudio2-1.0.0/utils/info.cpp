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
// GNU ccAudio2 is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with GNU ccAudio2.  If not, see <http://www.gnu.org/licenses/>.

#include "audiotool.h"

static const char *fname(const char *cp)
{
	const char *fn = strrchr(cp, '/');
	if(!fn)
		fn = strrchr(cp, '\\');
	if(fn)
		return ++fn;
	return cp;
}

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

void Tool::info(char **argv)
{
	AudioFile au;
	Info info;
	const char *fn;
	timeout_t framing = 0;
	unsigned long size, end;
	unsigned long minutes, seconds, subsec, scale;
	char duration[32];

	fn = *argv;
	if(!strnicmp(fn, "--", 2))
		++fn;
	if(!strnicmp(fn, "-framing=", 9)) {
		framing = atoi(fn + 9);
		++argv;
	}
	else if(!stricmp(fn, "-framing"))
	{
		framing = atoi(*(++argv));
		++argv;
	}

	while(*argv) {
		if(!isFile(*argv)) {
			cout << "audiotool: " << fname(*(argv++)) << ": invalid" << endl;
			continue;
		}
		if(!canAccess(*argv)) {
			cout << "audiotool: " << fname(*(argv++)) << ": inaccessable" << endl;
			continue;
		}
		au.open(*argv, modeInfo, framing);
		au.getInfo(&info);
		au.setPosition();
		size = end = au.getPosition();
		cout << fname(*(argv++)) << endl;
		cout << "    Format: ";
		fn = getMIME(info);
		if(fn)
			cout << fn << endl;
		else
			switch(info.format) {
			case raw:
				cout << "raw audio" << endl;
				break;
			case snd:
				cout << "sun audio" << endl;
				break;
			case riff:
				cout << "riff" << endl;
				break;
			case wave:
				cout << "ms wave" << endl;
				break;
			case mpeg:
				cout << "mpeg audio" << endl;
				break;
			}

		cout << "    Encoding: " << getName(info.encoding) << endl;
		if(isStereo(info.encoding))
			cout << "    Channels: 2" << endl;
		else
			cout << "    Channels: 1" << endl;
		if(info.framing)
			cout << "    Frame Size: " << info.framing << "ms" << endl;
		if(isLinear(info.encoding)) {
			cout << "    Byte Order: ";
			if(info.order == __BIG_ENDIAN)
				cout << "big" << endl;
			else if(info.order == __LITTLE_ENDIAN)
				cout << "little" << endl;
			else
				cout << "native" << endl;
		}
		cout << "    Sample Rate: " << info.rate << endl;
		cout << "    Encoding Bit Rate: " << info.bitrate << endl;
		cout << "    Total Samples: " << end << endl;

		scale = info.rate / 1000;

		subsec = (end % info.rate) / scale;

		end /= info.rate;
		seconds = end % 60;
		end /= 60;
		minutes = end % 60;
		end /= 60;
		snprintf(duration, sizeof(duration),
			"%02ld:%02ld:%02ld.%03ld", end, minutes, seconds, subsec);
		cout << "    Total Duration: " << duration << endl;

		if(info.headersize)
			cout << "    Computed Frame Size: " << info.framesize - info.headersize - info.padding << ", header=" << info.headersize << ", padding=" << info.padding << endl;

		au.close();
	}
	exit(0);
}

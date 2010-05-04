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

void Tool::chart(char **argv)
{
	AudioFile file;
	Info info;
	AudioCodec *codec = NULL;
	char *fn;
	timeout_t framing = 20;
	Level silence = 0;
	unsigned char *buffer;
	short max, current;
	unsigned long sum;
	unsigned long count;

retry:
	if(!*argv) {
		cerr << "audiotool: -chart: missing arguments" << endl;
		exit(-1);
	}

	fn = *argv;

	if(!strcmp(fn, "--")) {
		++argv;
		goto skip;
	}

	if(!strnicmp(fn, "--", 2))
		++fn;
	if(!strnicmp(fn, "-framing=", 9)) {
		framing = atoi(fn + 9);
		++argv;
		goto retry;
	}
	else if(!stricmp(fn, "-framing"))
	{
		++argv;
		if(!*argv) {
			cerr << "audiotool: -chart: -framing: missing argument" << endl;
			exit(-1);
		}
		framing = atoi(*(argv++));
		goto retry;
	}

	if(!strnicmp(fn, "-silence=", 9)) {
		silence = atoi(fn + 9);
		++argv;
		goto retry;
	}
	else if(!stricmp(fn, "-silence"))
	{
		++argv;
		if(!*argv) {
			cerr << "audiotool: -chart: -silence: argument missing" << endl;
			exit(-1);
		}
		silence = atoi(*(argv++));
		goto retry;
	}

skip:

	if(!framing)
		framing = 20;

	while(*argv) {
		if(!isFile(*argv)) {
			cout << fname(*(argv++)) << ": invalid" << endl;
			continue;
		}
		if(!canAccess(*argv)) {
			cout << fname(*(argv++)) << ": inaccessable" << endl;
			continue;
		}
		file.open(*argv, modeRead, framing);
		file.getInfo(&info);
		if(!isLinear(info.encoding))
			codec = AudioCodec::getCodec(info, false);
		if(!isLinear(info.encoding) && !codec) {
			cout << fname(*(argv++)) << ": cannot load codec" << endl;
			continue;
		}

		cout << fname(*(argv++)) << ": " << flush;

		buffer = new unsigned char[info.framesize];

		max = 0;
		sum = 0;
		count = 0;

		// autochart for silence value

		while(!silence) {
			if(file.getBuffer(buffer, info.framesize) < (int)info.framesize)
				break;
			++count;
			if(codec)
				sum += codec->getImpulse(buffer, info.framecount);
			else
				sum += getImpulse(info, buffer, info.framecount);
		}

		if(!silence && count)
			silence = (Level)(((sum / count) * 2) / 3);

		max = 0;
		sum = 0;
		count = 0;

		file.setPosition(0);

		for(;;)
		{
			if(file.getBuffer(buffer, info.framesize) < (int)info.framesize)
				break;
			++count;
			if(codec) {
				current = codec->getPeak(buffer, info.framecount);
				if(current > max)
					max = current;
				sum += codec->getImpulse(buffer, info.framecount);
				if(codec->isSilent(silence, buffer, info.framecount)) {
					if(codec->getPeak(buffer, info.framecount) >= silence)
						cout << '^';
					else
						cout << '.';
				}
				else
					cout << '+';
				cout << flush;
				continue;
			}

			current = getPeak(info, buffer, info.framecount);
			if(current > max)
				max = current;

			sum += getImpulse(info, buffer, info.framecount);
			if(getImpulse(info, buffer, info.framecount) < silence) {
				if(getPeak(info, buffer, info.framecount) >= silence)
					cout << '^';
				else
					cout << '.';
			}
			else
				cout << '+';
			cout << flush;
		}
		cout << endl;
		if(count)
			cout 	<< "silence threashold = " << silence
				<< ", avg frame energy = " << (sum / count)
				<< ", peak level = " << max << endl;

		if(buffer)
			delete[] buffer;


		if(codec)
			AudioCodec::endCodec(codec);

		codec = NULL;
		buffer = NULL;

		file.close();
	}
	exit(0);
}

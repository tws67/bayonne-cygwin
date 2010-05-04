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

void Tool::trim(char **argv)
{
	AudioFile file, tmp;
	unsigned long first = 0, last = 0, total = 0, padding = 0;
	Info info;
	AudioCodec *codec = NULL;
	char *fn;
	timeout_t framing = 20;
	Level silence = 0;
	int rtn;
	unsigned char *buffer;
	Linear samples = NULL;
	Level max;
	unsigned long sum;
	unsigned long count;
	char target[PATH_MAX];
	bool use = false;

retry:
	if(!*argv) {
		cerr << "audiotool: -trim: missing arguments" << endl;
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
			cerr << "audiotool: -trim: -framing: missing argument" << endl;
			exit(-1);
		}
		framing = atoi(*(argv++));
		goto retry;
	}

	if(!strnicmp(fn, "-padding=", 9)) {
		padding = atoi(fn + 9);
		++argv;
		goto retry;
	}
	else if(!stricmp(fn, "-padding"))
	{
		++argv;
		if(!*argv) {
			cerr << "audiotool: --trim: -padding: argument missing" << endl;
			exit(-1);
		}
		padding = atol(*(argv++));
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
			cerr << "audiotool: -trim: -silence: argument missing" << endl;
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
			cout << *(argv++) << ": invalid" << endl;
			continue;
		}
		if(!canAccess(*argv)) {
			cout << *(argv++) << ": inaccessable" << endl;
			continue;
		}
		rewrite(*argv, target, PATH_MAX);
		setDelete(target);
		file.open(*argv, modeRead, framing);
		file.getInfo(&info);
		if(!isLinear(info.encoding))
			codec = AudioCodec::getCodec(info, false);
		if(!isLinear(info.encoding) && !codec) {
			cout << *(argv++) << ": cannot load codec" << endl;
			continue;
		}

		buffer = new unsigned char[info.framesize];

		max = 0;
		sum = 0;
		count = 0;

		// compute silence value

		while(!silence) {
			rtn = file.getBuffer(buffer, info.framesize);
			if(rtn < (int)info.framesize)
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
			rtn = file.getBuffer(buffer, info.framesize);
			if(rtn < (int)info.framesize)
				break;

			++total;
			if(codec) {
				if(codec->isSilent(silence, buffer, info.framecount)) {
					use = false;
					if(codec->getPeak(buffer, info.framecount) >= silence)
						use = true;
				}
				else
					use = true;
			}
			if(use && !first)
				first = total;
			if(use)
				last = total;
		}

		if(!last || !first) {
			cout << *(argv++) << ": all silent, skipping" << endl;
			continue;
		}

		--first;
		total = last - first;
		file.setPosition(first * info.framecount);

		tmp.create(target, &info);
		if(!tmp.isOpen()) {
			cout << *(argv++) << ": cannot rewrite" << endl;
			continue;
		}

		while(total--) {
			rtn = file.getBuffer(buffer, info.framesize);
			if(rtn < (int)info.framesize)
				break;
			tmp.putBuffer(buffer, info.framesize);
		}

		if(padding) {
			if(!codec)
				memset(buffer, 0, info.framesize);
			else if(isStereo(info.encoding))
			{
				samples = new Sample[info.framecount * 2];
				memset(samples, 0, info.framecount * 2);
				codec->encode(samples, buffer, info.framecount);
			}
			else {
				samples = new Sample[info.framecount];
				memset(samples, 0, info.framecount);
				codec->encode(samples, buffer, info.framecount);
			}
		}

		while(padding--) {
			rtn = tmp.putBuffer(buffer, info.framesize);
		}

		if(samples)
			delete[] samples;

		if(buffer)
			delete[] buffer;

		if(codec)
			AudioCodec::endCodec(codec);

		codec = NULL;
		buffer = NULL;

		file.close();
		tmp.close();

		rtn = rename(target, *argv);
		remove(target);
		setDelete(NULL);

		if(rtn)
			cout << *argv << ": could not be replaced" << endl;

		++argv;
	}
	exit(0);
}

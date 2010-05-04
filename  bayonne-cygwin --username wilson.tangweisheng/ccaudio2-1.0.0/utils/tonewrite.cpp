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

using namespace ost;

static const char *fname(const char *cp)
{
	const char *fn = strrchr(cp, '/');
	if(!fn)
		fn = strrchr(cp, '\\');
	if(fn)
		return ++fn;
	return cp;
}

void Tool::write(char **argv, bool append)
{
	AudioTone *tone;
	AudioStream output;
	Info info, make;
	const char *target;
	char *option;
	char *offset = NULL;
	char *encoding = (char *)"pcmu";
	Level level = 30000;
	timeout_t framing = 20, interdigit = 60;
	timeout_t maxtime = 60000;
	unsigned maxframes;
	Linear buffer;
	char *filename = (char *)"tones.conf";

retry:
	option = *argv;

	if(!strcmp("--", option)) {
		++argv;
		goto skip;
	}

	if(!strnicmp("--", option, 2))
		++option;

	if(!strnicmp(option, "-encoding=", 10) && !append) {
		encoding = option + 10;
		++argv;
		goto retry;
	}

	if(!stricmp(option, "-interdigit")) {
		++argv;
		if(*argv) {
			cerr << "tonetool: -interdigit: missing argument" << endl;
			exit(-1);
		}
		interdigit = atoi(*(argv++));
		goto retry;
	}

	if(!strnicmp(option, "-interdigit=", 12)) {
		interdigit = atoi(option + 12);
		++argv;
		goto retry;
	}

	if(!stricmp(option, "-framing")) {
		++argv;
		if(*argv) {
			cerr << "tonetool: -framing: missing argument" << endl;
			exit(-1);
		}
		framing = atoi(*(argv++));
		goto retry;
	}

	if(!strnicmp(option, "-framing=", 9)) {
		framing = atoi(option + 9);
		++argv;
		goto retry;
	}

	if(!stricmp(option, "-encoding")) {
		++argv;
		if(!*argv) {
			cerr << "tonetool: -encoding: missing argument" << endl;
			exit(-1);
		}
		encoding = *(argv++);
		goto retry;
	}

	if(!strnicmp(option, "-offset=", 8) && append) {
		offset = option + 8;
		++argv;
		goto retry;
	}

	if(!stricmp(option, "-offset") && append) {
		++argv;
		if(!*argv) {
				cerr << "tonetool: -offset: argument missing" << endl;
			exit(-1);
		}
		offset = *(argv++);
		goto retry;
	}

skip:
	if(*argv && **argv == '-') {
	        cerr << "tonetool: " << *argv << ": unknown option" << endl;
		exit(-1);
	}

	TelTone::load(filename);

	if(*argv)
		target = *(argv++);

	if(!*argv) {
		cerr << "teltones: no tone spec to use" << endl;
		exit(-1);
	}

	if(!append && encoding)
		make.encoding = getEncoding(encoding);

	make.rate = rate8khz;

	if(!append) {
		remove(target);
		output.create(target, &make, false, framing);
	}
	else {
		output.open(target, modeWrite, framing);
		if(offset)
			output.setPosition(atol(offset));
		else
			output.setPosition();
	}

	if(!output.isOpen()) {
		cerr << "teltones: " << target << ": cannot access" << endl;
		exit(-1);
	}

	if(!output.isStreamable()) {
		cerr << "teltones: " << target << ": unable to load codec" << endl;
		exit(-1);
	}

	output.getInfo(&info);

	tone = getTone(argv, level, info.framing, interdigit);

	if(!tone) {
		cerr << "teltones: unrecognized spec used" << endl;
		exit(-1);
	}

	cout << fname(target) << ": ";
	maxframes = maxtime / info.framing;

	while(maxframes--) {
		buffer = tone->getFrame();
		if(!buffer) {
			if(tone->isComplete())
				break;
			cout << '!';
			continue;
		}
		output.putMono(buffer);
		if(tone->isSilent())
			cout << '.';
		else
			cout << '+';
	}
	cout << endl;
	output.close();

	exit(0);
}


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

void Tool::detect(char **argv)
{
	DTMFDetect *dtmf;
	AudioStream input;
	Info info, make;
	const char *target;
	char *option;
	timeout_t framing = 20;
	Linear buffer;
	char result[128];

retry:
	option = *argv;

	if(!strcmp("--", option)) {
		++argv;
		goto skip;
	}

	if(!strnicmp("--", option, 2))
		++option;

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

skip:
	if(*argv && **argv == '-') {
	        cerr << "tonetool: " << *argv << ": unknown option" << endl;
		exit(-1);
	}


	while(*argv) {
		target = *(argv++);

		input.open(target, modeRead, framing);

		if(!input.isOpen()) {
			cerr << "tonetool: " << target << ": cannot access" << endl;
			exit(-1);
		}

		if(!input.isStreamable()) {
			cerr << "tonetool: " << target << ": unable to load codec" << endl;
			exit(-1);
		}

		input.getInfo(&info);
		dtmf = new DTMFDetect;

		buffer = new Sample[input.getCount()];
		for(;;)
		{
			if(!input.getMono(buffer, 1))
				break;

			dtmf->putSamples(buffer, input.getCount());
		}

		dtmf->getResult(result, sizeof(result));
		cout << fname(target) << ": " << result << endl;

		input.close();
		delete[] buffer;
		delete dtmf;
	}
	exit(0);
}


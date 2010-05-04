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

void Tool::list(char **argv)
{
	const char *filename = "tones.conf";
	const char *locale = NULL;
	const char *option;
	TelTone::tonekey_t *key;
	TelTone::tonedef_t *def;

retry:
	option = *argv;

	if(!strcmp("--", option)) {
		++argv;
		goto skip;
	}

	if(!strnicmp("--", option, 2))
		++option;

	if(!strnicmp(option, "-file=", 6)) {
		filename = option + 6;
		++argv;
		goto retry;
	}

	if(!stricmp(option, "-file")) {
		++argv;
		if(*argv) {
			cerr << "tonetool: -file: missing argument" << endl;
			exit(-1);
		}
		filename = *(argv++);
		goto retry;
	}

	if(!strnicmp(option, "-locale=", 8)) {
		locale = option + 8;
		++argv;
		goto retry;
	}

	if(!stricmp(option, "-locale")) {
		++argv;
		if(*argv) {
			cerr << "tonetool: -file: missing argument" << endl;
			exit(-1);
		}
		locale = *(argv++);
		goto retry;
	}


skip:
	if(*argv && **argv == '-') {
		printf("ERR!\n");
	        cerr << "tonetool: " << *argv << ": unknown option" << endl;
		exit(-1);
	}

	if(!*argv) {
		cerr << "tonetool: tone name missing" << endl;
		exit(-1);
	}

	if(!TelTone::load(filename)) {
		cerr << "tonetool: " << fname(filename) << ": unable to load" << endl;
		exit(-1);
	}

	while(*argv) {
		cout << *argv << ':';
		key = TelTone::find(*(argv++), locale);
		if(!key) {
			cout << " not found" << endl;
			continue;
		}
		cout << endl;
		def = key->first;
		while(def) {
			cout << "   ";
			if(def->f2 && def->f1)
				cout << def->f1 << "+" << def->f2;
			else
				cout << def->f1;

			if(def->duration)
				cout << " " << def->duration << "ms" << endl;
			else {
				cout << " forever" << endl;
				break;
			}

			if(def->silence)
				cout << "   silence " << def->silence << "ms" << endl;

			if(def == key->last) {
				if(def->next == key->first)
					cout << "   repeat full" << endl;
				else if(def->next)
					cout << "   repeat partial" << endl;
				break;
			}
			def = def->next;
		}
	}
	exit(0);
}


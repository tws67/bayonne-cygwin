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

void Tool::notation(char **argv)
{
	char *fn = *(argv++);
	char *ann = NULL;
	AudioFile file, tmp;
	Info info;
	char target[PATH_MAX];
	unsigned char buffer[4096];
	int rtn;

	if(!fn) {
		cerr << "audiotool: --notation: no file specified" << endl;
		exit(-1);
	}

	ann = *argv;

	file.open(fn, modeRead);
	if(!file.isOpen()) {
		cerr << "*** " << fn << ": cannot access or invalid" << endl;
		exit(-1);
	}
	file.getInfo(&info);
	if(info.annotation && !ann)
		cout << info.annotation << endl;
	if(!ann)
		exit(0);
	rewrite(fn, target, PATH_MAX);
	info.annotation = ann;
	setDelete(target);
	remove(target);
	tmp.create(target, &info);
	if(!tmp.isOpen()) {
		cerr << "*** " << target << ": unable to create" << endl;
		exit(-1);
	}
	for(;;)
	{
		rtn = file.getBuffer(buffer, sizeof(buffer));
		if(!rtn)
			break;
		if(rtn < 0) {
			cerr << "*** " << fn << ": read failed" << endl;
			remove(target);
			exit(-1);
		}
		rtn = tmp.putBuffer(buffer, rtn);
		if(rtn < 1) {
			cerr << "*** " << target << ": write failed" << endl;
			remove(target);
			exit(-1);
		}
	}
	file.close();
	tmp.close();
	rtn = rename(target, fn);
	remove(target);
	setDelete(NULL);
	if(rtn) {
		cerr << "*** " << fn << ": could not be replaced" << endl;
		exit(-1);
	}

	exit(0);
}

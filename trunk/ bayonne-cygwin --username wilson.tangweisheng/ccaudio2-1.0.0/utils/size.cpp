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

void Tool::size(char **argv)
{
	char *fn = *(argv++);
	AudioFile file;
	Info info;
	unsigned long pos;

	if(!fn) {
		cerr << "audiotool: --size: no file specified" << endl;
		exit(-1);
	}

	file.open(fn, modeRead);
	if(!file.isOpen()) {
		cerr << "*** " << fn << ": cannot access or invalid" << endl;
		exit(-1);
	}
	file.getInfo(&info);
	file.setPosition();
	pos = file.getPosition();
	pos /= info.rate;
	cout << pos << endl;
	exit(0);
}

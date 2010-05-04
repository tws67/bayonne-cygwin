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

void Tool::rewrite(const char *source, char *target, size_t max)
{
	char *fn;
	char buffer[PATH_MAX];

#ifdef	WIN32
	char *ext;
	snprintf(buffer, sizeof(buffer), "%s", source);
	while(NULL != (fn = strchr(buffer, '\\')))
		*fn = '/';

	fn = strrchr(buffer, '/');
	if(fn) {
		*(fn++) = 0;
		ext = strrchr(fn, '.');
		if(ext)
			*ext = 0;
		snprintf(target, max, "%s/%s.tmp", buffer, fn);
	}
	else {
		ext = strrchr(buffer, '.');
		if(ext)
			*ext = 0;
		snprintf(target, max, "%s.tmp", buffer);
	}
#else
	snprintf(buffer, sizeof(buffer), "%s", source);
	fn = strrchr(buffer, '/');
	if(fn) {
		*(fn++) = 0;
		snprintf(target, max, "%s/.tmp.%s", buffer, fn);
	}
	else
		snprintf(target, max, ".tmp.%s", source);
#endif
}


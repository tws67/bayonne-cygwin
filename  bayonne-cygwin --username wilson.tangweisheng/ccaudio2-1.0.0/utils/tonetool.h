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

#include "private.h"
#ifdef	W32
#undef	__EXPORT
#endif
#include "audio2.h"
#include <cmath>
#include <iostream>
#include <cstdio>

#ifndef	W32
#include <unistd.h>
#include <cstdlib>
#include <cstddef>
#include <csignal>
#include <sys/stat.h>
#endif

#ifndef	PATH_MAX
#define	PATH_MAX	256
#endif

using namespace std;
using namespace ost;

extern const char *delfile;

class Tool : public Audio
{
public:
	Tool();
	static bool isFile(const char *path);
	static bool canAccess(const char *path);
	static AudioTone *getTone(char **argv, Level l, timeout_t framing = 20, timeout_t interdigit = 60);
	static void write(char **argv, bool append);
	static void play(char **argv);
	static void detect(char **argv);
	static void list(char **argv);
};

extern class Tool tool;

inline void setDelete(const char *delname)
	{delfile = delname;}


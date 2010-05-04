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

#define __EXPORT __declspec(dllexport)

#define	HAVE_MODULES	1
#define	HAVE_ALLOCA_H	1

#define	_WIN32_WINNT	0x0400

#include <windows.h>
#include <malloc.h>
#include <cstring>

#define	snprintf	_snprintf
#define	vsnprintf	_vsnprintf

#define	HAVE_SNPRINTF

#if	defined(_M_PPC) || defined(_M_MPPC)
#define	__BYTE_ORDER	4321
#endif

#ifndef	__BYTE_ORDER	// may be overriden in .dsp target
#define	__BYTE_ORDER	1234
#endif

#define	__LITTLE_ENDIAN	1234
#define	__BIG_ENDIAN	4321

class ccAudio_Mutex_
{
private:
	CRITICAL_SECTION mutex;

public:
	ccAudio_Mutex_();
	~ccAudio_Mutex_();
	void enter(void);
	void leave(void);
};

#include "audio2.h"

// Copyright (C) 2000-2005 Open Source Telecom Corporation.
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
#include "audio2.h"

using namespace ost;

AudioBase::AudioBase()
{
	memset(&info, 0, sizeof(info));
}

AudioBase::AudioBase(Info *i)
{
	memcpy(&info, i, sizeof(info));
}

AudioBase::~AudioBase()
{
}

ssize_t AudioBase::putNative(Encoded data, size_t bytes)
{
	swapEncoded(info, data, bytes);
	return putBuffer(data, bytes);
}

ssize_t AudioBase::getNative(Encoded data, size_t bytes)
{
	ssize_t result = getBuffer(data, bytes);

	if(result < 1)
		return result;

	swapEncoded(info, data, result);
	return result;
}



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

AudioBuffer::AudioBuffer(Info *i, size_t sz) :
AudioBase(i)
{
	size = sz;
	buf = new char[size];
	start = len = 0;
	mutexObject = new ccAudio_Mutex_;
}

AudioBuffer::~AudioBuffer()
{
	ccAudio_Mutex_ *mutex = (ccAudio_Mutex_ *)mutexObject;

	if(mutexObject)
		delete mutex;

	if(buf)
		delete[] buf;
	buf = NULL;
	mutexObject = NULL;
}

void AudioBuffer::enter(void)
{
	ccAudio_Mutex_ *mutex = (ccAudio_Mutex_ *)mutexObject;

	mutex->enter();
}

void AudioBuffer::leave(void)
{
	ccAudio_Mutex_ *mutex = (ccAudio_Mutex_ *)mutexObject;

	mutex->leave();
}

ssize_t AudioBuffer::getBuffer(Encoded data, size_t amount)
{
	size_t left, copied;

	if(amount == 0)
		return 0;

	enter();
	if(len == 0) {
		memset(data, 0, amount);
		leave();
		return (ssize_t)amount;
	}

	if(amount > len)
		memset(data + len, 0, amount - len);

	left = ((amount) < (len) ? amount : len);
	if((start + left) > size) {
		copied = size - start;
		memcpy(data, buf + start, copied);
		data += copied;
		left -= copied;
		len -= copied;
		start = 0;
	}

	if(left > 0) {
		memcpy(data, buf + start, left);
		len -= left;
		start = (start + left) % size;
	}
	leave();
	return (ssize_t)amount;
}

ssize_t AudioBuffer::putBuffer(Encoded data, size_t amount)
{
	size_t nl, written, removed, offset;

	if(amount == 0)
		return 0;

	enter();
	nl = len + amount;
	if(len > size) {
		removed = nl - size;
		start = (start + removed) % size;
		len -= removed;
	}

	offset = (start + len) % size;
	if((offset + amount) > size) {
		written = size - offset;
		memcpy(buf + offset, data, written);
		offset = 0;
		data += written;
		amount -= written;
		len += written;
	}

	if(amount > 0) {
		memcpy(buf + offset, data, amount);
		len += amount;
	}
	leave();
	return (ssize_t)amount;
}





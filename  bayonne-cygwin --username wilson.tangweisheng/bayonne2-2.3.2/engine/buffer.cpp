// Copyright (C) 2005 Open Source Telecom Corp.
//  
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software 
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#include "engine.h"

using namespace ost;
using namespace std;

StreamingBuffer *StreamingBuffer::first = NULL;

StreamingBuffer::StreamingBuffer(const char *tag, timeout_t size, Rate r)
{
	id = tag;
	next = first;
	first = this;

	position = 0;
	count = (size * ((unsigned long)r)) / 1000;
	data = new Sample[count];
	memset(data, 0, sizeof(Sample) * count);
}

StreamingBuffer::~StreamingBuffer()
{
	cleanup();
}

void StreamingBuffer::cleanup(void)
{
	if(data)
	{
		delete[] data;
		data = NULL;
	}
}

StreamingBuffer *StreamingBuffer::get(const char *id, Rate rate)
{
	StreamingBuffer *node = first;
	while(node)
	{
		if(!stricmp(id, node->id) && rate == node->rate)
			return node;

		node = node->next;
	}
	return NULL;
}

bool StreamingBuffer::isActive(void)
{
	return true;
}

Audio::Linear StreamingBuffer::getBuffer(unsigned long *pos, timeout_t duration)
{
	unsigned long copy = ((duration * rate) / 1000);
	Linear target = &data[*pos];

	if(count % copy)
		return NULL;

	*pos += copy;
	return target;
}

unsigned long StreamingBuffer::getPosition(timeout_t framing)
{
	timeout_t past = 120;
	if(framing == 50)
		past = 150;
	return (position - ((past * rate) / 1000l)) % count;
}

Audio::Linear StreamingBuffer::putBuffer(timeout_t duration)
{
	unsigned long copy = (duration * rate) / 1000l;
	Linear target = &data[position];

	if(count % copy)
		return NULL;

	position += copy;
	return target;
}

void StreamingBuffer::clearBuffer(timeout_t duration)
{
    unsigned long copy = (duration * rate) / 1000l;
    Linear target = &data[position];

    if(count % copy)
        return;

    position += copy;
    while(copy--)
		*(target++) = 0;
}


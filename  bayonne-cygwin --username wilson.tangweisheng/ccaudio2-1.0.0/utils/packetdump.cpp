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

class PacketStream : public AudioStream
{
private:
	char **list;
	char *getContinuation(void);

public:
	PacketStream();
	void open(char **argv);
};

PacketStream::PacketStream() : AudioStream()
{
	list = NULL;
}

void PacketStream::open(char **argv)
{
	AudioStream::open(*(argv++), modeRead, 10);
	if(isOpen())
		list = argv;
}

char *PacketStream::getContinuation(void)
{
	if(!list)
		return NULL;

	return *(list++);
}

void Tool::packetdump(char **argv)
{
	PacketStream packetfile;
	const char *path = *argv;
	Encoded buffer;
	Info info;
	ssize_t count;

	packetfile.open(argv);

	if(!packetfile.isOpen()) {
		cerr << "audiotool: " << path << ": unable to access" << endl;
		exit(-1);
	}

	if(!packetfile.isStreamable()) {
		cerr << "audiotool: " << path << ": missing needed codec" << endl;
		exit(-1);
	}

	packetfile.getInfo(&info);

	buffer = new unsigned char[maxFramesize(info)];

	while((count = packetfile.getPacket(buffer)) > 0)
		cout << "-- " << count << endl;

	delete[] buffer;
	packetfile.close();
	exit(0);
}


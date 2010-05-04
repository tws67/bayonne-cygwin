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

class PlayStream : public AudioStream
{
private:
	char **list;
	char *getContinuation(void);

public:
	PlayStream();
	void open(char **argv);
};

PlayStream::PlayStream() : AudioStream()
{
	list = NULL;
}

void PlayStream::open(char **argv)
{
	AudioStream::open(*(argv++), modeRead, 10);
	if(isOpen())
		list = argv;
}

char *PlayStream::getContinuation(void)
{
	if(!list)
		return NULL;

	return *(list++);
}

void Tool::play(char **argv)
{
	AudioDevice *dev;
	PlayStream playfile;
	const char *path = *argv;
	Linear buffer;
	Info info;
	unsigned bufcount, pages;

	dev = getDevice();

	if(!hasDevice() && !dev) {
		cerr << "no device supported" << endl;
		exit(-1);
	}
	else if(!dev)
	{
		cerr << "device unavailable" << endl;
		exit(-1);
	}

	playfile.open(argv);

	if(!playfile.isOpen()) {
		cerr << "audiotool: " << path << ": unable to access" << endl;
		exit(-1);
	}

	if(!playfile.isStreamable()) {
		cerr << "audiotool: " << path << ": missing needed codec" << endl;
		exit(-1);
	}

	playfile.getInfo(&info);
	if(!dev->setAudio((Rate)info.rate, isStereo(info.encoding), 10)) {
		cerr << "audiotool: sound device does not support rate" << endl;
		exit(-1);
	}

	bufcount = playfile.getCount();
	if(isStereo(info.encoding))
		buffer = new Sample[bufcount * 2];
	else
		buffer = new Sample[bufcount];

	for(;;)
	{
		if(isStereo(info.encoding))
			pages = playfile.getStereo(buffer, 1);
		else
			pages = playfile.getMono(buffer, 1);

		if(!pages)
			break;

		dev->putSamples(buffer, bufcount);
	}

	dev->sync();
	delete dev;
	playfile.close();
	exit(0);
}


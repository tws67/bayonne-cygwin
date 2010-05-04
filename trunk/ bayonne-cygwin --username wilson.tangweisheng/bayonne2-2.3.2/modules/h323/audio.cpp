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
//
// As a special exception to the GNU General Public License, permission is
// granted for additional uses of the text contained in its release
// of Bayonne as noted here.
//
// This exception is that permission is hereby granted to link Bayonne 2
// with the OpenH323 and Pwlib libraries, and distribute the combination, 
// without applying the requirements of the GNU GPL to the OpenH323
// and pwd libraries as long as you do follow the requirements of the 
// GNU GPL for all the rest of the software thus combined.
//
// This exception does not however invalidate any other reasons why
// the resulting executable file might be covered by the GNU General
// public license or invalidate the licensing requirements of any
// other component or library.

#include "driver.h"

namespace h323driver {
using namespace ost;
using namespace std;

Channel::Channel(Session *parent) : 
Mutex()
{
	session = parent;
	receive = NULL;
	transmit = NULL;
	tcodec = rcodec = NULL;
	tbuf = rbuf = NULL;
	tsize = rsize = 0;
	rcount = 0;

	//dtmf = new DTMFDetect();
	dtmf = NULL;

	if(Driver::h323.getBool("inbanddtmf"))
		dtmf = new DTMFDetect();
}

Channel::~Channel()
{
//	if(receive)
//		delete receive;
//	if(transmit)
//		delete transmit;
	if(tbuf)
		delete[] tbuf;
	if(rbuf)
		delete[] rbuf;
	if(dtmf)
		delete dtmf;
}

BOOL Channel::hasTransmit(void)
{ 
	if(transmit)
        	return TRUE;
	return FALSE;
}

BOOL Channel::hasReceive(void)
{
	if(receive)
		return TRUE;
	return FALSE;
}

BOOL Channel::IsOpen()
{
	return TRUE;
}

void Channel::attachTransmitter(AudioBase *channel, AudioCodec *c)
{
	enter();
	tcodec = c;
	transmit = channel;
	leave();
}

void Channel::attachReceiver(AudioBase *channel, AudioCodec *c, timeout_t duration)
{
	enter();
	rcodec = c;
	receive = channel;
	rcount = 0;
	if(duration)
		rcount = duration * 8;
	leave();
}

void Channel::stop(int channelType)
{
	Event event;
	bool stopped = false;

	enter();
	if(channelType == AUDIO_TRANSMIT && transmit)
	{
		stopped = true;
//		delete transmit;
		transmit = NULL;
		tcodec = NULL;
	}
	else if(channelType == AUDIO_RECEIVE && receive)
	{
		stopped = true;
//		delete receive;
		receive = NULL;
		rcodec = NULL;
		rcount = 0;
	}
	else
	{
		if(receive)
		{
			stopped = true;
//			delete receive;
			receive = NULL;
			rcodec = NULL;
			rcount = 0;
		}
		if(transmit)
		{
			stopped = true;
//			delete transmit;
			transmit = NULL;
			tcodec = NULL;
		}
	}

	if(stopped)
	{
		memset(&event, 0, sizeof(event));
		event.id = AUDIO_IDLE;
		session->postEvent(&event);
	}
	leave();
}

BOOL Channel::Write(const void *buf, PINDEX len)
{
	Event event;
	static unsigned lastDtmf = 0;
	int dtmfFound = 0;
	unsigned i, size;
	char digits[128] = {0};

	enter();

	/*if(dtmf)
	{
		lastDtmf += len;
		dtmfFound = dtmf->putSamples((int16_t*)buf, len/2);
		if(dtmfFound && lastDtmf > 1000)
		{
			dtmf->getResult(digits, 64);
			if(digits)
			{
				for(i = 0; i < strlen(digits); i++)
				{
					event.id = DTMF_KEYUP;
					event.parm.dtmf.digit = (int)(digits[i] - '0');
					event.parm.dtmf.duration = 60;
					trunk->postEvent(&event);
				}
				lastDtmf = 0;
			}
		}
	}*/

	if(!receive && session->peer)
		session->peer->peerAudio((Encoded)buf);

	if(receive && rcodec)
	{
		Info info = rcodec->getInfo();
		size = toBytes(info, len / 2);
		if(size != rsize)
                {
                        if(rbuf)
                                delete[] rbuf;
                        rbuf = new unsigned char[size];
                        rsize = size;
                } 
		rcodec->encode((Linear)buf, rbuf, len / 2);
		receive->putBuffer(rbuf, size);
		if(rcount && rcount <= (len / 2))
			stop(AUDIO_RECEIVE);
		else if(rcount)
			rcount -= (len / 2);
	}
	if(receive)
		receive->putBuffer((Encoded)buf, len);
	leave();

	lastWriteCount = len;
	writeDelay.Delay(lastWriteCount/16);

	return TRUE;
}

BOOL Channel::Read(void *buf, PINDEX len)
{
	int count = 0;
	unsigned size;
	struct _frame *frame;

	enter();
	if(session->peer)
	{
		session->enter();
		frame = session->frame_first;
		if(frame)
		{
			if(!transmit)
			{
				memcpy(buf, frame->data, len);
				count = len;
			}
			session->frame_first = frame->next;
			delete[] frame;
		}
	}
	if(transmit && tcodec)
	{
		Info info = tcodec->getInfo();
		size = toBytes(info, len / 2);
		if(size != tsize)
		{
			if(tbuf)
				delete[] tbuf;
			tbuf = new unsigned char[size];
			tsize = size;
		}
		count = transmit->getBuffer(tbuf, size);
		if(count < size)
		{
			stop(AUDIO_TRANSMIT);
			goto skip;
		}
		count = len;
		tcodec->decode((Linear)buf, tbuf, len / 2);
	}
	else if(transmit)
	{
		count = transmit->getBuffer((Encoded)buf, len);
		if(count < 1)
			stop(AUDIO_TRANSMIT);
	}
skip:
	leave();

	if(count < len)
	{
		memset((char*)buf+count, 0, len - count);
	}

	lastReadCount = len;
	readDelay.Delay(lastReadCount/16);

	return TRUE;
}

BOOL Channel::Close()
{
	closed = true;
	//if(trunk->isOpen())
	//	trunk->close();
	PChannel::Close();
	return true;
}


} // namespace

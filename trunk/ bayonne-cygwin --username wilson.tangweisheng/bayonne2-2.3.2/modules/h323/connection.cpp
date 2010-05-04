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

Connection::Connection(Endpoint &ep, unsigned ref, Session *s) :
H323Connection(ep, ref)
{
	session = s;
	channel = NULL;
	endpoint = &ep;

	isForwarding = transmitUp = receiveUp = false;
}

Connection::~Connection()
{
	if(channel)
	{
		delete channel;
		channel = NULL;
	}
}

BOOL Connection::OnIncomingCall(const H323SignalPDU & setupPDU, H323SignalPDU &)
{
	const H225_Setup_UUIE & setup = setupPDU.m_h323_uu_pdu.m_h323_message_body;
	const H225_ArrayOf_AliasAddress & addr = setup.m_destinationAddress;
	PString dnid("");
	int i;

	for(i = 0; i < addr.GetSize(); ++i)
		dnid = H323GetAliasAddressString(addr[i]);

	if(setupPDU.m_h323_uu_pdu.HasOptionalField(H225_H323_UU_PDU::e_nonStandardData))        
	{
		PString param = setupPDU.m_h323_uu_pdu.m_nonStandardData.m_data.AsString();                     
		if(!param)
			slog.debug() << "h323: non-standard param in setup PDU: " << param << 
		endl;
      	}

	session->enter();
	session->setConst("session.dialed", dnid);
	session->setConst("session.caller", GetControlChannel().GetRemoteAddress());
	session->setConst("session.display", GetRemotePartyName());
	session->leave();
	callToken = GetCallToken();
	return true;
}

H323Connection::AnswerCallResponse Connection::OnAnswerCall(const PString & callerName, const H323SignalPDU &setupPDU, H323SignalPDU &)
{
	Event event;
	
	memset(&event, 0, sizeof(event));
	event.id = CALL_OFFERED;
	event.data = (void *)(const unsigned char *)callToken;
	session->postEvent(&event);
	return H323Connection::AnswerCallPending;
}

void Connection::OnEstablished()
{
	Event event;

	memset(&event, 0, sizeof(event));
	event.id = CALL_ANSWERED;
	session->postEvent(&event);
}

void Connection::OnCleared()
{
	Event event;

	if(GetCallEndReason() != H323Connection::EndedByCallForwarded)
	{
		memset(&event, 0, sizeof(event));
		event.id = STOP_DISCONNECT;
		session->postEvent(&event);
	}
}

BOOL Connection::OpenAudioChannel(BOOL isEncoding, unsigned bufferSize, H323AudioCodec & codec)
{
	BOOL rtn;

	if(isEncoding)
		outCodec = &codec;
	else
		inCodec = &codec;

	if(!channel)
		channel = new Channel(session);

	rtn = codec.AttachChannel(channel, FALSE);
	return TRUE;
}

BOOL Connection::OnSendSignalSetup(H323SignalPDU & setupPDU)
{
        //H225_Setup_UUIE &setup = setupPDU.m_h323_uu_pdu.m_h323_message_body;
        //H225_ArrayOf_AliasAddress &addr = setup.m_sourceAddress;

	setupPDU.m_h323_uu_pdu.IncludeOptionalField(H225_H323_UU_PDU::e_nonStandardData);
        setupPDU.m_h323_uu_pdu.m_nonStandardData.m_nonStandardIdentifier.SetTag(H225_NonStandardIdentifier::e_h221NonStandard);
        endpoint->SetH221NonStandardInfo(setupPDU.m_h323_uu_pdu.m_nonStandardData.m_nonStandardIdentifier);
        return TRUE;
}

BOOL Connection::OnStartLogicalChannel(H323Channel & channel)
{
	switch(channel.GetDirection())
	{
        case H323Channel::IsTransmitter:
                transmitUp = true;
                break;
        case H323Channel::IsReceiver:
                receiveUp = true;
                break;
        default:
                break;
        }

	return TRUE;
}

void Connection::OnUserInputString(const PString & value)
{
        Event event;

        const char *digit = (const char *)value;
        int i;

//        if(value.Left(3) == "MSG")
//        {
//                trunk->setSymbol(SYM_NOTIFYTEXT, value.Mid(3));
//                trunk->setSymbol(SYM_NOTIFYTYPE, "user");
//                event.id = TRUNK_CALL_INFO;
//                if(trunk->postEvent(&event))
//                        return;
//        }

        for(i = 0; i < value.GetLength(); i++)
        {
                switch(digit[i])
                {
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
			memset(&event, 0, sizeof(event));
                        event.id = DTMF_KEYUP;
                        event.dtmf.digit = (int)(digit[i] - '0');
                        event.dtmf.duration = 60;
                        session->postEvent(&event);
                        break;
                case '*':
			memset(&event, 0, sizeof(event));
                        event.id = DTMF_KEYUP;
                        event.dtmf.digit = 10;
                        event.dtmf.duration = 60;
                        session->postEvent(&event);
                        break;
                case '#':
                        memset(&event, 0, sizeof(event));
                        event.id = DTMF_KEYUP;      
                        event.dtmf.digit = 11;
                        event.dtmf.duration = 60;
                        session->postEvent(&event);
                        break;
		}
	}
}


void Connection::OnUserInputTone(char tone, unsigned duration, unsigned channel, unsigned rtpts) 
{
        OnUserInputString(tone);
}

void Connection::CleanUpOnCallEnd()
{
	Event event;

	H323Connection::CleanUpOnCallEnd();

//	memset(&event, 0, sizeof(event));
//	event.id = CALL_CLEARED;
//	session->queEvent(&event);
}

void Connection::muteTransmit(BOOL mute)
{
	H323Channel *channel = FindChannel(RTP_Session::DefaultAudioSessionID, FALSE);  
        if(channel != NULL)
                channel->SetPause(mute);
}

void Connection::muteReceive(BOOL mute)
{
	H323Channel *channel = FindChannel(RTP_Session::DefaultAudioSessionID, TRUE);   
        if(channel != NULL)
                channel->SetPause(mute);
}

void Connection::setForward(void)
{
	isForwarding = true;
}

} // namespace

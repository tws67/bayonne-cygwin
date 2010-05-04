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
//
// This class implements Citron's NAT technology, allowing a NAT'ed endpoint to
// connect to a public GNUGK gatekeeper from behind a NAT gateway that has no
// special configuration relating to H.323.
//
// This code comes from Chih-Wei Huang <cwhuang@citron.com.tw>, and was taken
// from GnomeMeeting.

#include "driver.h"

namespace h323driver {
using namespace ost;
using namespace std;

Gatekeeper::Gatekeeper(H323EndPoint &ep, H323Transport *trans) : H323Gatekeeper(ep, trans)
{
	isMakeRequestCalled = false;
	detectorThread = 0;
	incomingTCP = outgoingTCP = 0;
	gkport = 0;
}

Gatekeeper::~Gatekeeper()
{
	StopDetecting();
}

BOOL Gatekeeper::MakeRequest(Request &request)
{
	if(request.requestPDU.GetAuthenticators().IsEmpty())
		request.requestPDU.SetAuthenticators(authenticators);

	if(isMakeRequestCalled)
	{
		isMakeRequestCalled = false;
		return H225_RAS::MakeRequest(request);
	}

	PWaitAndSignal lock(requestMutex);
	while(!H225_RAS::MakeRequest(request))
	{
		if(request.responseResult != Request::NoResponseReceived && request.responseResult != Request::TryAlternate)
			return false;

		PINDEX alt, altsize = alternates.GetSize();
		if(altsize == 0)
			return FALSE;

		H323TransportAddress tempAddr = transport->GetRemoteAddress();
		PString tempIdentifier = gatekeeperIdentifier;
		StopDetecting();
		PSortedList<AlternateInfo> altGKs = alternates;

		for(alt = 0; alt < altsize; ++alt)
		{
			AlternateInfo *altInfo = &altGKs[alt];
			transport->SetRemoteAddress(altInfo->rasAddress);
			transport->Connect();

			gatekeeperIdentifier = altInfo->gatekeeperIdentifier;

			H323RasPDU pdu;
			Request req(SetupGatekeeperRequest(pdu), pdu);

			if(H225_RAS::MakeRequest(req))
			{
				endpointIdentifier = "";
				isMakeRequestCalled = true;

				if(!RegistrationRequest(autoReregister))
					continue;

				if(request.requestPDU.GetChoice().GetTag() == H323RasPDU::e_admissionRequest)
				{
					H225_AdmissionRequest & arq = dynamic_cast<H225_RasMessage&>
						(request.requestPDU.GetChoice());
					arq.m_gatekeeperIdentifier = gatekeeperIdentifier;
					arq.m_endpointIdentifier = endpointIdentifier;
				}
				else if(request.requestPDU.GetChoice().GetTag() == H323RasPDU::e_registrationRequest)
					return TRUE;
				break;
			}
		}
		
		if(alt >= altsize)
		{
			// switch failed, use primary
			transport->SetRemoteAddress(tempAddr);
			transport->Connect();
			gatekeeperIdentifier = tempIdentifier;
			return FALSE;
		}
	}

	return TRUE;
}

BOOL Gatekeeper::OnReceiveRegistrationConfirm(const H225_RegistrationConfirm &rcf)
{
	if(rcf.HasOptionalField(H225_RegistrationConfirm::e_nonStandardData))
		if(rcf.m_nonStandardData.m_data.AsString().Find("NAT=") == 0)
		{
			PWaitAndSignal lock(threadMutex);

			// Connection is NAT'ed
			if(!detectorThread)
			{
				if(rcf.m_callSignalAddress.GetSize() > 0)
				{
					const H225_TransportAddress &addr = rcf.m_callSignalAddress[0];
					if(addr.GetTag() == H225_TransportAddress::e_ipAddress)
					{
						const H225_TransportAddress_ipAddress & ip = addr;
						gkip = PIPSocket::Address(ip.m_ip[0], ip.m_ip[1],
							ip.m_ip[2], ip.m_ip[3]);
						gkport = ip.m_port;
					}
				}
				detectorThread = new DetectIncomingCallThread(this);
			}
		}

	return H323Gatekeeper::OnReceiveRegistrationConfirm(rcf);
}

BOOL Gatekeeper::OnReceiveUnregistrationRequest(const H225_UnregistrationRequest &urq)
{
	StopDetecting();
	return H323Gatekeeper::OnReceiveUnregistrationRequest(urq);
}

void Gatekeeper::OnSendRegistrationRequest(H225_RegistrationRequest & rrq)
{
	H323Gatekeeper::OnSendRegistrationRequest(rrq);
}

void Gatekeeper::OnSendUnregistrationRequest(H225_UnregistrationRequest &urq)
{
	StopDetecting();
	H323Gatekeeper::OnSendUnregistrationRequest(urq);
}

void Gatekeeper::DetectIncomingCall()
{
	const H323ListenerList & listeners = endpoint.GetListeners();

	if(listeners.IsEmpty())
		return;

	H323TransportAddress localAddr = listeners[0].GetTransportAddress();

	isDetecting = true;
	while(isDetecting)
	{
		if(!gkport)
		{
			H323TransportAddress gksig;

			if(!LocationRequest(endpoint.GetAliasNames(), gksig))
			{
				PProcess::Current().Sleep(60 * 1000);
				continue;
			}
			gksig.GetIpAndPort(gkip, gkport);
		}

		socketMutex.Wait();
		incomingTCP = new PTCPSocket(gkport);
		socketMutex.Signal();

		if(!incomingTCP->Connect(gkip))
		{
			socketMutex.Wait();
			delete incomingTCP;
			incomingTCP = 0;
			socketMutex.Signal();
			PProcess::Current().Sleep(60 * 1000);
			continue;
		}

		while(incomingTCP->IsOpen())
		{
			SendInfo(Q931::CallState_IncomingCallProceeding);
			PSocket::SelectList rlist;
			rlist.Append(incomingTCP);
			if((PSocket::Select(rlist, 86400 * 1000) == PSocket::NoError) && !rlist.IsEmpty())
			{
				// incoming call detected
				PIPSocket::Address ip;
				WORD port;
				localAddr.GetIpAndPort(ip, port);
				socketMutex.Wait();
				outgoingTCP = new PTCPSocket(port);
				socketMutex.Signal();

				if(!outgoingTCP->Connect(ip))
					break;

				DoForwarding(incomingTCP, outgoingTCP);
				break;
			}
		}

		PWaitAndSignal socketLock(socketMutex);
		delete incomingTCP;
		incomingTCP = 0;
		delete outgoingTCP;
		outgoingTCP = 0;
	}
}

void Gatekeeper::StopDetecting()
{
	PWaitAndSignal lock(threadMutex);
	if(detectorThread)
	{
		isDetecting = false;
		socketMutex.Wait();
		if(incomingTCP)
		{
			if(outgoingTCP)
				outgoingTCP->Close();
			else
				SendInfo(Q931::CallState_DisconnectRequest);
			incomingTCP->Close();
		}

		socketMutex.Signal();
		detectorThread->Terminate();
		detectorThread->WaitForTermination();
		delete detectorThread;
		detectorThread = 0;
	}
}

bool Gatekeeper::SendInfo(int state)
{
	Q931 information;
	information.BuildInformation(0, false);
	PBYTEArray buf, epid(endpointIdentifier, endpointIdentifier.GetLength(), false);
	information.SetIE(Q931::FacilityIE, epid);
	information.SetCallState(Q931::CallStates(state));
	information.Encode(buf);
	return SendTPKT(incomingTCP, buf);
}

bool Gatekeeper::SendTPKT(PTCPSocket *sender, const PBYTEArray &buf)
{
	WORD len = buf.GetSize();
	WORD tlen = len + 4;
	PBYTEArray tbuf(tlen);
	BYTE *bptr = tbuf.GetPointer();

	bptr[0] = 3;
	bptr[1] = 0;

	*(reinterpret_cast<WORD *>(bptr + 2)) = PIPSocket::Host2Net(WORD(len + 4));
	memcpy(bptr + 4, buf, len);

	return sender->Write(bptr, tlen);
}

bool Gatekeeper::ForwardMesg(PTCPSocket *receiver, PTCPSocket *sender)
{
	BYTE tpkt[4];
	int bSize;

	if(!receiver->ReadBlock(tpkt, sizeof(tpkt)))
		return false;

	if(tpkt[0] != 3)
		return true;

	bSize = ((tpkt[2] << 8) | tpkt[3]) - 4;
	PBYTEArray buffer(bSize);

	if(!receiver->ReadBlock(buffer.GetPointer(), bSize))
		return false;
	
	return SendTPKT(sender, buffer);
}

void Gatekeeper::DoForwarding(PTCPSocket *incomingTCP, PTCPSocket *outgoingTCP)
{
	while(incomingTCP->IsOpen() && outgoingTCP->IsOpen())
	{
		switch(PSocket::Select(*incomingTCP, *outgoingTCP))
		{
		case -3:
			// data available on both
			if(!ForwardMesg(outgoingTCP, incomingTCP))
				return;
		case -1:
			ForwardMesg(incomingTCP, outgoingTCP);
			break;
		case -2:
			ForwardMesg(outgoingTCP, incomingTCP);
			break;
		default:
			break;
		}
	}
}

} // namespace

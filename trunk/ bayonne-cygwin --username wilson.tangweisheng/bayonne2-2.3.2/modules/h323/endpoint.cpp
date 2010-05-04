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

Endpoint::Endpoint() : H323EndPoint()
{
	listener = NULL;
	gatekeeper = NULL;

	terminalType = e_GatewayOnly;
}
        
Endpoint::~Endpoint()
{
	if(listener)
	{
		RemoveListener(listener);
		ClearAllCalls(H323Connection::EndedByLocalUser, true);
		delete listener;
	}
}

BOOL Endpoint::Initialise()
{
	PString UserName = Driver::h323.getLast("username");
	int usegk = atoi(Driver::h323.getLast("usegk"));
	unsigned rtpmin = atoi(Driver::h323.getLast("rtpmin"));
	unsigned rtpmax = atoi(Driver::h323.getLast("rtpmax"));
	unsigned tcpmin = atoi(Driver::h323.getLast("tcpmin"));
	unsigned tcpmax = atoi(Driver::h323.getLast("tcpmax"));
	unsigned udpmin = atoi(Driver::h323.getLast("udpmin"));
	unsigned udpmax = atoi(Driver::h323.getLast("udpmax"));

	SetLocalUserName(UserName);

	SetCapabilities();

	if(!Driver::h323.getBool("faststart"))
		DisableFastStart(true);
	if(!Driver::h323.getBool("h245tunneling"))
		DisableH245Tunneling(true);
	if(!Driver::h323.getBool("h245insetup"))
		DisableH245inSetup(true);

	if(rtpmin && rtpmin <= rtpmax)
		SetRtpIpPorts(rtpmin, rtpmax);
	if(tcpmin && tcpmin <= tcpmax)
		SetTCPPorts(tcpmin, tcpmax);
	if(udpmin && udpmin <= udpmax)
		SetUDPPorts(udpmin, udpmax);

	slog.info() << "Waiting for incoming calls for \"" << GetLocalUserName() << '"' << endl;

	if(usegk)
	{
		const char *gkpw = Driver::h323.getLast("secret");
		if(!gkpw)
			gkpw = server->getLast("secret");

		const char *gkaddr = Driver::h323.getLast("server");
		if(!gkaddr)
			gkaddr = server->getLast("secret");
		if(!gkaddr || !*gkaddr)
			gkaddr = server->getLast("proxy");
		if(!gkaddr || !*gkaddr)
			gkaddr = "localhost";

		const char *gkid = Driver::h323.getLast("userid");
		if(!gkid)
			gkid = server->getLast("userid");

		bool rtn;

		if(gkpw && strlen(gkpw))
		{
			slog(Slog::levelInfo) << "oh323: H.235 security enabled for gatekeeper" << endl;
			SetGatekeeperPassword(gkpw);
		}

		if(!UseGatekeeper(gkaddr, gkid))
		{
			slog.error() << "h323: gatekeeper registration failed: " << endl;;
			gatekeeper = GetGatekeeper();
			if(gatekeeper)
			{
				switch(gatekeeper->GetRegistrationFailReason())
				{
				case H323Gatekeeper::DuplicateAlias:
					slog(Slog::levelError) << "\tduplicate alias" << endl;
					break;
				case H323Gatekeeper::SecurityDenied:
					slog(Slog::levelError) << "\tnot authorised to connect" << endl;
					break;
				case H323Gatekeeper::TransportError:
					slog(Slog::levelError) << "\ttransport error" << endl;
					break;
				default:
					slog(Slog::levelError) << "\tnot found" << endl;
					break;
				}
			}
			else
				slog(Slog::levelError) << "\tunknown error" << endl;
		}
		else
		{
			gatekeeper = GetGatekeeper();
			if(gatekeeper)
				slog.info() << "h323: successfully registered with " << 
					gatekeeper->GetName() << endl;
		}
	}

	return true;
}

H323Gatekeeper * Endpoint::CreateGatekeeper(H323Transport *transport)
{
	return new Gatekeeper(*this, transport);
}

H323Connection * Endpoint::CreateConnection(unsigned callRef, void *userData,
						 H323Transport *transport, H323SignalPDU *setupPDU)
{
	unsigned i;
	Session *session = (Session *)userData;

	if(session)
		return (H323Connection *)session->attachConnection(callRef);

	session = (Session *)Driver::h323.getIdle();
	if(session)
	{
		session->setState(STATE_RING);
		return (H323Connection*)session->attachConnection(callRef);
	}

	return NULL;
}

bool Endpoint::MakeCall(const PString & dest, PString & token, unsigned int *callReference,
				H323Capability *cap, char *callerId, void *userData)
{
	PString finalDest = dest;

	if(GetGatekeeper() == NULL && strrchr(finalDest, ':') == NULL)
	{
		finalDest += psprintf(":%i", H323EndPoint::DefaultTcpPort);
	}

	if(callerId)
		SetLocalUserName(PString(callerId));

	SetCapabilities();;

	H323EndPoint::MakeCall(finalDest, token, userData);
	return true;
}

BOOL Endpoint::OnConnectionForwarded(H323Connection & connection, const PString &forwardParty,
					  const H323SignalPDU &)
{
	Connection & conn = (Connection &)connection;
	PString currentToken = connection.GetCallToken();
	PString newToken;

	if(MakeCall(forwardParty, newToken, NULL, NULL, NULL, (void*)conn.session))
	{
		conn.setForward();
		return TRUE;
	}
	else
		return FALSE;
}
	
void Endpoint::OnConnectionCleared(H323Connection & connection, const PString & token) 
{
	Event event;
	Session *session =  ((Connection &)connection).session;

//	memset(&event, 0, sizeof(event));
//	event.id = CALL_CLEARED;
//	if(session)
//		session->queEvent(&event);
}

void Endpoint::OnConnectionEstablished(H323Connection & connection, const PString & token)
{
	Session *session = ((Connection &)connection).session;
	Event event;

	if(!session)
		return;	

	event.id = CALL_CONNECTED;
	session->postEvent(&event);
}

void Endpoint::SetCapabilities(void)
{
	const char *cp = Driver::h323.getLast("uimode");

#ifndef	H323_CODECS_DISABLED
	H323Capability *gsm;
	H323Capability *msgsm;
#endif
	H323Capability *ulaw;
	H323Capability *alaw;

#ifndef	H323_CODECS_DISABLED
	SetCapability(0, 0, gsm = new H323_GSM0610Capability);
	SetCapability(0, 0, msgsm = new MicrosoftGSMAudioCapability);
#endif
	SetCapability(0, 0, ulaw = new H323_G711Capability(H323_G711Capability::muLaw, H323_G711Capability::At64k));
	SetCapability(0, 0, alaw = new H323_G711Capability(H323_G711Capability::ALaw, H323_G711Capability::At64k));

#ifndef	H323_CODECS_DISABLED
	gsm->SetTxFramesInPacket(4);
	msgsm->SetTxFramesInPacket(4);
#endif
	ulaw->SetTxFramesInPacket(30);
	alaw->SetTxFramesInPacket(30);

#ifndef	H323_CODECS_DISABLED
	SetCapability(0, 0, new SpeexNarrow2AudioCapability());
	SetCapability(0, 0, new SpeexNarrow3AudioCapability());
	SetCapability(0, 0, new SpeexNarrow4AudioCapability());
	SetCapability(0, 0, new SpeexNarrow5AudioCapability());
	SetCapability(0, 0, new SpeexNarrow6AudioCapability());

	SetCapability(0, 0, new H323_G726_Capability(*this, H323_G726_Capability::e_16k));
	SetCapability(0, 0, new H323_G726_Capability(*this, H323_G726_Capability::e_24k));
	SetCapability(0, 0, new H323_G726_Capability(*this, H323_G726_Capability::e_32k));
	SetCapability(0, 0, new H323_G726_Capability(*this, H323_G726_Capability::e_40k));


	SetCapability(0, 0, new H323_LPC10Capability(*this));
#endif

	AddAllUserInputCapabilities(0, 1);
	AddAllUserInputCapabilities(0, 2);

#ifdef	H323_CODECS_DISABLED
	AddAllCapabilities(0, 0, "*");
#endif

        if(!stricmp(cp, "q931"))
                SetSendUserInputMode(H323Connection::SendUserInputAsQ931);
        else if(!stricmp(cp, "signal"))
                SetSendUserInputMode(H323Connection::SendUserInputAsTone);
        else if(!stricmp(cp, "rfc2833"))
                SetSendUserInputMode(H323Connection::SendUserInputAsInlineRFC2833);
        else
                SetSendUserInputMode(H323Connection::SendUserInputAsString);

	SetAudioJitterDelay(GetMinAudioJitterDelay(), GetMinAudioJitterDelay());
}

void Endpoint::SetEndpointTypeInfo(H225_EndpointType & info) const
{
	const char *pfxcfg = Driver::h323.getLast("prefixes");
	char *prefixes[65];
	char *opt;
	char *cp;
	char *sp;
	int pcount = 0;

	H323EndPoint::SetEndpointTypeInfo(info);

	info.m_gateway.IncludeOptionalField(H225_GatewayInfo::e_protocol);
	info.m_gateway.m_protocol.SetSize(1);
	H225_SupportedProtocols &protocol = info.m_gateway.m_protocol[0];
	protocol.SetTag(H225_SupportedProtocols::e_voice);

	if(pfxcfg)
	{
		opt = strdup(pfxcfg);
		cp = strtok_r(opt, " ,;\t\n", &sp);
		while(cp)
		{
			prefixes[pcount++] = cp;
			cp = strtok_r(NULL, " ,;\t\n", &sp);
		}
		((H225_VoiceCaps &)protocol).m_supportedPrefixes.SetSize(pcount);
		while(pcount--)
			H323SetAliasAddress(PString(prefixes[pcount]),
				((H225_VoiceCaps &)protocol).m_supportedPrefixes[pcount].m_prefix);
		free(opt);
	}
}

void Endpoint::SetVendorIdentifierInfo(H225_VendorIdentifier & info) const
{
	H323EndPoint::SetVendorIdentifierInfo(info);

	info.IncludeOptionalField(H225_VendorIdentifier::e_productId);
	info.m_productId = PString("GNU Bayonne");
	info.IncludeOptionalField(H225_VendorIdentifier::e_versionId);
	info.m_productId = "2.0.0";
}

void Endpoint::SetH221NonStandardInfo(H225_H221NonStandard & info) const
{
	H323EndPoint::SetH221NonStandardInfo(info);
}


} // namespace

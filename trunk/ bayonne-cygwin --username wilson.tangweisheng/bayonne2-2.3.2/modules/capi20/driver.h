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

#include "bayonne.h"
#include <cc++/slog.h>
#include <capi20.h>

namespace capidriver {
using namespace ost;
using namespace std;

struct capi_profile {
        uint16 ncontroller;      /* number of installed controller */
        uint16 nbchannel;        /* number of B-Channels */
        uint32 goptions;         /* global options */
        uint32 support1;         /* B1 protocols support */
        uint32 support2;         /* B2 protocols support */
        uint32 support3;         /* B3 protocols support */
        uint32 reserved[6];      /* reserved */
        uint32 manu[5];          /* manufacturer specific information */
};

class Driver : public BayonneDriver, public Thread
{
protected:
	struct capi_profile cprofile;
	unsigned appl_id;
        unsigned msg_id;
	unsigned contr_count;

	void run(void);

public:
	static Driver capi;	// plugin activation

	Driver();

	void startDriver(void);
	void stopDriver(void);
};

// in this driver we really only have one instance of session since we
// only use one timeslot, but the coding style is more reflective of
// drivers with multiport
class Session : public BayonneSession, public TimerPort
{
protected:

public:
	Session(BayonneSpan *span, timeslot_t ts);
	~Session();

	// core timer virtuals all port session objects need to define
	timeout_t getRemaining(void);
	void startTimer(timeout_t timer);
	void stopTimer(void);
};

} // namespace


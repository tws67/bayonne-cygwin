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

#include "driver.h"

namespace vpbdriver {
using namespace ost;
using namespace std;

Callerid::Callerid(ScriptInterp *interp, int h) :
ScriptThread(interp, 1)
{
	handle = h;
}

void Callerid::run(void)
{
	Thread::sleep(1500);
	setCancel(cancelImmediate);
	memset(cidbuf, 0, sizeof(cidbuf));
	vpb_record_buf_start(handle, VPB_LINEAR);
	vpb_record_buf_sync(handle, (char *)cidbuf, sizeof(cidbuf));
	vpb_record_buf_finish(handle);
	strcpy(cidnbr, "pstn/");
	if(VPB_OK != vpb_cid_decode(cidnbr, cidbuf, CID_BUF_SZ))
	{
		lock();
		interp->setConst("session.caller", "unknown");
		slog.debug("%s: caller unknown", interp->getLogname());
		release();
		exit("callerid-failed");
	}
	lock();
	interp->setConst("session.caller", cidnbr);
	release();
	exit(NULL);
}

} // namespace

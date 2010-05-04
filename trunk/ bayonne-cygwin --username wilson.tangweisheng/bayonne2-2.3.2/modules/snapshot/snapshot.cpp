// Copyright (C) 2005 Open Source Telecom Corporation.
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

#include <bayonne.h>
#include <cc++/slog.h>
#include <cc++/socket.h>

namespace moduleSnapshot {
using namespace ost;
using namespace std;

class Snapshot : public BayonneService, public TimerPort, public Bayonne
{
private:
	void run(void);
	void stopService(void);
	char tmpfile[256];
	char runfile[256];

public:
	Snapshot();
};

static Snapshot snapshot;

Snapshot::Snapshot() :
BayonneService(0, 0), TimerPort()
{
	const char *runpath = server->getLast("runfiles");

	snprintf(runfile, sizeof(runfile), "%s/bayonne.info", runpath);
	snprintf(tmpfile, sizeof(tmpfile), "%s/.bayonne.info", runpath);
}

void Snapshot::run(void)
{
	FILE *fp;
	time_t dt;
	struct tm *now, ex;

	slog.debug("snapshot service started");

	for(;;)
	{
		remove(tmpfile);
		fp = fopen(tmpfile, "w");
		if(!fp)
			Thread::sync();
		
		time(&dt);
		now = localtime_r(&dt, &ex); 
		if(now->tm_year < 1900)
			now->tm_year += 1900;

		fprintf(fp, "Server Snapshot: %04d-%02d-%02d %02d:%02d\n",
			now->tm_year, now->tm_mon + 1, now->tm_mday,
			now->tm_hour, now->tm_min);

		fprintf(fp, "Total Timeslots: %d\n", 
			getTimeslotCount());
		fprintf(fp, "Used Timeslots:  %d\n",
			getTimeslotsUsed());
		fprintf(fp, "Active Calls:    %d\n",
			idle_limit - idle_count);

		if(Bayonne::status)
			fprintf(fp, "Status Record:   %s\n", Bayonne::status); 

		fclose(fp);
		rename(tmpfile, runfile);
		incTimer(60000);
		Thread::sleep(getTimer());
	}
}			

void Snapshot::stopService(void)
{
	terminate();
	remove(tmpfile);
}

} // namespace

	

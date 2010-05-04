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

namespace scdriver {
using namespace ost;
using namespace std;

Keyboard::Keyboard(timeslot_t timeslot) :
Thread(0, 2048)
{
#ifndef	WIN32
	struct termios n;
        tcgetattr(0, &t);
	        tcgetattr(0, &n);
        n.c_lflag &= ~(ECHO|ICANON);
	n.c_cc[VMIN] = 0;
	n.c_cc[VTIME] = 1;
        tcsetattr(0, TCSANOW, &n);
#endif
	ts = timeslot;
}

Keyboard::~Keyboard()
{
	terminate();
#ifndef	WIN32
	tcsetattr(0, TCSANOW, &t);
#endif
}

void Keyboard::run(void)
{
	char ch;
	Event event;
	BayonneSession *session = getSession(ts);

	setCancel(cancelImmediate);
	waitLoaded();

	for(;;)
	{
#ifdef	WIN32
		ch = _getch();
#else
		ch = 0;
		read(0, &ch, 1);
		Thread::yield();
#endif
		memset(&event, 0, sizeof(event));
		switch(ch)
		{
		case 'w':
		case 'W':
			event.id = LINE_WINK;
			session->queEvent(&event);
			break;
		case 'h':
		case 'H':
			event.id = LINE_DISCONNECT;
			session->queEvent(&event);
			break;
		case 'r':
		case 'R':
			event.id = RING_ON;
			session->queEvent(&event);
			event.id = RING_OFF;
			session->queEvent(&event);
			break;
		case 'x':
		case 'X':
			event.id = TIMER_EXPIRED;
			session->queEvent(&event);
			break;
		case ' ':
			event.id = NULL_EVENT;
			session->queEvent(&event);
			break;
		case 'q':
		case 'Q':
		case 3:
		case 27:
			slog.debug("keyboard: exiting...");
#ifdef	WIN32
			ExitProcess(0);
#else
			raise(SIGTERM);
#endif
			break;
		case 0:
			break;
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
                        event.dtmf.digit = ch - '0';
dtmf:
			event.id = DTMF_KEYUP;
                        event.dtmf.duration = 60;
			session->queEvent(&event);
                        Thread::sleep(10);
                        break;
		case '*':
			event.dtmf.digit = 10;
			goto dtmf;
		case '#':
		case '\r':
		case '\n':
			event.dtmf.digit = 11;
			goto dtmf;
		default:
			slog.debug("keyboard: unknown key %02x", ch);
		}
	}
}

}

%/* Copyright (C) 2005 Open Source Telecom Corporation.
% *
% *
% * This program is free software; you can redistribute it and/or
% * modify it under the terms of the GNU Library General Public
% * License version 2 as published by the Free Software Foundation.
% * 
% * This library is distributed in the hope that it will be useful,
% * but WITHOUT ANY WARRANTY; without even the implied warranty of
% * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
% * Library General Public License for more details.
% * 
% * You should have received a copy of the GNU Library General Public License
% * along with this library; see the file COPYING.LIB.  If not, write to
% * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
% * Boston, MA 02111-1307, USA.                                                 
% */

#ifdef	RPC_HDR
%#pragma pack(1)
#endif

const BAYONNE_PORT_CALLER_SZ = 32;
const BAYONNE_PORT_DIALED_SZ = 32; 
const BAYONNE_PORT_DISPLAY_SZ = 64;    
const BAYONNE_PORT_DURATION_SZ = 16;
const BAYONNE_PORT_LOGNAME_SZ = 16;
const BAYONNE_PORT_SID_SZ = 16;
const BAYONNE_NODE_SERVER_SZ = 12;
const BAYONNE_NODE_VERSION_SZ = 10;
const BAYONNE_SESSION_SZ = 32;
const BAYONNE_START_SCRIPT_SZ = 32;
const BAYONNE_START_NUMBER_SZ = 128;
const BAYONNE_START_CALLER_SZ = 32;
const BAYONNE_START_DISPLAY_SZ = 64;

struct bayonne_status
{
	int node_uptime;
	int node_active;
	int node_count;
	string node_server<BAYONNE_NODE_SERVER_SZ>;
	string node_version<BAYONNE_NODE_VERSION_SZ>;
};

struct bayonne_port
{
	string port_cid<BAYONNE_PORT_SID_SZ>;
	string port_pid<BAYONNE_PORT_SID_SZ>;
	string port_logname<BAYONNE_PORT_LOGNAME_SZ>;
	string port_caller<BAYONNE_PORT_CALLER_SZ>;
	string port_dialed<BAYONNE_PORT_DIALED_SZ>;
	string port_display<BAYONNE_PORT_DISPLAY_SZ>;
	string port_duration<BAYONNE_PORT_DURATION_SZ>;
};

struct bayonne_start
{
	string start_script<BAYONNE_START_SCRIPT_SZ>;
	string start_number<BAYONNE_START_NUMBER_SZ>;
	string start_caller<BAYONNE_START_CALLER_SZ>;
	string start_display<BAYONNE_START_DISPLAY_SZ>;
};

enum bayonne_error
{
	BAYONNE_SUCCESS = 0,
	BAYONNE_FAILURE,
	BAYONNE_INVALID_VALUES,
	BAYONNE_INVALID_COMMAND,
	BAYONNE_INVALID_PORT,
	BAYONNE_INVALID_SESSION,
	BAYONNE_INVALID_DRIVER,
	BAYONNE_INVALID_SCRIPT,
	BAYONNE_BUSY
};

struct bayonne_result
{
	bayonne_error result_code;
	string result_id<BAYONNE_SESSION_SZ>;
};

struct bayonne_session
{
	string session_id<BAYONNE_SESSION_SZ>;
};

#ifdef	RPC_HDR
%#pragma pack()
#endif

program BAYONNE_PROGRAM
{
	version BAYONNE_VERSION
	{
		bayonne_error BAYONNE_RELOAD(void) = 1;
		bayonne_error BAYONNE_SHUTDOWN(void) = 2;
		bayonne_status BAYONNE_STATUS(void) = 3;
		bayonne_result BAYONNE_START(bayonne_start) = 4;
		bayonne_error BAYONNE_STOP(bayonne_session) = 6;
	} = 2;
} = 0x29000001;

	

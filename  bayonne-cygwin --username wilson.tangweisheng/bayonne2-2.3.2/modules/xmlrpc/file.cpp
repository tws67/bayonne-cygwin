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

#include <engine.h>
#include <cc++/slog.h>
#include <cc++/socket.h>

namespace moduleXMLRPC {
using namespace ost;
using namespace std;

typedef	Bayonne::rpcint_t rpcint_t;
typedef	rpcint_t rpcbool_t;

static bool isValid(const char *path)
{
	if(!path)
		return false;

	if(!*path)
		return false;

	if(*path == '/' || *path == '\\' || *path == '.')
		return false;

	if(strstr("..", path))
		return false;

#ifdef	WIN32
	if(path[1] == ':')
		return false;
#endif

	if(!strchr(path, '/'))
		return false;

	if(path[strlen(path) - 1] == '/')
		return false;

	if(isDir(path))
		return false;

	return true;
}

static void file_version(BayonneRPC *rpc)
{
    if(rpc->getCount())
    {
        rpc->sendFault(3, "Invalid parameters");
		return;
	}

	rpc->buildResponse("(ii)", 
		"current", (rpcint_t)1,
		"prior", (rpcint_t)0);
}

static void file_test(BayonneRPC *rpc)
{
	const char *path;

	if(!rpc->transport.authorized)
	{
		rpc->transportFault(401, "Not Authorized");
		return;
	}

    if(rpc->getCount() != 1)
    {
        rpc->sendFault(3, "Invalid parameters");
        return;
    }

	path = rpc->getIndexed(1);
	if(!isValid(path))
	{
		rpc->sendFault(4, "Invalid parameter value");
		return;
	}

	rpc->buildResponse("b", (rpcbool_t)isFile(path));
}

static void file_erase(BayonneRPC *rpc)
{
	const char *path;
	bool result = false;

	if(!rpc->transport.authorized)
	{
		rpc->transportFault(401, "Not Authorized");
		return;
	}

    if(rpc->getCount() != 1)
    {
        rpc->sendFault(3, "Invalid parameters");
        return;
    }

	path = rpc->getIndexed(1);
	if(!isValid(path))
	{
		rpc->sendFault(4, "Invalid parameter value");
		return;
	}

	if(!remove(path))
		result = true;

	rpc->buildResponse("b", (rpcbool_t)result);
}

static void file_write(BayonneRPC *rpc)
{
	const char *text;
	const char *path;
	const char *ext;
	char tpath[128];
	unsigned char buf[512];
	bool result = false;
	AudioFile af, tmp;
	Audio::Info info;
	FILE *fp;
	int len;

	if(!rpc->transport.authorized)
	{
		rpc->transportFault(401, "Not Authorized");
		return;
	}

    if(rpc->getCount() != 2)
    {
        rpc->sendFault(3, "Invalid parameters");
        return;
    }

	path = rpc->getIndexed(1);
	text = rpc->getIndexed(2);

	if(!isValid(path) || !text)
	{
		rpc->sendFault(4, "Invalid parameter value");
		return;
	}

	ext = strrchr(path, '.');
	if(!ext)
		goto append;

	if(!stricmp(ext, ".wav") || !stricmp(ext, ".au"))
		goto annotate;

	if(Audio::getEncoding(ext) != Audio::unknownEncoding)
	{
		rpc->sendFault(8, "Cannot write raw audio");
		return;
	}

append:
	fp = fopen(path, "a");
	if(!fp)
		goto finish;
	
	result = true;
	fprintf(fp, "%s\n", text);
	fclose(fp);
	goto finish;

annotate:
	af.open(path);
	if(!af.isOpen())
		goto finish;	

	af.getInfo(&info);
	snprintf(tpath, sizeof(tpath), "%s.tmp-%d%s", 
		path, (int)getThread()->Thread::getId(), ext);
	info.annotation = (char *)text;
	tmp.create(tpath, &info);
	if(!tmp.isOpen())
	{
		af.close();
		rpc->sendFault(7, "Cannot create annotation");
		return;
	}

	for(;;)
	{	
		len = af.getBuffer(buf, sizeof(buf));
		if(len < 1)
			break;
		tmp.putBuffer(buf, len);
	}
	tmp.close();
	if(rename(tpath, path))
		remove(tpath);
	else
		result = true;

finish:
	rpc->buildResponse("b", (rpcbool_t)result);
}

static void file_info(BayonneRPC *rpc)
{
	const char *path, *ext;
	const char *type = "unknown";
	const char *mime = "unknown";
	const char *encoding = "unknown";
	const char *note = NULL;
	char duration[65];
	rpcint_t secs = 0;
	AudioFile af;
	Audio::Info ai;
	struct stat ino;

	setString(duration, sizeof(duration), "0");

	if(!rpc->transport.authorized)
	{
		rpc->transportFault(401, "Not Authorized");
		return;
	}

    if(rpc->getCount() != 1)
    {
        rpc->sendFault(3, "Invalid parameters");
        return;
    }

	path = rpc->getIndexed(1);
	if(!isValid(path))
	{
		rpc->sendFault(4, "Invalid parameter value");
		return;
	}

	if(stat(path, &ino))
	{
		rpc->sendFault(7, "File Missing");
		return;
	}

	ext = strrchr(path, '.');	
	if(ext && !stricmp(ext, ".au"))
		goto audio;
	else if(ext && !stricmp(ext, ".wav"))
		goto audio;
	else if(ext && Audio::getEncoding(ext) != Audio::unknownEncoding)
		goto audio;
	else if(ext && !stricmp(ext, ".txt"))
		goto text;
	else if(ext && !stricmp(ext, ".text"))
		goto text;
	else if(ext && !stricmp(ext, ".csv"))
		goto text;
	goto finish;

text:
	type = "text";
	mime = "text/plain";
	encoding = "ascii";
	goto finish;

audio:
	type = "audio";
	af.open(path);

	if(!af.isOpen())
		goto finish;

	encoding = Audio::getName(af.getEncoding());
	note = af.getAnnotation();
	af.setPosition();
	af.getPosition(duration, sizeof(duration));
	secs = (rpcint_t)(af.getPosition() / af.getSampleRate());
	af.getInfo(&ai);
	mime = Audio::getMIME(ai);

	af.close();	
	goto finish;

finish:
	rpc->buildResponse("(sssiitis)", 
		"type", type,
		"mime", mime,
		"encoding", encoding,
		"duration", duration,
		"duration_int", secs,
		"size", (rpcint_t)(ino.st_size / 1024),
		"modified", ino.st_mtime,
		"modified_int", (rpcint_t)ino.st_mtime,
		"note", note);
}

static void file_move(BayonneRPC *rpc)
{
	const char *path1, *path2;
	bool result = false;

	if(!rpc->transport.authorized)
	{
		rpc->transportFault(401, "Not Authorized");
		return;
	}

    if(rpc->getCount() != 2)
    {
        rpc->sendFault(3, "Invalid parameters");
        return;
    }

	path1 = rpc->getIndexed(1);
	path2 = rpc->getIndexed(2);
	if(!isValid(path1) || !isValid(path2))
	{
		rpc->sendFault(4, "Invalid parameter value");
		return;
	}

	if(!rename(path1, path2))
		result = true;

	rpc->buildResponse("b", (rpcbool_t)result);
}

static void file_copy(BayonneRPC *rpc)
{
	const char *path1, *path2;
	bool result = false;
	FILE *f1 = NULL, *f2 = NULL;
	const char *ext1, *ext2;
	char buf[512];
	size_t len;

	if(!rpc->transport.authorized)
	{
		rpc->transportFault(401, "Not Authorized");
		return;
	}

    if(rpc->getCount() != 2)
    {
        rpc->sendFault(3, "Invalid parameters");
        return;
    }

	path1 = rpc->getIndexed(1);
	path2 = rpc->getIndexed(2);
	if(!isValid(path1) || !isValid(path2))
	{
		rpc->sendFault(4, "Invalid parameter value");
		return;
	}

	ext1 = strrchr(path1, '.');
	ext2 = strrchr(path2, '.');
	if(!ext1 || !ext2 || stricmp(ext1, ext2))
	{
		rpc->sendFault(8, "File type mismatch");
		return;
	}

	f1 = fopen(path1, "rb");
	if(!f1)
		goto finish;

	remove(path2);
	f2 = fopen(path2, "wb");
	if(!f2)
		goto finish;

	result = true;
	while(!feof(f1))
	{
		len = fread(buf, 1, sizeof(buf), f1);
		if(len)
			fwrite(buf, len, 1, f2);
	}

finish:
	if(f1)
		fclose(f1);
	if(f2)
		fclose(f2);
	rpc->buildResponse("b", (rpcbool_t)result);
}

static Bayonne::RPCDefine dispatch[] = {
	{"version", file_version,
		"API version level of file rpc", "struct"},
	{"test", file_test,
		"Test for specified file path", "boolean, string"},
	{"erase", file_erase,
		"Remove specified file path", "boolean, string"},
	{"copy", file_copy,
		"Copy file to new path", "boolean, string, string"},
	{"move", file_move,
		"Move file to new path", "boolean, string"},
	{"info", file_info,
		"File type information", "struct, string"},
	{"write", file_write,
		"Write text into file", "boolean, string, string"},
	{NULL, NULL, NULL, NULL}};

static Bayonne::RPCNode xmlrpc_file("file", dispatch);

}; // namespace

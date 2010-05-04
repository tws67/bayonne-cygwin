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

#include "server.h"

namespace server {
using namespace ost;
using namespace std;

WriteThread::WriteThread(ScriptInterp *interp, const char *name) :
ScriptThread(interp, 0)
{
	unsigned len;
	strcpy(path, name);

	fp = NULL;
	text[0] = 0;
	
	while(NULL != (cp = interp->getValue(NULL)))
		addString(text, sizeof(text) - 3, cp);

	len = strlen(text);
	if(len && text[len - 1] != '\n')
		addString(text, sizeof(text), "\n");
}

WriteThread::~WriteThread()
{
	terminate();

	if(fp)
		fclose(fp);
}	

void WriteThread::run(void)
{
	fp = fopen(path, "a");

	if(fp)
	{
		fputs(text, fp);
		exit(NULL);
	}
	else
		exit("write-failed");
}

ListThread::ListThread(ScriptInterp *interp, const char *path, Symbol *var) :
ScriptThread(interp, 0), Dir(path) 
{
	sym = var;
	prefix = interp->getKeyword("match");
	suffix = interp->getKeyword("extension");
	prior = interp->getKeyword("prior");
        after = interp->getKeyword("after");
        save = interp->getKeysymbol("after");

	if(after)
		setString(last, sizeof(last), after);
	else
		last[0] = 0;

	snprintf(filepath, sizeof(filepath), "%s/", path);
	fplen = strlen(filepath); 

	longinfo = false;

	if(suffix && !stricmp(suffix, ".au"))
		longinfo = true;

	if(suffix && !stricmp(suffix, ".wav"))
		longinfo = true;
		
	if(longinfo)
		switch(sym->type)
		{
		case symARRAY:
		case symFIFO:
		case symSTACK:
			break;
		default:
			longinfo = false;
		}
}

ListThread::~ListThread()
{
	terminate();

	if(longinfo && af.isOpen())
		af.close();

	Dir::close();
}

void ListThread::run(void)
{
	const char *name;
	unsigned len, slen = 0;
	char buf[256];
	const char *note;

	if(suffix)
		slen = strlen(suffix);

	while(NULL != (name = Dir::getName()))
	{
		if(*name == '.')
			continue;

		if(prefix && strnicmp(name, prefix, strlen(prefix)))
			continue;

		if(slen)
		{
			len = strlen(name);
			if(len < slen)
				continue;
			len -= slen;
			if(stricmp(name + len, suffix))
				continue;
		}

		note = NULL;
				if(prior)
					if(stricmp(name, prior) > 0)
						continue;

                if(after)
                        if(stricmp(name, after) <= 0)
                                continue;

		if(longinfo)
		{
			setString(filepath + fplen, sizeof(filepath) - fplen, name);
			af.open(filepath);
			note = af.getAnnotation();
		}

		interp->enter();
		if(note)
		{
			snprintf(buf, sizeof(buf), "id=%s;%s", name, note);
			name = buf;
		}
		else if(longinfo)
			snprintf(buf, sizeof(buf), "id=%s", name);
	
		interp->append(sym, name);
		interp->leave();

                if(stricmp(name, last) > 0)
                        setString(last, sizeof(last), name);

		if(longinfo)
			af.close();			
	}

        if(save)
        {
                interp->enter();
                commit(save, last);
                interp->leave();
        }

	exit(NULL);
}

BuildThread::BuildThread(ScriptInterp *interp, BayonneAudio *au, Audio::Info *inf, const char *d, const char **list) :
ScriptThread(interp, 0)
{
	memcpy(&to, inf, sizeof(to));
	in = au;
	buffer = NULL;
	lbuffer = NULL;
	paths = list;
	completed = false;
	dest = setString(destname, sizeof(destname), d);

#ifdef	AUDIO_RATE_RESAMPLER
	resampler = NULL;
	resample = NULL;
#endif
}

BuildThread::~BuildThread()
{
	terminate();
	if(in)
		in->cleanup();

	strcpy(in->var_position, "00:00:00.000");
	if(out.isOpen())
	{
		if(!completed)
			remove(pathname);
		else
			out.getPosition(in->var_position, 12);	
		out.close();
	}

	if(buffer)
		delete[] buffer;

	if(lbuffer)
		delete[] lbuffer;

#ifdef	AUDIO_RATE_RESAMPLER
	if(resampler)
		delete resampler;

	if(resample)
		delete[] resample;

	resampler = NULL;
	resample = NULL;
#endif

	in = NULL;
	buffer = NULL;
	lbuffer = NULL;
}

void BuildThread::run(void)
{
	const char *fn = dest;

	remove(fn);
	out.create(fn, &to, false, to.framing);
	if(!out.isOpen())
		exit("unable-to-create");

	Thread::yield();

	out.getInfo(&to);

	in->play(paths, modeReadAny);
	if(!in->isOpen())
		exit("unable-to-read");

	in->getInfo(&from);
	Thread::yield();

	if(to.encoding == from.encoding && to.rate == from.rate)
		copyDirect();
	else
	{
		if(!in->isStreamable() || !out.isStreamable())
		{
			remove(pathname);
			exit("unable-to-convert");
		}
		copyConvert();
	}
	exit(NULL);
}	

void BuildThread::copyDirect(void)
{
	unsigned bufsize;
	bool endian = false;
	ssize_t status = 1;

	if(to.order && from.order && to.order != from.order)
		endian = true;

	bufsize = from.framesize;
	buffer = new unsigned char[bufsize];

	while(status > 0)
	{
		Thread::yield();
		status = in->getNative(buffer, bufsize);
		if(status < 1)
			break;

		Thread::yield();
		status = out.putNative(buffer, status);
	}
	completed = true;
}

void BuildThread::copyConvert(void)
{
	unsigned long samples, copied;
	unsigned pages, npages;
	bool mono = true;
	Linear source;

	if(isStereo(from.encoding) || isStereo(to.encoding))
		mono = false;

	samples = in->getCount();

	if(mono)
		lbuffer = new Sample[samples];
	else
		lbuffer = new Sample[samples * 2];

	source = lbuffer;
#ifdef	AUDIO_RATE_RESAMPLER
	if(from.rate != to.rate)
	{
		resampler = new AudioResample((Rate)from.rate, (Rate)to.rate);
		resample = new Sample[resampler->estimate(samples)];
		source = resample;
	}
#endif

	for(;;)
	{
		Thread::yield();
		if(mono)
			pages = in->getMono(lbuffer, 1);
		else
			pages = in->getStereo(lbuffer, 1);

		if(!pages)
			break;

		Thread::yield();

		copied = samples;
#ifdef	AUDIO_RATE_RESAMPLER
		if(resampler)
			copied = resampler->process(lbuffer, resample, samples);
#endif

		if(mono)
			npages = out.bufMono(source, copied);
		else
			npages = out.bufStereo(source, copied);
	}
	completed = true;
}

CopyThread::CopyThread(ScriptInterp *interp, const char *from, const char *to) :
ScriptThread(interp, 0)
{
	src = setString(fn1, sizeof(fn1), from);
	dest = setString(fn2, sizeof(fn2), to);
	in = NULL;
	out = NULL;
}

CopyThread::~CopyThread()
{
	terminate();
	if(in)
		fclose(in);
	if(out)
	{
		fclose(out);
		remove(dest);
	}
	in = out = NULL;
}

void CopyThread::run(void)
{
	unsigned len;

	in = fopen(src, "rb");
	if(!in)
		exit("source-invalid");
	
	remove(dest);
	out = fopen(dest, "wb");
	if(!out)
	{
		fclose(in);
		in = NULL;
		exit("target-invalid");
	}

	while(!feof(in))
	{
		len = fread(buf, 1, sizeof(buf), in);
		if(len < 1)
			break;
		fwrite(buf, len, 1, out);
		Thread::yield();
	}

	fclose(in);
	fclose(out);
	in = out = NULL;
	exit(NULL);
}	

} // end namespace

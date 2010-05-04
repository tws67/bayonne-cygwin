// Copyright (C) 2005 David Sugar, Tycho Softworks
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
using System;
using System.Collections;
using System.IO;
using System.Text;

namespace Bayonne 
{
	public class Libexec 
	{
		protected string _tsid = 
			Environment.GetEnvironmentVariable("PORT_TSESSION");
		protected string _home = 
			Environment.GetEnvironmentVariable("SERVER_PREFIX");
		protected string _ramfs =
			Environment.GetEnvironmentVariable("SERVER_TMPFS");
		protected string _tmpfs =
			Environment.GetEnvironmentVariable("SERVER_TMP");

		protected string _query = "";
		protected string _digits = "";
		protected string _position = "";
		protected int _exitcode = 0;
		protected int _resultcode = 0;
		protected int _reply = 0;
		protected string _voice = null;
		protected short _tonelevel = 0;

		private Hashtable _head = new Hashtable();
		private Hashtable _args = new Hashtable();

		protected TextReader _in = Console.In;
		protected TextWriter _out = Console.Out;
		protected TextWriter _log = Console.Out;

		static public string getVersion()
		{
			return "4.0";
		}

		public Libexec()
		{
			string lbuf, keyword, value;
			int num;

			if(_tsid == null) 
			{
				_exitcode = 3;
				return;
			}

			_log = Console.Error;
			_out.WriteLine(_tsid + " HEAD");

			while(_exitcode < 1)
			{
				lbuf = _in.ReadLine();

				if((lbuf == null) || (lbuf.Length < 4))
				{
					_exitcode = 3;
					break;
				}
				try 
				{
					num = int.Parse(lbuf.Substring(0, 3));
				}
				catch(Exception)
				{
					num = 0;
				}
				if(num > 900)
					_exitcode = num - 900;
				if(num > 0)
				{
					_reply = num;
					continue;
				}
				num = lbuf.IndexOf(':');
				keyword = lbuf.Substring(0, num);
				value = lbuf.Substring(num + 2);
				_head.Add(keyword, value);
			}
			_out.WriteLine(_tsid + " ARGS");
			while(_exitcode < 1)
			{
				lbuf = _in.ReadLine();

				if((lbuf == null) || (lbuf.Length < 4))
				{
					_exitcode = 3;
					break;
				}
				try 
				{
					num = int.Parse(lbuf.Substring(0, 3));
				}
				catch(Exception)
				{
					num = 0;
				}
				if(num > 900)
					_exitcode = num - 900;
				if(num > 0)
				{
					_reply = num;
					continue;
				}
				num = lbuf.IndexOf(':');
				keyword = lbuf.Substring(0, num);
				value = lbuf.Substring(num + 2);
				_args.Add(keyword, value);
			}
					
		}

		public void echo(string text)
		{
			_log.WriteLine(text);
		}

		public string getPosition() 
		{
			return _position;
		}

		public int lastResult()
		{
			return _resultcode;
		}

		public int getExit()
		{
			return _exitcode;
		}

		public string getHead(string key)
		{
			return (string)_head[key];
		}

		public string getArg(string key)
		{
			return (string)_args[key];
		}

		public bool isLive()
		{
			if(_tsid == null || _exitcode > 0)
				return false;
			return true;
		}

		public void postSymbol(string id, string value)
		{
			string sid = getHead("SESSION");
			_out.WriteLine(sid + " POST " + id + " " + value);			
		}

		public string getPath(string file)
		{
			string ext = getHead("EXTENSION");
			string pre = getHead("PREFIX");
			int epos, spos;

			if(ext == null || ext.Length < 1)
				ext = ".au";

			if(pre != null && pre.Length < 1)
				pre = null;

			if(file == null)
				return null;

			if(file.StartsWith("/") || file.StartsWith("."))
				return null;

			if(file.Substring(1, 1) == ":")
				return null;

			if(file.IndexOf("..") > 0 || file.IndexOf("/.") > 0)
				return null;

			spos = file.LastIndexOf('/');
			epos = file.LastIndexOf('.');
			if(epos < spos)
				epos = 0;

			if(epos < 1)
				file = file + ext;

			if(file.StartsWith("tmp:"))
				return _tmpfs + "/" + file.Substring(4);

			if(file.StartsWith("ram:"))
				return _ramfs + "/" + file.Substring(4);

			if(file.IndexOf(':') > 0)
				return null;

			if((file.IndexOf('/') < 1) && (pre != null))
				return _home + "/" + pre + "/" + file;

			if(file.IndexOf('/') < 1)
				return null;

			return _home + "/" + file;
		}		

		public void hangup()
		{
			if(_tsid == null || _exitcode > 0)
				return;

			_out.WriteLine(_tsid + " HANGUP");
			_tsid = null;
		}

		public void error(string msg)
		{
			if(_tsid == null || _exitcode > 0)
				return;

			_out.WriteLine(_tsid + " ERROR " + msg);
			_tsid = null;
		}

		public void detach(int result)
		{
			if(_tsid == null || _exitcode > 0)
				return;

			_out.WriteLine(_tsid + " EXIT " + result);
			_tsid = null;
		}

		protected int command(string text)
		{
			string lbuf, keyword, value;
			int num = 0;
	
			_resultcode = 255;
			_query = "";

			if(_tsid == null || _exitcode > 0)
				return 255;

			_out.WriteLine(_tsid + " " + text);
			while(_exitcode < 1)
			{
                                lbuf = _in.ReadLine();

                                if((lbuf == null) || (lbuf.Length < 4))
                                {
                                        _exitcode = 3;  
                                        break;
                                }  
				try
				{
					num = int.Parse(lbuf.Substring(0, 3));
				}
				catch(Exception)
				{
					num = 0;
				}
				if(num > 900)
					_exitcode = num - 900;
				if(num > 0) 
                                {
                                        _reply = num; 
                                        continue;
                                }
                                num = lbuf.IndexOf(':');
                                keyword = lbuf.Substring(0, num);
                                value = lbuf.Substring(num + 2);
				switch(_reply)
				{
				case 100:
					if(keyword == "RESULT")
					{
						_resultcode = int.Parse(value);
						break;
					}
					if(keyword == "DIGITS")
					{
						_digits = value;
						break;
					}
					if(keyword == "POSITION")
					{
						_position = value;
						break;
					}
					break;
				case 200:
					_resultcode = 0;
					_query = value;
					break;
				} 
			}
			return _resultcode;
		}

		public int replayFile(string file)
		{
			return command("REPLAY " + file);
		}

		public int replayOffset(string file, string offset)
		{
			return command("REPLAY " + file + " " + offset);
		}

		public int recordFile(string file, long total, long silence)
		{
			return command("RECORD " + file + " " + total + " " + silence);
		}

		public int recordOffset(string file, string offset, long total, long silence)
		{
			return command("RECORD " + file + " " + total + " " + silence + " " + offset);
		}

		public int eraseFile(string fn)
		{
			fn = getPath(fn);
			if(fn == null)
			{
				_resultcode = 254;
				return 254;
			}
			File.Delete(fn);
			_resultcode = 0;
			return 0;
		}

                public int moveFile(string fn1, string fn2)
                {
                        fn1 = getPath(fn1);
			fn2 = getPath(fn2);
                        if(fn1 == null || fn2 == null)
                        {
                                _resultcode = 254;               
                                return 254;
                        }
                        File.Move(fn1, fn2);
                        _resultcode = 0;
                        return 0;
                } 

		public bool setVoice(string voice)
		{
			_voice = voice;
			return true;
		}

		public void setLevel(short level)
		{
			_tonelevel = level;
		}

		public int clearInput()
		{
			return command("FLUSH");
		}

		public char readKey(long timeout)
		{
			if(_tsid == null || _exitcode > 0)
				return (char)0;
			_digits = "";
			if(command("READ " + timeout) > 0)
				return (char)0;
			if(_digits.Length < 1)
				return (char)0;
			return _digits[0];
		}                                                                        

		public string readInput(short count, long timeout)
		{
			if(_tsid == null || _exitcode > 0)
				return "";

			_digits = "";
			if(command("READ " + timeout + " " + count) > 0)
				return "";

			return _digits;
		}

		public bool waitInput(long timeout)
		{
			if(_tsid == null || _exitcode > 0)
				return false;

			_digits = "";
			if(command("WAIT " + timeout) > 0)
				return false;

			if(_digits.Length > 0)
				return true;

			return false;
		}

		public int sendResult(string text)
		{
			return command("RESULT " + text);
		}

		public int xferCall(string dest)
		{
			return command("XFER " + dest);
		}

		public int speak(string text)
		{
			string voice = "PROMPT";
			if(_voice != null)
				voice = _voice;

			return command(voice + " " + text);
		}

		public int playTone(string tone, long timeout)
		{
			return command("TONE " + tone + " " + timeout + " " + _tonelevel);
		}

		public int playSingleTone(short tone, long timeout)
		{
			return command("STONE " + tone + " " + timeout + " " + _tonelevel);
		}

		public int playDualTone(short tone1, short tone2, long timeout)
		{
			return command("DTONE " + tone1 + " " + tone2 + " " + timeout + " " + _tonelevel);
		}
	
		public int sizeSymbol(string sym, int count)
		{
			return command("NEW " + sym + " " + count);
		}

                public int setSymbol(string sym, string value)
                {
                        return command("SET " + sym + " " + value);
                }

                public int addSymbol(string sym, string value)
                {
                        return command("ADD " + sym + " " + value);
                }

		public string getSymbol(string sym)
		{
			if(_tsid == null || _exitcode > 0)
				return null;
			_query = "";
			_reply = 0;
			command("GET " + sym);
			if(_reply != 200)
				return null;
			return _query;
		}                      
	}
}	

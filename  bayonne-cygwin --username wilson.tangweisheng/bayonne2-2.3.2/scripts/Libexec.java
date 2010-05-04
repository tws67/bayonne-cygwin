// Copyright (C) 2005 David Sugar, Tycho Softworks.
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
// This and other java classes related to the Bayonne server will be 
// collected together into the bayonne.jar file.  The .jar is not
// rebuilt by default, but rather is redistributed for convenience of
// those users who may not have a jdk installed.  If you wish to rebuild 
// bayonne.jar, just run "make java" from this directory.

package org.gnutelephony.bayonne;
import java.io.*;
import java.util.*;

public class Libexec
{
	protected String _tsid = System.getProperty("bayonne.tsid");
	protected String _home = System.getProperty("bayonne.home");
	protected String _ramfs = System.getProperty("bayonne.ramfs");
	protected String _tmpfs = System.getProperty("bayonne.tmpfs");
	protected String _query = "";
	protected String _digits = "";
	protected String _position = "00:00:00.000";
	protected int _exitcode = 0;
	protected int _resultcode = 0;
	protected int _reply = 0;
	private String _voice = null;
	private short _tonelevel = 0; 
	private Map _head = new HashMap();
	private Map _args = new HashMap();
	private BufferedReader _in = new BufferedReader(new InputStreamReader(System.in));

	static public String getVersion() {
		return "4.0";
	}

	public Libexec() {
		String lbuf, keyword, value;
		int num;

		if(_tsid == null) {
			_exitcode = 3;
			return;
		}
		System.out.println(_tsid + " HEAD");
		while(_exitcode < 1) {
			try {
				lbuf = _in.readLine();
			}
			catch(IOException e) {
				lbuf = null;
			}
			if(lbuf == null) {
				 _exitcode = 3;
				break;
			}
			if(lbuf.length() < 4) {
				break;
			}
			try {
				num = Integer.parseInt(lbuf.substring(0, 3));
			}
			catch (NumberFormatException e) {
				num = 0;
			}
			if(num > 900) {
				_exitcode = num - 900;
			}
			if(num > 0) {
				_reply = num;
				continue;
			}
			num = lbuf.indexOf(':');
			keyword = lbuf.substring(0, num);
			num += 2;
			value = lbuf.substring(num);
			_head.put(keyword, value);			
		}	
		System.out.println(_tsid + " ARGS");
		while(_exitcode < 1) {
			try {
				lbuf = _in.readLine();
			}
			catch(IOException e) {
				lbuf = null;
			}
			if(lbuf == null) {
				 _exitcode = 3;
				break;
			}
			if(lbuf.length() < 4) {
				break;
			}
			try {
				num = Integer.parseInt(lbuf.substring(0, 3));
			}
			catch (NumberFormatException e) {
				num = 0;
			}
			if(num > 900) {
				_exitcode = num - 900;
			}
			if(num > 0) {
				_reply = num;
				continue;
			}
			num = lbuf.indexOf(':');
			keyword = lbuf.substring(0, num);
			num += 2;
			value = lbuf.substring(num);
			_args.put(keyword, value);			
		}	
	}

	public void postSymbol(String id, String value) {
		String sid = getHead("SESSION");
		System.out.println(sid + " POST " + id + " " + value);
	}		

	public String getPosition() {
		return _position;
	}

	public int lastResult() {
		return _resultcode;
	}

	public int getExit() {
		return _exitcode;
	}

	public void echo(String text) {
		if(_tsid == null) {
			System.out.println(text);
		}
		else {
			System.err.println(text);
		}
	}

	public String getHead(String key) {
		return (String)_head.get(key);
	}

	public String getArg(String arg) {
		return (String)_args.get(arg);
	}

	public boolean isLive() {
		if(_tsid == null || _exitcode > 0) {
			return false;
		}
		return true;
	}

	public String getPath(String file) {
		String ext = getHead("EXTENSION");
		String pre = getHead("PREFIX");
		int epos, spos;

		if(ext == null) {
			ext = ".au";
		}
		
		if(file == null) {
			return null;
		}

		if(file.startsWith("/") || file.startsWith(".")) {
			return null;
		}

		if(file.substring(1, 1) == ":") {
			return null;
		}

		if(file.indexOf("..") > 0 || file.indexOf("/.") > 0) {
			return null;
		}

		spos = file.lastIndexOf('/');
		epos = file.lastIndexOf('.');
		if(epos < spos) {
			epos = 0;
		}

		if(epos < 1) {
			file = file + ext;
		}

		if(file.startsWith("tmp:")) {
			return _tmpfs + "/" + file.substring(4);
		}

		if(file.startsWith("ram:")) {
			return _ramfs + "/" + file.substring(4);
		}

		if(file.indexOf(':') > 0) {
			return null;
		}

		if((file.indexOf('/') < 1) && (pre != null)) {
			return _home + "/" + pre + "/" + file;
		}

		if(file.indexOf('/') < 1) {
			return null;
		}
		return _home + "/" + file;
	}

	public void hangup() {
		if(_tsid == null || _exitcode > 0) {
			return;
		}
		System.out.println(_tsid + " HANGUP");
		_tsid = null;
	}

	public void error(String msg) {
		if(_tsid == null || _exitcode > 0) {
			return;
		}
		System.out.println(_tsid + " ERROR " + msg);
		_tsid = null;
	}

	public void detach(int result) {
		if(_tsid == null || _exitcode > 0) {
			return;
		}
		if(result < 0 || result > 255) {
			result = 255;
		}
		
		System.out.println(_tsid + " EXIT " + result);
		_tsid = null;
	}

	protected int command(String text) {
		String lbuf, keyword, value;
		int num = 0;

		_resultcode = 255;
		_query = "";

		if(_tsid == null || _exitcode > 0) {
			return 255;
		}

		System.out.println(_tsid + " " + text);
		while(_exitcode < 1) {
			try {
				lbuf = _in.readLine();
			}
			catch(IOException e) {
				lbuf = null;
			}
			if(lbuf == null) {
				_exitcode = 3;
				return 255;
			}
			if(lbuf.length() < 4) {
				break;
			}
			try {
				num = Integer.parseInt(lbuf.substring(0, 3));
			}
			catch (NumberFormatException e) {
				num = 0;
			}
			if(num > 900) {
				_exitcode = num - 900;
				return 255;
			}
			if(num > 0) {
				_reply = num;
				continue;
			}
			num = lbuf.indexOf(':');
			keyword = lbuf.substring(0, num);
			value = lbuf.substring(num + 2);
			switch(_reply) {
			case 100:
				if(keyword == "RESULT") {
					try {
						_resultcode = Integer.parseInt(value);
					}
					catch (NumberFormatException e) {
						_resultcode = 255;
					}
					break;
				}
				if(keyword == "DIGITS") {
					_digits = value;
					break;
				}
				if(keyword == "POSITION") {
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

	public int replayFile(String file) {
		return command("REPLAY " + file);
	}

	public int replayOffset(String file, String offset) {
		return command("REPLAY " + file + " " + offset);
	}

	public int recordFile(String file, long total, long silence) {
		return command("RECORD " + file + " " + total + " " + silence);
	}

	public int recordOffset(String file, String offset, long total, long silence) {
		return command("RECORD " + file + " " + total + " " + silence + " " + offset);
	}

	public int eraseFile(String file) {
		file = getPath(file);
		if(file == null) {
			_resultcode = 254;
			return 254;
		}
		boolean success = (new File(file)).delete();
		if(!success) {
			_resultcode = 1;
			return 1;
		}
		_resultcode = 0;
		return 0;
	}

	public int moveFile(String file1, String file2) {
		file1 = getPath(file1);
		file2 = getPath(file2);
		if(file1 == null || file2 == null) {
			_resultcode = 254;
			return 254;
		}
		boolean success = (new File(file1)).renameTo(new File(file2));
		if(!success) {
			_resultcode = 1;
			return 1;
		}
		_resultcode = 0;
		return 0;
	}

	public boolean setVoice(String v) {
		_voice = v;
		return true;
	}

	public void setLevel(short level) {
		_tonelevel = level;
	}

	public int clearInput() {
		return command("FLUSH");
	}

	public char readKey(long timeout) {
		if(_tsid == null || _exitcode > 0) {
			return 0;
		}
		_digits = "";
		if(command("READ " + timeout) > 0) {
			return 0;
		}
		if(_digits.length() < 1) {
			return 0;
		}
		return _digits.charAt(0);
	}

	public String readInput(short count, long timeout) {
		if(_tsid == null || _exitcode > 0) {
			return "";
		}
		
		_digits = "";
		if(command("READ " + timeout + " " + count) > 0) {
			return "";
		}
		return _digits;
	}

	public boolean waitInput(long timeout) {
		if(_tsid == null || _exitcode > 0) {
			return false;
		}
		_digits = "";
		if(command("WAIT " + timeout) > 0) {
			return false;
		}
		if(_digits.length() > 0) {
			return true;
		}
		return false;
	}			

	public int sendResult(String text) {
		return command("RESULT " + text);
	}

	public int speak(String text) {
		String voice = "PROMPT";
		if(_voice != null) {
			voice = _voice;
		}
		return command(voice + " " + text);
	}

	public int playTone(String tone, long timeout) {
	return command("TONE " + tone + " " + timeout + " " + _tonelevel);
	}

        public int playSingleTone(short tone, long timeout) {
  		return command("STONE " + tone + " " + timeout + " " + _tonelevel);
        }   

        public int playTone(short tone1, short tone2, long timeout) {
		return command("DTONE " + tone1 + " " + tone2 + " " + timeout + " " + _tonelevel);
	}   

	public int sizeSymbol(String sym, int count) {
		return command("NEW " + sym + " " + count);
	}

	public int setSymbol(String sym, String value) {
		return command("SET " + sym + " " + value);
	}

        public int addSymbol(String sym, String value) {
                return command("ADD " + sym + " " + value);
        }
            
	public String getSymbol(String sym) {
		if(_tsid == null || _exitcode > 0) {
			return null;
		}
		_query = "";
		_reply = 0;
		command("GET " + sym);
		if(_reply != 200) {
			return null;
		}
		return _query;
	}
}


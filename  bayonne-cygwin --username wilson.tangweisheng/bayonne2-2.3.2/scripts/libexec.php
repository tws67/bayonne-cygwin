<?php
/* Copyright (C) 2005 David Sugar, Tycho Softworks    
 *
 * This file is free software; as a special exception the author gives
 * unlimited permission to copy and/or distribute it, with or without
 * modifications, as long as this notice is preserved.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *
 * Bayonne php libexec interface module.
 */

class Libexec {
	var $voice = "";
	var $digits = "";
	var $query = "";
	var $position = "00:00:00.000";
	var $reply = 0;
	var $resultcode = 0;
	var $exitcode = 0;
	var $level = 0;
	var $version = "4.0";
	var $tsession;
	var $stdin;
	var $stdout;
	var $stderr;
	var $head = array();
	var $args = array();
	
	function Libexec() {
		$this->stdin = fopen("php://stdin", "r");
		$this->stdout = fopen("php://stdout", "w");
		$this->stderr = fopen("php://stderr", "w");
		$this->tsession = getenv('PORT_TSESSION');
		$tsid = $this->tsession;
		flush();
		ob_implicit_flush(1);
		if(strlen($this->tsession) < 1) {
			return;
		}
		fputs($this->stdout, "$tsid HEAD\n");
		while($line = fgets($this->stdin, 1024))
		{
			if($line === "\n") {
				break;
			}
			if($line > 900) {
				$this->reply = $line - 0;
				$this->exitcode = $line - 900;
				$this->resultcode = 255;
				break;
			}			
			if($line > 0) {
				$this->reply = $line - 0;
				continue;
			}
			preg_match("/(.*?)[:][ ](.*\n)/", $line, $match);
			$match[2] = trim($match[2]);
			$this->head[$match[1]] = $match[2];
		}
		if($this->exitcode > 0) {
			return;
		}
		fputs($this->stdout, "$tsid ARGS\n");
		while($line = fgets($this->stdin, 1024))
		{
			if($line === "\n") {
				break;
			}
			if($line > 900) {
				$this->reply = $line - 0;
				$this->exitcode = $line - 900;
				$this->resultcode = 255;
				break;
			}			
			if($line > 0) {
				$this->reply = $line - 0;
				continue;
			}
			preg_match("/(.*?)[:][ ](.*\n)/", $line, $match);
			$match[2] = trim($match[2]);
			$this->args[$match[1]] = $match[2];
		}
	}

	function stop_hangup() {
		$tsid = $this->tsession;
		if(strlen($tsid) < 1 || $this->exitcode > 0) {
			return;
		}
		fputs($this->stdout, "$tsid HANGUP\n");
		$this->tsession = "";
		return;
	}

        function stop_detach($code) {
                $tsid = $this->tsession;
                if(strlen($tsid) < 1 || $this->exitcode > 0) { 
                        return; 
                }
                fputs($this->stdout, "$tsid EXIT $code\n");
                $this->tsession = "";
                return;  
	}

	function send_command($command) {
		$tsid = $this->tsession;
		$result = 253;

		if(strlen($tsid) < 1 || $this->exitcode > 0) {
			return 255;
		}
		fputs($this->stdout, "$tsid $command\n");
		while($line = fgets($this->stdin, 1024))
		{
			if($line === "\n") {
				break;
			}
			if($line > 900) {
				$this->reply = $line - 0;
				$this->exitcode = $line - 900;
				$this->resultcode = 255;
				return 255;
			}
			if($line > 0) {
				$this->reply = $line - 0;
				continue;
			}		
			if(($this->reply != 100) && ($this->reply != 400)) {
				continue;
			}
			preg_match("/(.*?)[:][ ](.*\n)/", $line, $match);
                        $match[2] = trim($match[2]);
			$match[1] = strtolower($match[1]);
			if($this->reply == 200)
			{
				$this->query = $match[2];
				continue;
			}
			switch($match[1])
			{
			case 'digits':
				$this->digits = $match[2];
				break;
			case 'result':
				$result = $match[2];
				$this->resultcode = $result;
				break;
			case 'position':
				$this->position = $match[2];
				break;
			}
		}
		return $result;
	}

	function send_result($code) {
		return $this->send_command("RESULT $code");
	}

	function xfer_call($code) {
		return $this->send_command("XFER $code");
	}

	function print_output($output) {
		if(strlen($this->tsession) > 0 && $this->exitcode == 0) {
			fputs($this->stderr, $output);
		} else {
			fputs($this->stdout, $output);
		}
	}

	function get_pathname($file) {
		$varfs = getenv('SERVER_PREFIX');
		$ramfs = getenv('SERVER_TMPFS');
		$tmpfs = getenv('SERVER_TMP');
		$ext = $this->head['EXTENSION'];
		$prefix = $this->head['PREFIX'];

		if(strlen($file) < 1) {
			return "";
		}

		$spos = strrpos($file, "/");
		$epos = strrpos($file, ".");
		
		if(!$epos) {
			$epos = 0;
		}

		if(!$spos) {
			$spos = 0;
		}

		if($epos < $spos) {
			$epos = 0;
		}

		if($epos < 1) {
			$file = "$file$ext";
		}
		
		if(substr($file, 0, 4) === "tmp:") {
			$sub = substr($file, 4);
			return "$tmpfs/$sub";
		}

		if(substr($file, 0, 4) === "ram:") {
			$sub = substr($file, 4);
			return "ramfs/$sub";
		}
	
		if(strpos($file, ":")) {
			return "";
		}

		if(strpos($file, "/")) {
			return "$varfs/$file";
		}

		if(strlen($prefix) > 0) {
			return "$varfs/$prefix/$file";
		}

		return "";
	}

	function get_filename($file) {
		$prefix = $this->head['PREFIX'];

		if(strlen($file) < 1) {
			return "";
		}
		
		if(substr($file, 0, 4) === "tmp:") {
			return $file;
		}

		if(substr($file, 0, 4) === "ram:") {
			return $file;
		}
	
		if(strpos($file, ":")) {
			return "";
		}

		if(substr($file, 0, 1) === "/") {
			return "";
		}

		if(strpos($file, "/")) {
			return "$file";
		}

		if(strlen($prefix) > 0) {
			return "$prefix/$file";
		}

		return "";
	}

	function erase_file($file) {
		$file = $this->get_pathname($file);

		if(strlen($file) < 1) {
			$this->resultcode = 254;
			return 254;
		}
		$this->resultcode = 0;
		unlink($file);
		return 0;
	}

	function move_file($file1, $file2) {
		$file1 = $this->get_pathname($file1);
		$file2 = $this->get_pathname($file2);

		if(strlen($file1) < 1 || strlen($file2) < 1) {
			$this->resultcode = 254;
			return 254;
		}
		$this->resultcode = 0;
		rename($file1, $file2);
		return 0;		
	}

	function set_voice($voice) {
		$this->voice = $voice;
		return true;
	}

	function set_level($level) {
		$this->level = $level;
	}

	function replay_file($file) {
		$file = get_filename($file);
		if(strlen($file) < 1) {
			$this->resultcode = 254;
			return 254;
		}

		return $this->send_command("REPLAY $file");
	}

	function replay_offset($file, $offset) {
		$file = get_filename($file);
		if(strlen($file) < 1) {
			$this->resultcode = 254;
			return 254;
		}
		return $this->send_command("REPLAY $file $offset");
	}

	function record_file($file, $total, $silence) {
		$file = get_filename($file);
		if(strlen($file) < 1) {
			$this->resultcode = 254;
			return 254;
		}
		return $this->send_command("RECORD $file $total $silence");
	}

	function record_offset($file, $offset, $total, $silence) {
		$file = get_filename($file);
		if(strlen($file) < 1) {
			$this->resultcode = 254;
			return 254;
		}
		return $this->send_command("RECORD $file $total $silence $offset");
	}

	function speak_phrase($prompt) {
		$voice = $this->voice;
		if(strlen($voice) < 1) {
			$voice = "PROMPT";
		}
		return $this->send_command("$voice $prompt");
	}

	function play_tone($tone, $duration) {
		$level = $this->level;
		return $this->send_command("TONE $tone $duration $level");
	}

        function single_tone($tone, $duration) {
                $level = $this->level;
                return $this->send_command("STONE $tone $duration $level");
        }      

        function dual_tone($tone1, $tone2, $duration) {
                $level = $this->level;
                return $this->send_command("DTONE $tone1 $tone2 $duration $level");
        }      
            
	function read_input($count, $timeout) {
		$result = $this->send_command("READ $timeout $count");
		if($result == 0) {
			return $this->digits;
		}
		return "";
	}

	function read_key($timeout) {
		$result = $this->send_command("READ $timeout");
		if($result == 0) {
			return substr($this->digits, 0, 1);
		}
		return "";
	}

	function wait_input($timeout) {
		$result = $this->send_command("WAIT $timeout");
		if($result == 3) {
			return true;
		}
		return false;
	}

	function clear_input() {
		return $this->send_command("FLUSH");
	}		

	function size_symbol($sym, $size) {
		$size = $size - 0;
		return $this->send_command("new $id $size");
	}

	function set_symbol($sym, $value) {
		return $this->send_command("set $id $value");
	}

	function add_symbol($sym, $value) {
		return $this->send_command("add $id $value");
	}

	function get_symbol($sym) {
		$this->query = "";
		$this->send_command("get $sym");
		return $this->query;
	}
}
?>

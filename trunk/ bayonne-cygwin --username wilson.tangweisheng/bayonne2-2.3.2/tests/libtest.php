#!/usr/bin/php -q
<? 
/* a shame there is no env var to set php include paths externally... */
$include = ini_get('include_path');
$libexec = getenv('SERVER_LIBEXEC');
if(strlen($libexec) > 0) {
	ini_set('include_path', "$include:$libexec");
} else {
	ini_set('include_path', "$include:../scripts");
}
require_once("libexec.php");
$TGI = &new Libexec;
$TGI->print_output("PHP LIBEXEC STARTED\n");
$result = $TGI->speak_phrase("&number 123");
$TGI->send_result($result);
$file1 = $TGI->get_pathname("tmp:edit");
$TGI->print_output("P1 $file1\n");
$file2 = $TGI->get_pathname("temp/edit");
$TGI->print_output("P2 $file2\n");
$TGI->print_output("** TGI RESULT $result\n");  
?>

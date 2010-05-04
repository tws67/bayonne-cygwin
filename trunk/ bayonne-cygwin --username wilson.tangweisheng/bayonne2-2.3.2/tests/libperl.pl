#!/usr/bin/perl
#bayonne: set %pin "123"
#bayonne: string %result
#bayonne: exec.3 %pin results=&result
#bayonne: echo "ERROR %script.error"
#bayonne: echo "LIBEXEC RESULT %result"
#
# Copyright (C) 2005 David Sugar, Tycho Softworks
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
# Basic perl script for testing of libexec subsystem and use of
# Bayonne::Libexec perl module.

print STDERR "STARTING!\n";

# this is needed if test script is executed directly (outside Bayonne)
use lib '../scripts';

# get Bayonne libexec package under Bayonne server (PERL5LIB ref)
use libexec;

# create $TGI instance of libexec (based on future cpan mapping...)
my($TGI) = new Bayonne::Libexec;

# get tgi version, and print it (sent to stderr if in tgi). 
my($ver)=$TGI->{version};
$TGI->print("TGI VERSION $ver\n"); 

# see if we are running under bayonne, or just stand-alone
if(!$TGI->{tsession}) {
	print STDERR "*** Started in local execution mode, exiting...\n";
	exit
}

my $pin = $TGI->{args}{PIN};
$TGI->print("TIMEOUT FROM HEADER IS $TGI->{head}{TIMEOUT}\n");
$TGI->print("PIN NUMBER PASSED IS $pin\n");
my $result = $TGI->speak("&number $pin");

$TGI->print("COMMAND RESULT $result\n");
$TGI->print("LAST COMMAND RESULT BEFORE EXIT $TGI->{result}\n");
$TGI->result("CODE $result");

my($file) = $TGI->filename("tmp:edit");
$TGI->print("FILE NAME $file\n");

exit;



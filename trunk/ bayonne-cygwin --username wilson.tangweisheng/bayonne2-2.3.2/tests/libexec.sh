#bayonne: echo "*** BAYONNE LIBEXEC SHELL STARTUP ***"
#
# Copyright (C) 2005 Open Source Telecom Corp.
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
# Basic shell script for testing of libexec subsystem and use of testexec
# command to extract transactional replies.

# echo to stderr will appear on server stderr if not in daemon mode
prefix=`pwd`
echo "*** STARTUP LIBEXEC" >&2
echo "*** BTSEXEC SHELL $SHELL_BTSEXEC" >&2
echo "*** LIBEXEC PATH $PATH" >&2
echo "*** LIBEXEC EXEC $SERVER_LIBEXEC" >&2
echo "*** LIBEXEC HOME $HOME" >&2
echo "*** LIBEXEC PERL $PERL5LIB" >&2
echo "*** LIBEXEC JAVA $CLASSPATH" >&2
echo "*** LIBEXEC PYTHON $PYTHONPATH" >&2
echo "*** LIBEXEC DIR $prefix" >&2
echo "*** LIBEXEC SCR $SERVER_SCRIPTS" >&2
echo "*** LIBEXEC SYS $SERVER_SYSEXEC" >&2

echo "*** PIN ARGUMENT PASSED IN ENV $ARGS_PIN" >&2

# issue head command using session id and report server headers
echo "$PORT_TSESSION head"
../../utils/testexec

# issue args command for retrieving libexec command line arguments (if any)
echo "$PORT_TSESSION args"
../../utils/testexec

# play a phrase through phrasebook using the default server voice settings
# and return transactional results
echo "$PORT_TSESSION prompt &number 123"
../../utils/testexec

# catch the exit message at timeout and show it
../../utils/testexec


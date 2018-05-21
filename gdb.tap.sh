#!/bin/bash
# Author: Michael Ambrus (ambrmi09@gmail.com)
# 2011-03-05
# Shell wraps tcp_tap to make it gdb.tap.sh
# Use this file "as-is" or as a template for your own wrappers. Mandatory
# adjustments are TCP_TAP_EXEC & TCP_TAP_PORT, which need to be unique for each
# program you intend to extend with a "tap".


if [ -z $TAP_SH ]; then

TAP_SH="tap.sh"

# This script works as the basename command, except that it also
# ripps away everything in the name before the next but last '.'
# I.e. usage like:
# $ tap /some/path/pre.fix.myshell.sh
#   myshell.sh
#

function tap() {
	exec tcp-tap "$@"
}

if [ "$TAP_SH" == $( basename $0 ) ]; then
	#Not sourced, do something with this.

	export TCP_TAP_EXEC="$(which gdb)"
	export TCP_TAP_PORT="8088"
	export TCP_TAP_LOG_STDIN="/dev/null"
	export TCP_TAP_LOG_STDOUT="/dev/null"
	export TCP_TAP_LOG_STDERR="/dev/null"
	export TCP_TAP_LOG_PARENT="/dev/null"
	export TCP_TAP_LOG_CHILD="/dev/null"

	tap "$@"
	exit $?
fi

fi

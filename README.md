Introduction
============

tcp_tap was developed to aid graphical GDB front-ends like kdbg that lack
command line interaction for fine-tuning control of a session with just
that, a console. It will work with any stdio based interactive program
however, not just GDB.

The rest of this document focuses on GDB.

Ever needed to be able to execute native debugger commands using the GDB
monitor commands in a deeply embedded project (flashing memory e.t.a.), but
can't because your favorite GDB GUI don't support it? Then this project is
for you!

Ever needed to execute commands you GUI can't, like for example check if
shared objects are loaded and where, or execute commands for stopping
kernel from running more threads than one (`set scheduler-locking on`).
Then this project is for you!

Ever needed to be able to run your own GDB Python scripts (which you
favorite GUI will never support), or in general extend your favorite GUI
with features it doesn't already has (both textual and graphical), THEN
THIS PROJECT IS FOR YOU!

Build and install
=================

To build and install, just:

 make

 sudo make install

No configure needed (it's a simple program).

Usage
=====

* Use the wrapper script GDB.tap.sh in place of the GDB command in the
  settings of your front-end. Or make a link to that script, place it in
  your ~/bin and  make sure ~/bin comes first in your $PATH.

* Edit and configure gdb.tap.sh (see where to get a better one it further
  below) with the true back-end GDB that you will use. I.e. See TCP_TAP_EXEC
  in the script, adjust to your liking, but remember: it has to be a full
  path.

* gdb.tap.sh when set up properly is now a drop in replacement for gdb. You
  can even rename the script to gdb, or place a link to it somewhere earlier
  in path and it should look and behave exactly as your systems original gdb
  in all aspects.

* Note that since the wrapper needs to be transparent to who-ever uses it,
  it can't accept settings via command-line flags. All adjustments and
  settings *must* be handled via environment variables. That's why the
  wrapper script is needed at all. You can write a wrapper script yourself,
  wrapping your favorite interactive program, use GDB.tap.sh as template and
  look into it and adjust to your preference. Note that for each program you
  wrap, you should choose a different executable (modify TCP_TAP_EXEC) and a
  different port (modify TCP_TAP_PORT). A much more competent wrapper is
  part of the script3 script-library, which in turn is a Google repo
  repository, but you don't need all that, just that particular project:

  https://github.com/mambrus/script3-gdb

  In fact, you don't even need that as you can easily write your own wrapper
  encapsulating the environment variables with, but it might serve as
  inspiration. The newest versions are as mentioned very capable aiming to fully
  generalize the use including "GNU readline" and full history for the
  client, but look at the history back to the beginning for this file and you
  will find a copy of the one in this project:
  https://github.com/mambrus/script3-gdb/blob/master/tap.sh
  That should still work...

Environment variables
---------------------
Here's a list of the environment variables tcp_tap will react upon. Note: all
have default values. This is for ease of debug ability you must nevertheless
change a couple of them.

* TCP_TAP_EXEC: The binary you want to run and extend with tap-ability.
  Default is /bin/sh

* TCP_TAP_PORT: Exactlly What is sounds like. Default is 6969

* TCP_TAP_NICNAME: Listen at NIC bound to this name (iether as human
  readable name or as IP-address).

  ATTENTION: 127.0.0.1 is the safest name to use as it can't easily be
  spoofed. Usage of "localhost" is less safe, but having 127.0.0.1 does the
  same thing for valid usages, i.e. server will listen and bind  to name
  "localhost". Leave as default if no outside connections are welcome.

  On multi-homed machines this field has the alternative use of letting
  traffic getting back to the interface it bound to. This matters when
  serves is behind a NAT and returning packages need to to back to the same
  router to be able to find their way home to the original host.

  Two special names can be used:
  1) @HOSTNAME@: To use the hosts own host-name, use special name
     @HOSTNAME@. This is fairly safe. It will bind to one IF only, the one
     physically associated.

  2) @ANY@ To connect to any IF, all NIC:s and all IF's (i.e. all physical
     interfaces and non-physical like localhost) with the name

* TCP_TAP_KILL_CMD: A special string that allow clients from disconnecting
  from just the tap without killing the original session. Defauilt is
  "@quit".

The following are used for debugging and should be piped to /dev/null
unless needed.

* TCP_TAP_LOG_STDIN:  /tmp/tcp_tap_stdin

* TCP_TAP_LOG_STDOUT: /tmp/tcp_tap_stdout

* TCP_TAP_LOG_STDERR: /tmp/tcp_tap_stderr

* TCP_TAP_LOG_PARENT: /tmp/tcp_tap_parent.log

* TCP_TAP_LOG_CHILD:  /tmp/tcp_tap_child.log

Some rambling about why this project exists at all:
===================================================

A very good graphical front-end for GDB is kdbg. It's simple and it's fast
(and is therefore the authors favorite). BUT: it lacks for example KDevelops
ability to provide you with a command-line window where you can enter
fine-tuning for a session, or for using GDB's new Python extension abilities
and user provided special commands. KDevelop used to be the favourite, but
it's lost it's shine since trying to enforce users a particular
build-environment and what-not. If one is going for bloated GUI:s one might
just as well go for Eclipse.

The basic concept is to wrap GDB with another program wich also acts as a
"tap". The wrapper pipes everything between the GDB front-end and back-end,
but it also listens to a socket. If somebody opens that port (using for
example telnet), the wrapper will allow GDB commands on the side (i.e. in
parallel with the graphical front-end) to be sent to the back-end.

This would normally be very simple to do with Expect (and has been done for
other front-ends like KDevelop4 and Eclipse), but for some reason either
Expect can't handle streams in non-interactive mode and slightly modifies
the stream and/or kdbg is very picky about the exact content in the stream.
tcp_tap shuffles the content exactly as-is

Anyway, with tcp_tap instead of a corresponding Expect wrapper, you'll also
as a special bonus get:

* A much faster "tap".

* See all commands sent from the front-end (useful for set-up debugging)

* All replies from the back-end will be sent to both front-end and
  socket users.

* It allows multi session debugging (this is pretty cool!). I.e. your
  college can participate in debugging the same session as you. Good for
  helping each other with a particular problem us a particular problem using
  the same session. This for natural reasons this works only with additional
  clients in text-mode (there can be only one full-GUI who also acts master).

- Drawback - socket users will not be able to use GDB terminal helpers (i.e.
  tab key and up/down arrows will not work). This is being dealt with (see
  reference to script3's gdb.tap.sh mentioned above).


Note
====
Note that a "tap" works excellent not only for native GDB, but also for
using with cross-tools. It's especially useful when GDB is hooked to a jTag
based GDBserver and when you need to enter control commands which the
GDBserver listens at (read about the monitor command for GDB).

Future work
============

* Parts of this README will be replicated into the Wiki, only build and
  basic usage information will be kept.

* A small program replacing telnet for better user-interaction instead of
  telnet.

* A small Qt based application intended as a template for extending a GDB
  front-end.

# TCP-TAP

![logo.png](logo.png)

tcp_tap is a utility for adding a TCP-tap to console programs

* Multi-session TCP-IP interactivity
* Adding ability by wrapping sub-commands similar to
  [GNU coreutils: stdbuf](https://www.gnu.org/software/coreutils/manual/html_node/stdbuf-invocation.html)
* Adds a `tee`, but on `stdin`


Originally intended to extend gdb front-ends that lack a console (like for
example kdbg). It can however be used for any program which uses stdio to
communicate (including interactive console programs which communicate with
the shell). tcp_tap sits in-between the io-stream and listens at a port.

When someone connects to that port, a new "tap"-session will be created for
that client.  Everything sent in-between the original program and it's
original client will be replicated back and forth to the new client as well.

## Is this for you?

Adding TCP-IP abilities to console programs can be done in many ways. Like
the classic `initd`/`xinitd` or `netcat`

You would use `tcp-tap` either when:

* Get in-between two programs (typically front- vs. back-ends)
* Multisession (queued) is required

By *not* accepting any arguments of it's own and send everything to the
*wrapped* command, tcp-tap is able to add it's extended features also to
programs (normally GUI front-ends) that have it's back-end hard-coded.

## Build

### Native builds:

`tcp-tap` binary and libraries (installed)

```bash
cmake .
ccmake .
make
sudo make install
make clean
```

### Examples:

Examples are built separately. Use above installed library.

```bash
cd examples/
cmake .
ccmake .
make
```

## [Cross builds](https://github.com/helsinova/xcmake/blob/master/x-build.md)

## How to use

* Set a few environment variables
  * Wrapped inside a shell-sript
* Optionally (i.e. if needed) rename this script to the intended command and
  change the `$PATH`

This stealthy ability makes it possible to extend without requiring modification.

An example of how to use in this script: [gdb.tap.sh](gdb.tap.sh)

*Note:* Whence on TCP stream, terminal awareness and features are either
lost or (probably) not functional.

## Cloning this project all in one go

To clone the project including it's git sub-modules:

```bash
git clone --recursive https://github.com/mambrus/tcp_tap.git
```

## More info

Browse the [Wiki](https://github.com/mambrus/tcp_tap/wiki) for more
information.

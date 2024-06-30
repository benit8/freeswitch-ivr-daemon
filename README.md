# FreeSWITCH IVRd

This repository contains a copy of [FreeSWITCH](https://github.com/signalwire/freeswitch)'s
[IVRd component](https://github.com/signalwire/freeswitch/blob/master/libs/esl/ivrd.c).

It has been refactored to prevent high CPU usage and zombie children processes,
among other things.

## Requirements

Binaries:

-   make

Libraries:

-   pthread

## Build instructions

The binary is built using make. It will produce the target at `./ivrd`.

```sh
make
```

## Usage

```
Usage: ivrd <script path> [script arguments...]

Description:
	Start the IVR daemon, listening on <host>:<port>.

Options:
	-h, --help                 Display this help message.
	-H, --hostname <hostname>  The hostname the daemon should listen to. Defaults to 127.0.0.1.
	-p, --port <port>          The port the daemon should listen to. Defaults to 8084.
	    --connect              Attach the ESL socket to a handle, effectively issuing a 'connect' command to FreeSwitch.
	    --threaded             Use threads instead of fork()'ing to handle connections.
	    --hotwire              Plug the connected socket's ends into a child process' STDIN/STDOUT.
	                           Only when not --threaded.

Arguments:
	script path                Path to a program that will be executed to handle incoming connections.
	                           It will receive the file descriptor of the connected socket on its arguments,
	                           followed by the <script arguments> provided to the daemon.
	script arguments           Any number of additional arguments to pass to the invoked <script path>.
```

CSC 360 Assigment 1 Description

Overview
This is a script written in C meant to emulate a simple operating system that can create and manage
processes. This script is only meant to be ran on the linux.csc.uvic.ca server.

Compilation
Ensure the following files are all in your current directory:
Makefile
linkedls.h
linkedls.c
main.c

Type 'make' into the terminal to compile.


Usage in the terminal

Initiate:
./pman

Start a background process:
bg ./[process path] [arg_1] ... [arg_n]

List background processes:
bglist

Kill a background process:
bgkill [pid]

Stop a background process:
bgstop [pid]

Continue a background process:
bgstart [pid]

Show status of a process
pstat [pid]

returns:
process path
state
utime
stime
rss
voluntary ctxt switches
nonvoluntary ctxt switches

If user enters an invalid pid pman will print:
"Process [pid] not found"


EXAMPLE
PMan: > bg ./inf tag 3
Started background process with PID 45137
PMan: > bglist
45137: ./inf
Total background jobs: 1
PMan: > bgstop 45137
Process 45137 stopped
PMan: > pstat 45137
comm: (inf)
state: T
utime: 0
stime: 0
rss: 145
voluntary_ctxt_switches: 19
nonvoluntary_ctxt_switches: 1
PMan: > bgstart 45137
Process 45137 started
PMan: > bgkill 45137
Process 45137 has been killed
PMan: > bglist
Total background jobs: 0
PMan: > 


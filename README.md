Kgrep Experiments
=================

This is a collection of experiments designed to test the feasiblity of performing data transformations in kernel space.
These tests are written to run on the FreeBSD kernel.

Installation
============

You should already have a working copy of FreeBSD built from source in /usr/src.

You will also need a few packages:

```console
pkg install git bison
```

To begin, clone the repository somewhere convenient.

```console
git clone git@github.com:gharris1727/kgrep.git kgrep
cd kgrep
```

Install the kgrep kernel module

```console
cd kgrep && sudo ./install.sh
```

This will install the kgrep module source into `/usr/src/sys/dev/kgrep` and the makefile into `/usr/src/sys/modules/kgrep`.

To build and link the kernel module into your kernel,

```console
cd /usr/src/sys/modules/kgrep && make all && sudo make unload load
```

This will then expose the `/dev/kgrep_control` device, to which we can send commands.

Running
=======

A simple test program is `kgrep/cli/kgrep`, build it before using it.

```console
cd kgrep/cli && make
```

One example usage is to search /var/log/messages for instances of 'kernel', printing output on stdout, maxing out at 1000 matches:

```console
kgrep/cli/kgrep kernel /var/log/messages - 1000
```

As a performance comparison, there is a user-space program `ugrep/ugrep` that performs the exact same operation, but in userspace.
To build:

```console
cd ugrep && make
```

Example usage, performing the same operation as the previous step:

```console
cd ugrep && ./ugrep kernel /var/log/messages - 10000
```

Notice that ugrep must be run from it's own directory: this is because it relies on local linking to the sregex library.

Testing
=======

In order to run scripted tests on the above applications, make sure they are all built as directed.
A script `run.sh` is provided, and it will perform timing tests on the different programs, varying their parameters.

```console
./run.sh
```

The output of these tests is put in `out.csv` in the root of the repository.


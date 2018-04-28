Kgrep Experiments
=================

This is a collection of experiments designed to test the feasiblity of performing data transformations in kernel space.
These tests are written to run on the FreeBSD kernel.

Installation
============

You should already have a working copy of FreeBSD built from source in /usr/src.

You will also need a few packages:

```console
pkg install git bison autoconf automake rsync
```

To begin, clone the repository somewhere convenient.

```console
git clone git@github.com:gharris1727/kgrep.git kgrep
cd kgrep
```

Build and load all of the kernel modules, userspace programs, and test data.
(in progress, doesn't work)

```console
make
```

Alternatively, build everything manually.

```console
cd kgrep/module && make obj all && sudo make load
cd kagrep/module/grep && ./bootstrap && ./configure
cd kagrep/module && make obj all && sudo make load
```

Build all of the userspace programs
```console
cd kgrep/cli && make
cd kagrep
cd kagrep/cli && make
cd ugrep && make
```

This will then expose the `/dev/kgrep_control` and `/dev/kagrep_control' devices, to which we can send commands.

Build kosher grep (THIS PROBABLY WONT WORK)

```console
cd kagrep/module/grep && ./bootstrap && ./configure
cd kagrep/cli/grep && ./bootstrap && ./configure
./bootstrap
./configure
```

Additional libraries that need to be symlinked:

```console
ln -s /home/greg/kgrep/kagrep/module/grep/gnulib/lib/wcsnlen.c lib/
ln -s /home/greg/kgrep/kagrep/module/grep/gnulib/lib/wcsnlen-impl.h lib/
ln -s /home/greg/kgrep/kagrep/module/grep/gnulib/lib/wcsncmp.c lib/
ln -s /home/greg/kgrep/kagrep/module/grep/gnulib/lib/wcsncmp-impl.h lib/
ln -s /home/greg/kgrep/kagrep/module/grep/gnulib/lib/wcsdup.c lib/
ln -s /home/greg/kgrep/kagrep/module/grep/gnulib/lib/wcsdup-impl.h lib/
ln -s /home/greg/kgrep/kagrep/module/grep/gnulib/lib/wcslen.c lib/
ln -s /home/greg/kgrep/kagrep/module/grep/gnulib/lib/wcslen-impl.h lib/
ln -s /home/greg/kgrep/kagrep/module/grep/gnulib/lib/wcsncpy.c lib/
ln -s /home/greg/kgrep/kagrep/module/grep/gnulib/lib/wcsncpy-impl.h lib/
ln -s /home/greg/kgrep/kagrep/module/grep/gnulib/lib/wmemcpy.c lib/
ln -s /home/greg/kgrep/kagrep/module/grep/gnulib/lib/wmemcpy-impl.h lib/
```

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


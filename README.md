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

Prepare to build the repository by initializing dependencies and creating object directories.

```console
make init obj objlink
```

Build and load all of the kernel modules and userspace programs.

```console
make 
sudo make load
```

This will then expose the `/dev/kgrep_control` and `/dev/kagrep_control` devices, to which we can send commands.

To remove everything completely, you can run each of these to remove all modules and built objects.

```console
sudo make unload 
make clean
make cleandir cleandir
```

Running
=======

The executables for each program will be avaliable at the following paths inside the repository:

```console
ugrep/obj/ugrep
kgrep/cli/obj/kgrep
kagrep/cli/obj/kagrep
```

One example usage is to search /var/log/messages for instances of 'kernel', printing output on stdout, maxing out at 1000 matches:
Both ugrep and kgrep have the same command-line syntax, as follows.

```console
ugrep/obj/ugrep kernel /var/log/messages - 1000
kgrep/cli/obj/kgrep kernel /var/log/messages - 1000
```

A similar usage for kagrep is as follows:

```console
kagrep/cli/obj/kagrep kernel fgrep o /var/log/messages - 1000
```

Notice the additional "fgrep" and "o" arguments for configuring the internal matcher.
"fgrep" indicates that fixed string matches should be used, and "o" indicates that only the match itself should be output.

Other values for these fields can be seen in kagrep/module/INTERFACE.md, for the "matcher" and "flags" fields.

Testing
=======

In order to run scripted tests on the above applications, make sure they are all built as directed.
There are currently two scripted tests, `urandom.sh` and `enron.sh`. 

`urandom.sh` reads data from /dev/urandom, and populates `urandom.csv` with results from each test run.
This is not a prepared data set, and can be run immediately after cloning and building.

In order to operate on the prepared datasets, we have to first download and preprocess them.

```console
make datasets
```

The enron dataset can be found in `enron/obj/` along with the preprocessed files.

`enron.sh` reads data from enron.tar, a single 1.7G tarball filled with emails.
The results of the test are put in `enron.csv`


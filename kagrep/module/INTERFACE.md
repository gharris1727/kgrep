
The kagrep kernel module exposes a character device for communication with userspace programs.
This character device is `/dev/kagrep_control`.
In order to use kagrep, you must:

* Open the `/dev/kagrep_control` device.
* Write commands to the device (in the format specified below).
* Read results from the device (in the standard format).
* Close the `/dev/kagrep_control` device.

The format for commands is as follows:

```
(grep|egrep|fgrep|awk|gawk|posixawk|perl)\n
[ivwxcLlobHhnTZz]*\n
limit\n
before\n
after\n
binary\n
device\n
colors\n
keycc\n
keys\n
(filename\n)*
```

The first line is the matcher engine to run.

The second line are a few flags, as follows:
* i indicates that string matching should be case-insensitive
* v indicates that matching should be inverted.
* w indicates that only whole-word matches should be included.
* x indicates that only whole-line matches should be included.
* c indicates that only the number of matches should be output.
* L indicates that only the filenames of files without matches should be printed.
* l indicates that only the filenames of files with matches should be printed.
* o indicates that only the match itself should be printed.
* b indicates that the byte offset in the input file is prepended to each match.
* H indicates that the name of the file should be prepended to each match.
* h indicates that the names of files should not be shown.
* n indicates that the line number of each match should be shown.
* T indicates that tabs should be aligned.
* Z indicates that filenames should be terminated with a null byte.
* z indicates that line boundaries should be \0 instead of \n.

limit is an ascii integer representing the number of matches to find before exiting, or -1 for no limit.
before & after are ascii integers representing the number of lines to print before and after a line.

binary is a string (one of '', 'text', 'binary', 'without-match') that indicate how binary files should be handled.
device is a string (one of '', 'read', 'skip') that indicates how character devices should be handled.

keycc is an ascii integer representing the number of characters in the following line (not including the trailing newline)
keys is a string, and can contain newlines, but these are interpreted as part of the pattern.

Subsequent lines are all treated as filenames to open and read through.
Each line in the input (including the newlines) (not including the keys string) must not 1024 characters in length.

The output will be the grep results from each of these files concatenated together in the order given.

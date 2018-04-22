
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
[zxw]*\n
keycc\n
keys\n
(filename\n)*
```

The first line is the matcher engine to run.

The second line are a few flags, as follows:
* z indicates that line boundaries should be \0 instead of \n.
* x indicates that only whole-line matches should be included.
* w indicates that only whole-word matches should be included.

keycc is an ascii integer representing the number of characters in the following line.
keys is a string, and can contain newlines, but these are interpreted as part of the pattern.

Subsequent lines are all treated as filenames to open and read through.
Each line in the input (including the newlines) (not including the keys string) must not 1024 characters in length.

The output will be the grep results from each of these files concatenated together in the order given.

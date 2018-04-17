
SRCDIR=/usr/src/sys/dev/kgrep
MKDIR=/usr/src/sys/modules/kgrep

rm -rI $SRCDIR $MKDIR

mkdir -p $SRCDIR $MKDIR

cp -r module/sregex $SRCDIR
cp module/kgrep.c $SRCDIR

cp module/Makefile $MKDIR

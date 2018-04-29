
PROG=kagrep

SRCS=kagrep.c

MK_MAN=no

.include <bsd.prog.mk>

kagrep: grep

grep:
	cd ${.CURDIR}/grep && gmake

init:
	cd ${.CURDIR}/grep && ./bootstrap && ./configure

load:

unload:

.PHONY: load unload

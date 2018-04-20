.PATH: ${.CURDIR}/../../dev/kagrep ${.CURDIR}/grep/lib ${.CURDIR}/grep/src ${.CURDIR}/lib

SRCS=kagrep.c stdlib.c pcresearch.c dfasearch.c kwsearch.c kwset.c searchutils.c error.c xmalloc.c dfa.c regex.c xalloc-die.c memrchr.c mbrlen.c obstack.c memchr2.c mbrtowc.c mbsinit.c exitfail.c btowc.c 

LOCALE_SRCS= collate.c localcharset.c xlocale.c runetype.c setrunelocale.c localeinfo.c table.c none.c toupper.c tolower.c wctype.c isctype.c wctomb.c wctob.c wcrtomb.c 

KMOD=kagrep

CFLAGS+=-I${.CURDIR}/include -I${.CURDIR}/grep/lib -I${.CURDIR}/grep
ERROR_CFLAGS+=-Wno-unused-function

DEBUG_FLAGS=-g

.include <bsd.kmod.mk>

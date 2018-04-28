.PATH: ${.CURDIR}/grep/src ${.CURDIR}/lib ${.CURDIR}/grep/gnulib/lib 

BROKEN=wcrtomb.c mbrtowc.c 

LOCALE_SRCS= collate.c localcharset.c runetype.c setrunelocale.c localeinfo.c table.c none.c toupper.c tolower.c wctype.c isctype.c wctomb.c wctob.c searchutils.c mbrlen.c wcsnlen.c wcsncmp.c wcsdup.c wcslen.c wcsncpy.c wmemcpy.c locale/lmonetary.c locale/ldpart.c locale/fix_grouping.c locale/lnumeric.c locale/xlocale.c locale/setlocale.c stdtime/timelocal.c locale/lmessages.c locale/mbrtowc.c locale/wcrtomb.c locale/btowc.c

SRCS=kagrep.c slbuf.c match.c control.c grep.c stdlib.c pcresearch.c dfasearch.c kwsearch.c kwset.c error.c xmalloc.c dfa.c regex.c xalloc-die.c memrchr.c obstack.c memchr2.c mbsinit.c exitfail.c quotearg.c $(LOCALE_SRCS)

KMOD=kagrep

CFLAGS+=-I${.CURDIR}/include -I${.CURDIR}/grep/gnulib/lib -I${.CURDIR}/grep/lib -I${.CURDIR}/grep
ERROR_CFLAGS+=-Wno-unused-function

DEBUG_FLAGS=-g

.include <bsd.kmod.mk>

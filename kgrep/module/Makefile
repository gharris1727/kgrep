.PATH: ${.CURDIR}/../../dev/kgrep $(.CURDIR}/../../dev/kgrep/sregex/src/sregex

SRCS=kgrep.c sregex/src/sregex/sre_capture.c sregex/src/sregex/sre_palloc.c sregex/src/sregex/sre_regex.c sregex/src/sregex/sre_regex_compiler.c sregex/src/sregex/sre_vm_bytecode.c sregex/src/sregex/sre_vm_pike.c sregex/src/sregex/sre_vm_thompson.c sregex/src/sregex/sre_yyparser.y 

KMOD=kgrep

DEBUG_FLAGS=-g
YACC=bison

CFLAGS+=-I${.CURDIR}/../../dev/kgrep/sregex/src/sregex

.include <bsd.kmod.mk>

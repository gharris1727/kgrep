
.PATH: ${.CURDIR}/sregex/src/sregex

SRCS=kgrep.c sre_capture.c sre_palloc.c sre_regex.c sre_regex_compiler.c sre_vm_bytecode.c sre_vm_pike.c sre_vm_thompson.c sre_yyparser.y

KMOD=kgrep

DEBUG_FLAGS=-g
YACC=bison

CFLAGS+=-I${.CURDIR}/sregex/src/sregex

.include <bsd.kmod.mk>

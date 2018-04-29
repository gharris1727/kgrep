
SUBDIR=cli module

.include <bsd.subdir.mk>

load:
	$(MAKE) -C ${.CURDIR}/module load

unload:
	$(MAKE) -C ${.CURDIR}/module unload

init:
	$(MAKE) -C ${.CURDIR}/cli init
	$(MAKE) -C ${.CURDIR}/module init

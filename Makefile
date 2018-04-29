
SUBDIR=ugrep kgrep kagrep

.include <bsd.subdir.mk>

init:
	git submodule update --init --recursive
	$(MAKE) -C ${.CURDIR}/kagrep init

load:
	$(MAKE) -C ${.CURDIR}/kgrep load
	$(MAKE) -C ${.CURDIR}/kagrep load

unload:
	$(MAKE) -C ${.CURDIR}/kgrep unload
	$(MAKE) -C ${.CURDIR}/kagrep unload

datasets:
	$(MAKE) -C ${.CURDIR}/enron obj objlink all

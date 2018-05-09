
SUBDIR=ugrep kgrep kagrep gnugrep

.include <bsd.subdir.mk>

init:
	git submodule update --init --recursive
	$(MAKE) -C ${.CURDIR}/kagrep init
	$(MAKE) -C ${.CURDIR}/gnugrep init

load:
	$(MAKE) -C ${.CURDIR}/kgrep load
	$(MAKE) -C ${.CURDIR}/kagrep load

unload:
	$(MAKE) -C ${.CURDIR}/kgrep unload
	$(MAKE) -C ${.CURDIR}/kagrep unload

datasets:
	$(MAKE) -C ${.CURDIR}/enron obj objlink all

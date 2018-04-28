
SUBDIR=ugrep kgrep kagrep

.include <bsd.subdir.mk>

init:
	git submodule update --init --recursive
	make -C kagrep init

datasets:
	make -C enron obj objlink all


all:

build: ugrep.build kgrep.cli.build kgrep.module.build kagrep.cli.build kagrep.module.build enron.build

ugrep.build:
	cd ugrep && make

kgrep.cli.build:
	cd kgrep/cli && make

kgrep.module.build:
	cd kgrep/module && make

kagrep.cli.build:
	cd kagrep/cli && make

kagrep.module.build:
	cd kagrep/module && make

enron.build:
	cd enron && make

kagrep/cli/kagrep: kagrep.cli.build
kgrep/cli/kgrep: kgrep.cli.build
ugrep/cli/ugrep: ugrep.build

.PHONY: all build ugrep.build kgrep.cli.build kgrep.module.build kagrep.cli.build kagrep.module.build enron.build

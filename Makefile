
all:

build: ugrep.build kgrep.cli.build kgrep.module.build kagrep.cli.build kagrep.module.build enron.build

load: kgrep.module.load kagrep.module.load

ugrep.build:
	cd ugrep && $(MAKE)

kgrep.cli.build:
	cd kgrep/cli && $(MAKE)

kgrep.module.build:
	cd kgrep/module && $(MAKE) obj all

kgrep.module.load: kgrep.module.build
	cd kgrep/module && sudo $(MAKE) unload load

kagrep.cli.build:
	cd kagrep/cli && $(MAKE)

kagrep.module.build:
	cd kagrep/module && $(MAKE) obj all

kagrep.module.load: kagrep.module.build
	cd kgrep/module && sudo $(MAKE) unload load

enron.build:
	cd enron && $(MAKE)

kagrep/cli/kagrep: kagrep.cli.build
kgrep/cli/kgrep: kgrep.cli.build
ugrep/cli/ugrep: ugrep.build

.PHONY: all build load ugrep.build kgrep.cli.build kgrep.module.build kgrep.module.load kagrep.cli.build kagrep.module.build kagrep.module.load enron.build

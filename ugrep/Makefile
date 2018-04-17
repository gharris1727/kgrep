
ugrep: ugrep.o sregex
	cc ugrep.o -o ugrep -L./sregex/ -Wl,-R./sregex/ -lsregex

ugrep.o: ugrep.c

.PHONY: clean sregex

sregex:
	cd sregex && gmake

clean:
	cd sregex && gmake clean
	rm *.o ugrep


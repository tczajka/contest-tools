.PHONY: all
all: bin/test_random

.PHONY: clean
clean:
	rm -fr bin

bin:
	mkdir -p bin

bin/test_random: tests/random.cc src/random.h | bin
	g++ -Wall -o $@ -Isrc $<

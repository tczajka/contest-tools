.PHONY: all
all: bin/test_random bin/test_reader

.PHONY: clean
clean:
	rm -fr bin

bin:
	mkdir -p bin

bin/test_random: tests/random.cc src/random.h | bin
	g++ -Wall -o $@ -Isrc $<

bin/test_reader: tests/reader.cc src/reader.h | bin
	g++ -Wall -o $@ -Isrc $<

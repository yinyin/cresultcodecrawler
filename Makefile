NOOP = $(`which true`)

all: cresultcodecrawler

cresultcodecrawler: src/main.cc src/chars.o src/codeio.o
	$(CXX) -std=c++14 -Wall -o $@ $^

src/chars.o: src/chars.c src/chars.h
	$(CC) -c -Wall -o $@ $<

src/codeio.o: src/codeio.cc src/codeio.hh src/chars.h src/errors.h
	$(CXX) -std=c++14 -c -Wall -o $@ $<

src/errors.h: src/codeio.cc src/main.cc
	./cresultcodecrawler -o $@ -p CRESTCODE -b -1 $^

errors-stub: src/errors.h
	$(NOOP)

clean:
	$(RM) cresultcodecrawler \
		src/chars.o \
		src/codeio.o

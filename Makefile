
cresultcodecrawler: src/main.cc src/chars.o src/codeio.o
	$(CXX) -Wall -o $@ $^

src/chars.o: src/chars.c src/chars.h
	$(CC) -c -Wall -o $@ $<

src/codeio.o: src/codeio.cc src/codeio.hh src/chars.h
	$(CXX) -c -Wall -o $@ $<

clean:
	$(RM) cresultcodecrawler \
		src/chars.o

all: test example

test: test.cpp jsrw.h
	g++ -std=c++17 -Wall -g -o $@ $<

example: example.cpp jsrw.h
	g++ -std=c++17 -Wall -g -o $@ $<

clean:
	rm -fr *.o test test.dSYM

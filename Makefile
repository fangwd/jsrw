all: test

test: test.cpp jsrw.h
	g++ -std=c++17 -Wall -g -o $@ $<

clean:
	rm -fr *.o test test.dSYM

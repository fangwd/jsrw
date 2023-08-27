all: test

test: test.cpp jsrw.h
	g++ -std=c++17 -Wall -g -o $@ test.cpp

clean:
	rm -fr *.o test test.dSYM


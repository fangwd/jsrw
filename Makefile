all: test

test: jsrw.cpp test.cpp
	g++ -std=c++11 -Wall -g -o $@ $^

clean:
	rm -fr *.o test test.dSYM


CXX      ?= g++
CXXFLAGS ?= -std=c++11 -Wall -Wextra -pedantic -O2

.PHONY: test clean

test: tests/test_susu.cpp src/susu.cpp src/susu.h
	$(CXX) $(CXXFLAGS) -o run_tests tests/test_susu.cpp src/susu.cpp
	./run_tests

clean:
	rm -f run_tests

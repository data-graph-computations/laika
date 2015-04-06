CC  = gcc
CXX = g++
CFLAGS = -O3 -Wall
CXXFLAGS = -std=c++11 -O3 -Wall
LDFLAGS =

.PHONY: all clean lint

HEADERS = common.h hilbert/hilbert.h
CXXSOURCES = reorder.cpp

TEST ?= 0
DEBUG ?= 0

all: lint reorder

lint:
	./cpplint.py *.cpp *.h

hilbert:
	$(CC) $(CFLAGS) -c libhilbert/hilbert.c

reorder: $(CXXSOURCES) hilbert
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -DTEST=$(TEST) -DDEBUG=$(DEBUG) -o reorder.out hilbert.o $(CXXSOURCES)

clean:
	rm -f *~ *.o *.out

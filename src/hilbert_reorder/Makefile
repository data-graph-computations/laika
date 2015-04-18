CC  = gcc
CXX = g++
CFLAGS = -O3 -Wall
CXXFLAGS = -std=c++11 -O3 -Wall
LDFLAGS =
ROOT = ../../

.PHONY: all clean lint

HEADERS = common.h hilbert/hilbert.h
CXXSOURCES = reorder.cpp

TEST ?= 0
DEBUG ?= 0

all: lint reorder

lint:
	$(ROOT)/cpplint.py --root=src/hilbert_reorder *.cpp *.h

hilbert:
	$(CC) $(CFLAGS) -c libhilbert/hilbert.c

reorder: $(CXXSOURCES) hilbert
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -DTEST=$(TEST) -DDEBUG=$(DEBUG) -o reorder.out hilbert.o $(CXXSOURCES)

clean:
	rm -f *~ *.o *.out

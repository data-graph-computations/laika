CC  = gcc
CXX = g++
CFLAGS = -O3 -Wall
CXXFLAGS = -fcilkplus -std=c++11 -O3 -Wall -m64
LDFLAGS = -lcilkrts
ROOT = ../../

.PHONY: all clean lint

HEADERS = common.h io.h
CXXSOURCES = compute.cpp io.cpp

TEST ?= 0
DEBUG ?= 0

all: lint compute

lint:
	$(ROOT)/cpplint.py --root=src/graph_compute *.cpp *.h

compute: $(CXXSOURCES)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -DTEST=$(TEST) -DDEBUG=$(DEBUG) -o compute $(CXXSOURCES)

clean:
	rm -f *~ *.o *.out

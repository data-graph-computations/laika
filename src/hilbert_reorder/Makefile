CC  ?= gcc
CXX ?= g++
CFLAGS = -O3 -Wall
CXXFLAGS = -std=c++11 -O3 -Wall
LDFLAGS =
ROOT = ../../

.PHONY: all clean lint

HEADERS = common.h hilbert/hilbert.h io.h
CXXSOURCES = reorder.cpp io.cpp

TEST ?= 0
DEBUG ?= 0
DEFS = -DTEST=$(TEST) -DDEBUG=$(DEBUG)

ifneq ($(HILBERTBITS),)
	DEFS += -DHILBERTBITS=$(HILBERTBITS)
endif

ifneq ($(PARALLEL),)
	DEFS += -DPARALLEL=$(PARALLEL)
endif

all: lint reorder

lint:
	$(ROOT)/cpplint.py --root=src/hilbert_reorder *.cpp *.h

hilbert:
	$(CC) $(CFLAGS) -c libhilbert/hilbert.c

reorder: $(CXXSOURCES) hilbert
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(DEFS) -o reorder hilbert.o $(CXXSOURCES)

clean:
	rm -f *~ *.o *.out reorder

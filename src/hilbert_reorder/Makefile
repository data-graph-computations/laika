CC  ?= gcc
CXX ?= g++
CFLAGS = -O3 -Wall
CXXFLAGS = -fcilkplus -std=c++11 -O3 -Wall -m64
LDFLAGS = -lcilkrts -lrt -ldl
ROOT = ../../

.PHONY: all clean lint

LIBS = ../libgraphio/libgraphio.o hilbert.o
HEADERS = common.h hilbert/hilbert.h io.h
SOURCES = reorder.cpp io.cpp

TEST ?= 0
DEBUG ?= 0
DEFS = -DTEST=$(TEST) -DDEBUG=$(DEBUG)

ifneq ($(HILBERTBITS),)
	DEFS += -DHILBERTBITS=$(HILBERTBITS)
endif

ifneq ($(PARALLEL),)
	DEFS += -DPARALLEL=$(PARALLEL)
endif

ifneq ($(BFS),)
	DEFS += -DBFS=$(BFS)
endif

ifneq ($(RANDOM),)
	DEFS += -DRANDOM=$(RANDOM)
endif

all: lint reorder

lint:
	$(ROOT)/cpplint.py --root=src/hilbert_reorder *.cpp *.h

hilbert:
	$(CC) $(CFLAGS) -c libhilbert/hilbert.c

reorder: $(SOURCES) hilbert
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(DEFS) -o reorder $(LIBS) $(SOURCES)

clean:
	rm -f *~ *.o *.out reorder

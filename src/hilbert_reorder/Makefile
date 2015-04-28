CC  ?= gcc
CXX ?= g++
CFLAGS = -O3 -Wall
CXXFLAGS = -fcilkplus -std=c++11 -O3 -Wall -m64
LDFLAGS = -lcilkrts -lrt -ldl
ROOT = ../../

.PHONY: all clean lint

HEADERS = common.h hilbert/hilbert.h io.h
REORDER_SOURCES = reorder.cpp io.cpp
UNDIRECT_SOURCES = undirect.cpp io.cpp

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

all: lint reorder undirect

lint:
	$(ROOT)/cpplint.py --root=src/hilbert_reorder *.cpp *.h

hilbert:
	$(CC) $(CFLAGS) -c libhilbert/hilbert.c

reorder: $(REORDER_SOURCES) hilbert
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(DEFS) -o reorder hilbert.o $(REORDER_SOURCES)

undirect: $(UNDIRECT_SOURCES)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o undirect $(UNDIRECT_SOURCES)

clean:
	rm -f *~ *.o *.out reorder

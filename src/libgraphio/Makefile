CC  ?= gcc
CXX ?= g++
CFLAGS = -O3 -Wall
CXXFLAGS = -fcilkplus -std=c++11 -O3 -Wall -m64
LDFLAGS = -lcilkrts -lrt -ldl
ROOT = ../../

.PHONY: all clean lint

HEADERS = common.h libgraphio.h
SOURCES = adjlist.cpp binadjlist.cpp
OBJECTS = adjlist.o binadjlist.o
PRODUCT = libgraphio.o

TEST ?= 1
DEBUG ?= 0
DEFS = -DTEST=$(TEST) -DDEBUG=$(DEBUG)

ifneq ($(PARALLEL),)
	DEFS += -DPARALLEL=$(PARALLEL)
endif

all: lint libgraphio.o

lint:
	$(ROOT)/cpplint.py --root=src $(SOURCES) $(HEADERS)

$(OBJECTS): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(DEFS) -c $(SOURCES)

libgraphio.o: $(OBJECTS)
	ld -r -o libgraphio.o $(OBJECTS)

clean:
	rm -f *~ *.o *.out

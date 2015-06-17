CC  ?= gcc
CXX ?= g++
CFLAGS = -O3 -Wall
CXXFLAGS = -fcilkplus -std=c++11 -O3 -Wall -m64
LDFLAGS = -lcilkrts -lrt -ldl
ROOT = ../../

.PHONY: all clean lint

LIBS = ../libgraphio/libgraphio.o
HEADERS = common.h io.h uniform_distribution.h stats.h edge_generator.h
SOURCES = graphgen2.cpp io.cpp stats.cpp edge_generator.cpp

TEST ?= 0
DEBUG ?= 0

DEFS = -DTEST=$(TEST) -DDEBUG=$(DEBUG)

ifneq ($(DIST_UNIFORM),)
	DEFS += -DDIST_UNIFORM=$(DIST_UNIFORM)
endif

ifneq ($(MIN_COORD),)
	DEFS += -DMIN_COORD=$(MIN_COORD)
endif

ifneq ($(MAX_COORD),)
	DEFS += -DMAX_COORD=$(MAX_COORD)
endif

ifneq ($(MAX_EDGE_LENGTH),)
	DEFS += -DMAX_EDGE_LENGTH=$(MAX_EDGE_LENGTH)
endif

ifneq ($(PARALLEL),)
	DEFS += -DPARALLEL=$(PARALLEL)
endif

ifneq ($(GRID_SIZE),)
	DEFS += -DGRID_SIZE=$(GRID_SIZE)
endif

all: lint graphgen2

lint:
	$(ROOT)/cpplint.py --root=src/graphgen2 *.cpp *.h

graphgen2: $(SOURCES)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(DEFS) -o graphgen2 $(SOURCES) $(LIBS)

clean:
	rm -f *~ *.o *.out graphgen2

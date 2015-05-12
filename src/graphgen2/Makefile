CC  ?= gcc
CXX ?= g++
CFLAGS = -O3 -Wall
CXXFLAGS = -fcilkplus -std=c++11 -O3 -Wall -m64
LDFLAGS = -lcilkrts -lrt -ldl
ROOT = ../../

.PHONY: all clean lint

HEADERS = common.h io.h uniform_distribution.h
SOURCES = graphgen2.cpp io.cpp

TEST ?= 0
DEBUG ?= 0

DEFS = -DTEST=$(TEST) -DDEBUG=$(DEBUG)

ifneq ($(DIST_UNIFORM),)
	DEFS += -DDIST_UNIFORM=$(DIST_UNIFORM)
endif

all: lint graphgen2

lint:
	$(ROOT)/cpplint.py --root=src/graphgen2 *.cpp *.h

graphgen2: $(SOURCES)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(DEFS) -o graphgen2 $(SOURCES)

clean:
	rm -f *~ *.o *.out graphgen2

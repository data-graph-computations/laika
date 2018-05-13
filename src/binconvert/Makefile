CC  ?= gcc
CXX ?= g++
CFLAGS = -O3 -Wall
CXXFLAGS = -fcilkplus -std=c++11 -O3 -Wall -m64
LDFLAGS = -lcilkrts -lrt -ldl
ROOT = ../../

.PHONY: all clean lint

LIBS = ../libgraphio/libgraphio.o
HEADERS = common.h io.h

TEST ?= 1
DEBUG ?= 0

DEFS = -DTEST=$(TEST) -DDEBUG=$(DEBUG)

ifneq ($(PARALLEL),)
	DEFS += -DPARALLEL=$(PARALLEL)
endif

all: lint binconvert humconvert

lint:
	$(ROOT)/cpplint.py --root=src/binconvert *.cpp *.h

binconvert: $(SOURCES)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(DEFS) -o binconvert binconvert.cpp $(LIBS)

humconvert: $(SOURCES)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(DEFS) -o humconvert humconvert.cpp $(LIBS)

clean:
	rm -f *~ *.o *.out binconvert

CC  ?= gcc-5
CXX ?= g++-5
CFLAGS = -O3 -Wall
CXXFLAGS = -fcilkplus -std=c++11 -O3 -Wall -m64
LDFLAGS = -lcilkrts -ldl
ROOT = ../../

.PHONY: all clean lint

LIBS = ../libgraphio/libgraphio.o
HEADERS = common.h io.h
SOURCES = io.cpp dv_reorder.cpp

TEST ?= 0
DEBUG ?= 0
DEFS = -DTEST=$(TEST) -DDEBUG=$(DEBUG)

all: lint dv_reorder

lint:
	$(ROOT)/cpplint.py --root=src/discounted_value_reorder $(SOURCES) $(HEADERS)

dv_reorder: $(SOURCES)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(DEFS) -o dv_reorder $(LIBS) $(SOURCES)

clean:
	rm -f *~ *.o *.out dv_reorder

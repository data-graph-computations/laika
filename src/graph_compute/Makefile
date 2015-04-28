CC  ?= gcc
CXX ?= g++
CFLAGS = -O3 -Wall
CXXFLAGS = -fcilkplus -std=c++11 -O3 -Wall -m64
LDFLAGS = -lcilkrts -ldl -lrt
ROOT = ../../

HEADERS = common.h io.h
CXXSOURCES = compute.cpp io.cpp

TEST ?= 0
DEBUG ?= 0
DEFS = -DTEST=$(TEST) -DDEBUG=$(DEBUG)

ifneq ($(PRIORITY_GROUP_BITS),)
	DEFS += -DPRIORITY_GROUP_BITS=$(PRIORITY_GROUP_BITS)
endif

ifneq ($(PARALLEL),)
	DEFS += -DPARALLEL=$(PARALLEL)
endif

ifneq ($(BASELINE),)
	DEFS += -DBASELINE=$(BASELINE)
endif

all: lint compute

lint:
	$(ROOT)/cpplint.py --root=src/graph_compute *.cpp *.h

compute: $(CXXSOURCES)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(DEFS) -o compute $(CXXSOURCES)

clean:
	rm -f *~ *.o *.out compute

.PHONY: all clean lint

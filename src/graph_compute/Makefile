CC  ?= gcc
CXX ?= g++
CFLAGS = -O3 -Wall
CXXFLAGS = -fcilkplus -std=c++11 -O3 -Wall -m64
LDFLAGS = -L/usr/lib64 -lcilkrts -ldl -lrt
ROOT = ../../

LIBS = ../libgraphio/libgraphio.o
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

ifneq ($(D0_BSP),)
	DEFS += -DD0_BSP=$(D0_BSP)
endif

ifneq ($(D1_PRIO),)
	DEFS += -DD1_PRIO=$(D1_PRIO)
endif

ifneq ($(D1_CHUNK),)
	DEFS += -DD1_CHUNK=$(D1_CHUNK)
endif

ifneq ($(D1_PHASE),)
	DEFS += -DD1_PHASE=$(D1_PHASE)
endif

ifneq ($(CHUNK_BITS),)
	DEFS += -DCHUNK_BITS=$(CHUNK_BITS)
endif

ifneq ($(DISTANCE),)
	DEFS += -DDISTANCE=$(DISTANCE)
endif

ifneq ($(BASELINE),)
	DEFS += -DBASELINE=$(BASELINE)
endif

all: lint compute

lint:
	$(ROOT)/cpplint.py --root=src/graph_compute *.cpp *.h

compute: $(CXXSOURCES)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(DEFS) -o compute $(CXXSOURCES) $(LIBS)

clean:
	rm -f *~ *.o *.out compute

.PHONY: all clean lint

CC  ?= gcc
CXX ?= g++
CFLAGS = -O3 -Wall 
CXXFLAGS = -fcilkplus -std=c++11 -O3 -Wall -m64 -march=native -mtune=native -pthread
LDFLAGS = -L/usr/lib64 -lcilkrts -ldl -lrt
ROOT = ../../

LIBS = ../libgraphio/libgraphio.o
HEADERS = common.h io.h numa_init.h
CXXSOURCES = compute.cpp io.cpp numa_init.cpp

TEST ?= 0
DEBUG ?= 0
PAGERANK ?= 1
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

ifneq ($(D1_NUMA),)
	DEFS += -DD1_NUMA=$(D1_NUMA)
endif

ifneq ($(NUMA_INIT),)
	DEFS += -DNUMA_INIT=$(NUMA_INIT)
endif

ifneq ($(NUMA_WORKERS),)
	DEFS += -DNUMA_WORKERS=$(NUMA_WORKERS)
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

ifneq ($(PAGERANK),)
	DEFS += -DPAGERANK=$(PAGERANK)
endif

all: lint compute

lint:
	$(ROOT)/cpplint.py --root=src/graph_compute *.cpp *.h

compute: $(CXXSOURCES)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(DEFS) -o compute $(CXXSOURCES) $(LIBS)

clean:
	rm -f *~ *.o *.out compute

.PHONY: all clean lint

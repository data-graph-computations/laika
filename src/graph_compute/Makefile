CC  ?= gcc
CXX ?= g++
CFLAGS = -O3 -Wall 
CXXFLAGS = -fcilkplus -std=c++11 -O3 -Wall -m64 -march=native -mtune=native -pthread -ffast-math -fgcse-las
LDFLAGS = -L/usr/lib64 -lcilkrts -ldl -lrt
ROOT = ../../

LIBS = ../libgraphio/libgraphio.o
HEADERS = common.h update_function.h io.h numa_init.h concurrent_queue.h
CXXSOURCES =  compute.cpp io.cpp numa_init.cpp

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

ifneq ($(D1_CHROM),)
	DEFS += -DD1_CHROM=$(D1_CHROM)
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

ifneq ($(NUMA_STEAL),)
	DEFS += -DNUMA_STEAL=$(NUMA_STEAL)
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

ifneq ($(MASS_SPRING_DASHPOT),)
	DEFS += -DMASS_SPRING_DASHPOT=$(MASS_SPRING_DASHPOT)
endif

ifneq ($(IN_PLACE),)
	DEFS += -DIN_PLACE=$(IN_PLACE)
endif

ifneq ($(TEST_CONVERGENCE),)
	DEFS += -DTEST_CONVERGENCE=$(TEST_CONVERGENCE)
endif

ifneq ($(PRINT_EDGE_LENGTH_HISTOGRAM),)
	DEFS += -DPRINT_EDGE_LENGTH_HISTOGRAM=$(PRINT_EDGE_LENGTH_HISTOGRAM)
endif

ifneq ($(USE_GLOBAL_REST_LENGTH),)
	DEFS += -DUSE_GLOBAL_REST_LENGTH=$(USE_GLOBAL_REST_LENGTH)
endif

ifneq ($(TEST_SIMPLE_AND_UNDIRECTED),)
	DEFS += -DTEST_SIMPLE_AND_UNDIRECTED=$(TEST_SIMPLE_AND_UNDIRECTED)
endif

ifneq ($(VERBOSE),)
	DEFS += -DVERBOSE=$(VERBOSE)
endif

ifneq ($(HUGE_GRAPH_SUPPORT),)
	DEFS += -DHUGE_GRAPH_SUPPORT=$(HUGE_GRAPH_SUPPORT)
endif

all: lint compute

lint:
	$(ROOT)/cpplint.py --root=src/graph_compute *.cpp *.h

compute: $(CXXSOURCES)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(DEFS) -o compute $(CXXSOURCES) $(LIBS)

clean:
	rm -f *~ *.o *.out compute

.PHONY: all clean lint

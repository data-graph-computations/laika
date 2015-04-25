TMP ?= /tmp/predrag/6.886/current_run
ORIGINAL_NODES_FILE ?= $(TMP)/nodes_original.node
ORIGINAL_EDGES_FILE ?= $(TMP)/edges_original.adjlist
REORDERED_NODES_FILE ?= $(TMP)/nodes_reordered.node
REORDERED_EDGES_FILE ?= $(TMP)/edges_reordered.adjlist

OUTPUT ?= $(TMP)/output.out

GRAPH_SIZE ?= 10000000
ROUNDS ?= 100
PARALLEL ?= 0
PRIORITY_GROUP_BITS ?= 8
HILBERTBITS ?= 4

all: build-full

build-full: build-hilbert-reorder build-graph-compute

clean: clean-hilbert-reorder clean-graph-compute

distclean: clean
	@cd $(TMP) && rm -f *.adjlist *.node *.out *.txt

clean-hilbert-reorder:
	cd src/hilbert_reorder && $(MAKE) clean

clean-graph-compute:
	cd src/graph_compute && $(MAKE) clean

build-hilbert-reorder:
	cd src/hilbert_reorder && $(MAKE)

build-graph-compute-baseline: clean-graph-compute
build-graph-compute-baseline: PRIORITY_GROUP_BITS = 0
build-graph-compute-baseline: BASELINE = 1
build-graph-compute-baseline: build-graph-compute

build-graph-compute-optimized: clean-graph-compute
build-graph-compute-optimized: BASELINE = 0
build-graph-compute-optimized: build-graph-compute

build-graph-compute:
	cd src/graph_compute && $(MAKE)

gen-graph:
	python src/graphgen/graphgen.py $(GRAPH_SIZE) $(ORIGINAL_NODES_FILE) $(ORIGINAL_EDGES_FILE)

reorder-graph: build-hilbert-reorder
	src/hilbert_reorder/reorder $(ORIGINAL_NODES_FILE) $(ORIGINAL_EDGES_FILE) $(REORDERED_NODES_FILE) $(REORDERED_EDGES_FILE)

run-original:
	src/graph_compute/compute $(ROUNDS) $(ORIGINAL_EDGES_FILE) 2>&1 >$(TMP)/original.out

run-reordered:
	src/graph_compute/compute $(ROUNDS) $(REORDERED_EDGES_FILE) 2>&1 >$(TMP)/reordered.out

run-original-concat:
	src/graph_compute/compute $(ROUNDS) $(ORIGINAL_EDGES_FILE) 2>&1 >>$(OUTPUT)

run-reordered-concat:
	src/graph_compute/compute $(ROUNDS) $(REORDERED_EDGES_FILE) 2>&1 >>$(OUTPUT)

run-full: clean gen-graph reorder-graph build-graph-compute-baseline run-original build-graph-compute-optimized run-reordered

.PHONY: clean run-full

.DEFAULT: build-full

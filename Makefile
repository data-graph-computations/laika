.PHONY: clean run-full

ORIGINAL_NODES_FILE = tmp/nodes_original.node
ORIGINAL_EDGES_FILE = tmp/edges_original.adjlist
REORDERED_NODES_FILE = tmp/nodes_reordered.node
REORDERED_EDGES_FILE = tmp/edges_reordered.adjlist

clean: clean-hilbert-reorder clean-graph-compute
	@cd tmp && rm -f *.adjlist *.node *.out *.txt

clean-hilbert-reorder:
	cd src/hilbert_reorder && $(MAKE) clean

clean-graph-compute:
	cd src/graph_compute && $(MAKE) clean

build-hilbert-reorder:
	cd src/hilbert_reorder && $(MAKE)

build-graph-compute:
	cd src/graph_compute && $(MAKE)

run-full: clean gen-graph reorder-graph run-original run-reordered

gen-graph:
	python src/graphgen/graphgen.py $(ORIGINAL_NODES_FILE) $(ORIGINAL_EDGES_FILE)

reorder-graph: build-hilbert-reorder
	src/hilbert_reorder/reorder $(ORIGINAL_NODES_FILE) $(ORIGINAL_EDGES_FILE) $(REORDERED_NODES_FILE) $(REORDERED_EDGES_FILE)

run-original: build-graph-compute
	src/graph_compute/compute $(ORIGINAL_EDGES_FILE) 2>&1 >tmp/original.out

run-reordered: build-graph-compute
	src/graph_compute/compute $(REORDERED_EDGES_FILE) 2>&1 >tmp/reordered.out

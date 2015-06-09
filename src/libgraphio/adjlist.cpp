#include <cstdio>
#include <iostream>
#include <string>
#include <cstring>
#include "./libgraphio.h"
#include "./common.h"

#define ADJGRAPH "AdjacencyGraph"

static void cleanupOnFormatError(FILE * const input,
                                 const std::string& type, const int line) {
  int result;
  std::cerr << "ERROR: Illegal " << type
            << " file format on line " << line << std::endl;
  result = fclose(input);
  assert(result == 0);
}

int adjlistfile_read(const std::string& filepath,
                     EdgeListBuilder * const builder) {
  FILE * input = fopen(filepath.c_str(), "r");
  if (input == NULL) {
    std::cerr << "ERROR: Couldn't open file " << filepath << std::endl;
    return -1;
  }

  char adjGraph[15];
  vid_t cntNodes, totalEdges, result;

  // read header line of adjlist file
  result = fscanf(input, "%15s\n%ld\n%ld\n", adjGraph, &cntNodes, &totalEdges);
  if (result != 3 || strcmp(ADJGRAPH, adjGraph) != 0) {
    cleanupOnFormatError(input, "edge", 1);
    return -1;
  }

  builder->set_node_count(cntNodes);
  builder->set_total_edge_count(totalEdges);

  for (vid_t i = 0; i < cntNodes; ++i) {
    vid_t firstEdgeIndex;
    result = fscanf(input, "%lu\n", &firstEdgeIndex);
    if (result != 1) {
      cleanupOnFormatError(input, "edge", i + 3);
      return -1;
    }
    builder->set_first_edge_of_node(i, firstEdgeIndex);
  }

  for (vid_t i = 0; i < totalEdges; ++i) {
    vid_t destination;
    result = fscanf(input, "%lu\n", &destination);
    if (result != 1) {
      cleanupOnFormatError(input, "edge", i + cntNodes + 3);
      return -1;
    }
    builder->create_edge(i, destination);
  }

  builder->build();
  return 0;
}
